#include "ncriticalpathtoolswidget.h"
#include "ncriticalpathparameters.h"
#include "ncriticalpaththeme.h"
#include "custommenu.h"
#include "simplelogger.h"
#include "client/keys.h"

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

#ifdef STANDALONE_APP
#include <QSettings>
#else
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
    , m_vprProcess("vpr")
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
    connect(m_bnRunPnRView, &QPushButton::clicked, this, &NCriticalPathToolsWidget::tryRunPnRView);
    connect(&m_vprProcess, &Process::runStatusChanged, this, [this](bool isRunning){
        m_bnRunPnRView->setEnabled(!isRunning && !m_parameters->getIsFlatRouting());
        emit PnRViewRunStatusChanged(isRunning);
    });

#ifdef STANDALONE_APP
    setWindowTitle("Client");

    QPushButton* bnFOEDAGProj = new QPushButton("FOEDAG proj...");
    layout->addWidget(bnFOEDAGProj);
    setupProjectMenu(bnFOEDAGProj);
#endif

    onConnectionStatusChanged(false);
}

void NCriticalPathToolsWidget::deactivatePlaceAndRouteViewProcess()
{
    m_bnRunPnRView->setEnabled(false);
    m_vprProcess.stop();
}

void NCriticalPathToolsWidget::onPathListReceived()
{
    //m_parameters->resetIsPathListConfigurationChangedFlag();
}

void NCriticalPathToolsWidget::onHightLightModeReceived()
{
    //m_parameters->resetIsHightLightModeChangedFlag();
}

QString NCriticalPathToolsWidget::projectLocation()
{
#ifdef STANDALONE_APP
    return m_leProjectLocation->text();
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
        QString vprFileFilePath = m_leVprFilePath->text();
        if (!vprFileFilePath.endsWith("/vpr")) {
            vprFileFilePath.append("/vpr");
        }
        QString hwXmlFilePath = m_leHardwareXmlFilePath->text();
        if (!deviceLayout.isEmpty()) {
            QList<QString> cmd;
            cmd << vprFileFilePath;
            cmd << hwXmlFilePath;
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
        SimpleLogger::instance().error("cannot run P&RView due to empty projPath");
    }
    return "";
#else
    return static_cast<FOEDAG::CompilerOpenFPGA_ql*>(m_compiler)->BaseVprCommand().c_str();
#endif
}

void NCriticalPathToolsWidget::restoreConfiguration()
{
    m_parameters->loadFromFile();
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
    });
    connect(m_pathsOptionsMenu, &CustomMenu::accepted, this, [this](){
        if (m_parameters->isFlatRoutingChanged()) {
            if (m_parameters->getIsFlatRouting()) {
                emit isFlatRoutingOnDetected();
            } else {
                if (!m_vprProcess.isRunning()) {
                    tryRunPnRView();
                }
            }
        } else {
            if (m_parameters->isPathListConfigChanged()) {
                emit pathListRequested("autorefresh because path list configuration changed");
            }
            if (m_parameters->isHightLightModeChanged()) {
                emit highLightModeChanged();
            }
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

    connect(m_cbHighlightMode, &QComboBox::currentTextChanged, this, [this](const QString& item) {
        m_parameters->setHighLightMode(item.toStdString());
    });

    formLayout->addRow(new QLabel(tr("Hight light mode:")), m_cbHighlightMode);

    //
    m_cbPathType = new QComboBox;
    m_cbPathType->addItem(KEY_SETUP_PATH_LIST);
    m_cbPathType->addItem(KEY_HOLD_PATH_LIST);
    //    m_cbPathType->addItem("skew");
    connect(m_cbPathType, &QComboBox::currentTextChanged, this, [this](const QString& newText) {
        m_parameters->setPathType(newText.toStdString());
    });
    formLayout->addRow(new QLabel(tr("Path type:")), m_cbPathType);

    //
    m_cbDetail = new QComboBox;
    m_cbDetail->addItem("netlist");
    m_cbDetail->addItem("aggregated");
    m_cbDetail->addItem("detailed routing");
    m_cbDetail->addItem("debug");
    connect(m_cbDetail, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        m_parameters->setPathDetailLevel(text.toStdString());
    });
    formLayout->addRow(new QLabel(tr("Timing report detail:")), m_cbDetail);

    //
    m_leNCriticalPathNum = new QLineEdit();
    QIntValidator intValidator(m_leNCriticalPathNum);
    m_leNCriticalPathNum->setValidator(&intValidator);

    connect(m_leNCriticalPathNum, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_parameters->setCriticalPathNum(text.toInt());
    });
    formLayout->addRow(new QLabel(tr("Timing report npaths:")), m_leNCriticalPathNum);

    m_cbIsFlatRouting = new QCheckBox("");
    formLayout->addRow(new QLabel(tr("Flat routing:")), m_cbIsFlatRouting);
    connect(m_cbIsFlatRouting, &QCheckBox::clicked, this, [this](bool checked) {
        m_parameters->setFlatRouting(checked);
    });

    m_cbSaveSettings = new QCheckBox("Save settings");
    m_cbSaveSettings->setChecked(m_parameters->getSavePathListSettings());
    connect(m_cbSaveSettings, &QCheckBox::toggled, this, [this](bool checked){
        m_parameters->saveOptionSavePathListSettingsExplicitly(checked);
    });
    formLayout->addRow(m_cbSaveSettings);

    resetConfigurationMenu();
}

