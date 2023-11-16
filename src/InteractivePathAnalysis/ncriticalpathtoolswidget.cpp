#include "ncriticalpathtoolswidget.h"
#include "custommenu.h"
#include "ncriticalpathsettings.h"
#include "ncriticalpaththeme.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QDir>
#include <QFileInfo>
#include <QIntValidator>
#include <QFormLayout>

#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

#ifndef STANDALONE_APP
#include "../NewProject/ProjectManager/project_manager.h"
#include "../Compiler/CompilerOpenFPGA_ql.h"
#include "../Compiler/QLSettingsManager.h"
#endif

NCriticalPathToolsWidget::NCriticalPathToolsWidget(
#ifndef STANDALONE_APP
        FOEDAG::Compiler* compiler,
#endif
        QWidget* parent)
    : QWidget(parent)
#ifndef STANDALONE_APP
    , m_compiler(compiler)
#endif
    , m_process("vpr")
    , m_parameters(std::make_shared<NCriticalPathParameters>())
{
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(NCriticalPathTheme::instance().borderSize());
    setLayout(layout);

    QPushButton* bnPathsOptions = new QPushButton("Configuration");
    layout->addWidget(bnPathsOptions);
    setupCriticalPathsOptionsMenu(bnPathsOptions);

    // bnRunPnRView
    m_bnRunPnRView = new QPushButton("Run P&&R View");
    layout->addWidget(m_bnRunPnRView);
    connect(m_bnRunPnRView, &QPushButton::clicked, this, &NCriticalPathToolsWidget::runPnRView);
    connect(&m_process, &Process::runStatusChanged, this, [this](bool isRunning){
        m_bnRunPnRView->setEnabled(!isRunning);
        m_isFirstTimeConnectedToParticularPnRViewInstance = true; // to get new path list on next PnRView run
        m_isPathListSettingsChanged = true;
        emit PnRViewRunStatusChanged(isRunning);
    });

#ifdef STANDALONE_APP
    setWindowTitle("Client");

    QPushButton* bnFOEDAGProj = new QPushButton("FOEDAG proj...");
    layout->addWidget(bnFOEDAGProj);
    setupProjectMenu(bnFOEDAGProj);
#endif


#ifndef BYPASS_AUTO_VPR_VIEW_RUN
    runPnRView(); 
#endif

    onConnectionStatusChanged(false);
}

void NCriticalPathToolsWidget::onPathListReceived()
{
    m_isPathListSettingsChanged = false;
}

QString NCriticalPathToolsWidget::projectLocation()
{
#ifdef STANDALONE_APP
    return m_leProj->text();
#else
    return m_compiler->ProjManager()->getProjectPath();
#endif
}

QString NCriticalPathToolsWidget::vprBaseCommand()
{
#ifdef STANDALONE_APP
    QString projPath = projectLocation();
    if (!projPath.isEmpty() && QDir().exists(projPath)) {

        QString projName = QFileInfo(projPath).fileName();
        QString projSettingsFilePath = projPath + "/" + projName + ".json";

        QString deviceLayout;
        QFile file(projSettingsFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray jsonData = file.readAll();
            file.close();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                QJsonObject jsonObject = jsonDoc.object();
                deviceLayout = jsonObject["general"].toObject()["device"].toObject()["layout"].toObject()["userValue"].toString();
            }
        }
        if (!deviceLayout.isEmpty()) {
            QList<QString> cmd;
            cmd << "/home/work/workspace/repos/aurora2/dev/bin/vpr";
            cmd << "/home/work/workspace/repos/aurora2/device_data/QLF_K6N10/TSMC/16nm/LVT/WORST/vpr.xml";
            cmd << projName+"_post_synth.blif";
            cmd << "--device";
            cmd << deviceLayout;
            cmd << "--timing_analysis";
            cmd << "on";
            cmd << "--constant_net_method";
            cmd << "route";
            cmd << "--clock_modeling";
            cmd << "ideal";
            cmd << "--exit_before_pack";
            cmd << "off";
            cmd << "--circuit_format";
            cmd << "eblif";
            cmd << "--absorb_buffer_luts";
            cmd << "off";
            cmd << "--route_chan_width";
            cmd << "180";
            cmd << "--flat_routing";
            cmd << "false";
            cmd << "--gen_post_synthesis_netlist";
            cmd << "on";
            cmd << "--post_synth_netlist_unconn_inputs";
            cmd << "gnd";
            cmd << "--post_synth_netlist_unconn_outputs";
            cmd << "unconnected";
            cmd << "--timing_report_npaths";
            cmd << m_leNCriticalPathNum->text();
            cmd << "--timing_report_detail";
            cmd << "netlist";
            cmd << "--allow_dangling_combinational_nodes";
            cmd << "on";
            return cmd.join(" ");
        }
    } else {
        qCritical() << "cannot run P&RView due to empty projPath";
    }
    return "";
#else
    return static_cast<FOEDAG::CompilerOpenFPGA_ql*>(m_compiler)->BaseVprCommand().c_str();
#endif
}

void NCriticalPathToolsWidget::restoreConfiguration()
{
    NCriticalPathSettings::instance().load();
    m_parameters->load();
    resetConfigurationMenu();
}