void NCriticalPathToolsWidget::resetConfigurationMenu()
{
    m_cbHighlightMode->blockSignals(true);
    m_cbHighlightMode->setCurrentText(m_parameters->getHighLightMode().c_str());
    m_cbHighlightMode->blockSignals(false);

    m_cbPathType->blockSignals(true);
    m_cbPathType->setCurrentText(m_parameters->getPathType().c_str());
    m_cbPathType->blockSignals(false);

    m_cbDetail->blockSignals(true);
    m_cbDetail->setCurrentText(m_parameters->getPathDetailLevel().c_str());
    m_cbDetail->blockSignals(false);

    m_leNCriticalPathNum->blockSignals(true);
    m_leNCriticalPathNum->setText(QString::number(m_parameters->getCriticalPathNum()));
    m_leNCriticalPathNum->blockSignals(false);
}

void NCriticalPathToolsWidget::saveConfiguration()
{
    m_parameters->saveToFile();
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

    QSettings settings;

    // m_leProjectLocation
    m_leProjectLocation = new QLineEdit;
    m_leProjectLocation->setPlaceholderText("set valid project location here...");

    if (QVariant value = settings.value("projPath"); value.isValid()) {
        m_leProjectLocation->setText(value.toString());
    }
    connect(m_leProjectLocation, &QLineEdit::textChanged, this, [](const QString& text){
        QSettings settings;
        settings.setValue("projPath", text);
    });
    layout->addRow(new QLabel(tr("FOEDAG project location:")), m_leProjectLocation);

    // m_leVprFilePath
    m_leVprFilePath = new QLineEdit;
    m_leVprFilePath->setPlaceholderText("set valid vpr filepath here...");

    if (QVariant value = settings.value("vprPath"); value.isValid()) {
        m_leVprFilePath->setText(value.toString());
    }
    connect(m_leVprFilePath, &QLineEdit::textChanged, this, [](const QString& text){
        QSettings settings;
        settings.setValue("vprPath", text);
    });
    layout->addRow(new QLabel(tr("VPR executable path:")), m_leVprFilePath);

    // m_leHardwareXmlFilePath
    m_leHardwareXmlFilePath = new QLineEdit;
    m_leHardwareXmlFilePath->setPlaceholderText("set valid hardware xml filepath here...");

    if (QVariant value = settings.value("hwXmlPath"); value.isValid()) {
        m_leHardwareXmlFilePath->setText(value.toString());
    }
    connect(m_leHardwareXmlFilePath, &QLineEdit::textChanged, this, [](const QString& text){
        QSettings settings;
        settings.setValue("hwXmlPath", text);
    });
    layout->addRow(new QLabel(tr("HW XML path:")), m_leHardwareXmlFilePath);
}
#endif

void NCriticalPathToolsWidget::tryRunPnRView()
{
    if (m_vprProcess.isRunning()) {
        SimpleLogger::instance().log("skip P&R View process run, because it's already run");
        return;
    }

    if (m_parameters->getIsFlatRouting()) {
        SimpleLogger::instance().log("skip P&R View process run, because vpr set using flat routing");
        emit isFlatRoutingOnDetected();
    } else {
        m_vprProcess.setWorkingDirectory(projectLocation());
        SimpleLogger::instance().log("set working dir", projectLocation());
        QString fullCmd = vprBaseCommand() + " --server --analysis --disp on";
        m_vprProcess.start(fullCmd);
    }
}

void NCriticalPathToolsWidget::onConnectionStatusChanged(bool isConnected)
{
    if (isConnected) {
#ifndef BYPASS_AUTO_PATH_LIST_FETCH
        emit pathListRequested("socket connection resumed");
#endif
    }
}