void NCriticalPathToolsWidget::setupCriticalPathsOptionsMenu(QPushButton* caller)
{
    if (m_pathsOptionsMenu) {
        return;
    }

    m_pathsOptionsMenu = new CustomMenu(caller);
    connect(m_pathsOptionsMenu, &CustomMenu::declined, this, [this](){
        restoreConfiguration();
        m_isPathListSettingsChanged = false;
    });
    connect(m_pathsOptionsMenu, &CustomMenu::accepted, this, [this](){
        if (m_isPathListSettingsChanged) {
            emit pathListRequested("autorefresh because path list configuration changed");
        }
        if (m_cbSaveSettings->isChecked()) {
            saveConfiguration();
        }
    });

    QFormLayout* formLayout = new QFormLayout;
    m_pathsOptionsMenu->addContentLayout(formLayout);

    //
    m_cbHighlightMode = new QComboBox;
    m_cbHighlightMode->addItem("Flylines");
    m_cbHighlightMode->addItem("Flylines Delays");
    m_cbHighlightMode->addItem("Routing");
    m_cbHighlightMode->addItem("Routing Delays");

    connect(m_cbHighlightMode, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_parameters->setHighLightMode(index);
        emit highLightModeChanged();
    });

    formLayout->addRow(new QLabel(tr("Hightlight mode:")), m_cbHighlightMode);

    //
    m_cbPathType = new QComboBox;
    m_cbPathType->addItem("setup");
    m_cbPathType->addItem("hold");
    //    m_cbPathType->addItem("skew");
    connect(m_cbPathType, &QComboBox::currentTextChanged, this, [this](const QString& newText) {
        m_parameters->setPathType(newText);
        m_isPathListSettingsChanged = true;
    });
    formLayout->addRow(new QLabel(tr("Type:")), m_cbPathType);

    //
    m_cbDetail = new QComboBox;
    m_cbDetail->addItem("netlist");
    m_cbDetail->addItem("aggregated");
    m_cbDetail->addItem("detailed routing");
    m_cbDetail->addItem("debug");
    connect(m_cbDetail, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_parameters->setDetailLevel(index);
        m_isPathListSettingsChanged = true;
    });
    formLayout->addRow(new QLabel(tr("Report detail:")), m_cbDetail);

    //
    m_leNCriticalPathNum = new QLineEdit();
    QIntValidator intValidator(m_leNCriticalPathNum);
    m_leNCriticalPathNum->setValidator(&intValidator);

    connect(m_leNCriticalPathNum, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_parameters->setCriticalPathNum(text.toInt());
        m_isPathListSettingsChanged = true;
    });
    formLayout->addRow(new QLabel(tr("Paths num limit:")), m_leNCriticalPathNum);

    m_cbSaveSettings = new QCheckBox("Save settings");
    m_cbSaveSettings->setChecked(NCriticalPathSettings::instance().getSavePathListSettings());
    connect(m_cbSaveSettings, &QCheckBox::toggled, this, [](bool checked){
        NCriticalPathSettings::instance().setSavePathListSettings(checked);
    });
    formLayout->addRow(m_cbSaveSettings);

    resetConfigurationMenu();
}

void NCriticalPathToolsWidget::resetConfigurationMenu()
{
    m_cbHighlightMode->blockSignals(true);
    m_cbHighlightMode->setCurrentIndex(m_parameters->getHighLightMode());
    m_cbHighlightMode->blockSignals(false);

    m_cbPathType->blockSignals(true);
    m_cbPathType->setCurrentText(m_parameters->getPathType());
    m_cbPathType->blockSignals(false);

    m_cbDetail->blockSignals(true);
    m_cbDetail->setCurrentIndex(m_parameters->getDetailLevel());
    m_cbDetail->blockSignals(false);

    m_leNCriticalPathNum->blockSignals(true);
    m_leNCriticalPathNum->setText(QString::number(m_parameters->getCriticalPathNum()));
    m_leNCriticalPathNum->blockSignals(false);
}

void NCriticalPathToolsWidget::saveConfiguration()
{
    m_parameters->saveToSettings();
}

#ifdef STANDALONE_APP
void NCriticalPathToolsWidget::setupProjectMenu(QPushButton* caller)
{
    if (m_FOEDAGProjMenu) {
        return;
    }
    m_FOEDAGProjMenu = new CustomMenu(caller);

    QFormLayout* layout = new QFormLayout;
    m_FOEDAGProjMenu->addContentLayout(layout);

    //
    m_leProj = new QLineEdit;
    m_leProj->setPlaceholderText("set valid project path here...");

    QSettings settings;
    QVariant valProjPath = settings.value("projPath");
    if (valProjPath.isValid()) {
        m_leProj->setText(valProjPath.toString());
    }
    connect(m_leProj, &QLineEdit::textChanged, this, [](const QString& projPath){
        QSettings settings;
        settings.setValue("projPath", projPath);
    });
    layout->addRow(new QLabel(tr("FOEDAG project location:")), m_leProj);

    m_cbIsFlatRouting = new QCheckBox("is_flat");
    layout->addRow(new QLabel(tr("Flat routing:")), m_cbIsFlatRouting);
}
#endif

void NCriticalPathToolsWidget::runPnRView()
{
    m_process.setWorkingDirectory(projectLocation());
    qInfo() << "set working dir" << projectLocation();

    QString fullCmd = vprBaseCommand() + " --server --analysis --disp on"; // TODO: add --server key
    m_process.start(fullCmd);
}

void NCriticalPathToolsWidget::onConnectionStatusChanged(bool isConnected)
{
    if (isConnected && m_isFirstTimeConnectedToParticularPnRViewInstance) {
#ifndef BYPASS_AUTO_PATH_LIST_FETCH
        emit pathListRequested("socket connection resumed");
#endif
        m_isFirstTimeConnectedToParticularPnRViewInstance = false;
    }
}
