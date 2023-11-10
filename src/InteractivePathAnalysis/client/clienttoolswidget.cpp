#include "clienttoolswidget.h"
#include "../custommenu.h"
#include "../pushbutton.h"
#include "ncriticalpathsettings.h"

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
#include "../../NewProject/ProjectManager/project_manager.h"
#include "../../Compiler/CompilerOpenFPGA_ql.h"
#include "../../Compiler/QLSettingsManager.h"
#endif

#define PRINT_PROC_LOGS

ClientToolsWidget::ClientToolsWidget(
        #ifndef STANDALONE_APP
            FOEDAG::Compiler* compiler,
        #endif
        QWidget* parent)
: QWidget(parent)
#ifndef STANDALONE_APP
, m_compiler(compiler)
#endif
{
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    // bnRequestPathList
    m_bnRequestPathList = new PushButton("Get Path List");
    connect(m_bnRequestPathList, &QPushButton::clicked, [this](){
        emit getPathListRequested("button click event");
    });

    QPushButton* bnPathsOptions = new QPushButton("Paths Cfg...");
    layout->addWidget(bnPathsOptions);
    setupPathsOptionsMenu(bnPathsOptions);

    // insert bnRequestPathList
    layout->addWidget(m_bnRequestPathList);

    // bnRunPnRView
    m_bnRunPnRView = new QPushButton("Run P&&R View");
    layout->addWidget(m_bnRunPnRView);
    connect(m_bnRunPnRView, &QPushButton::clicked, this, &ClientToolsWidget::runPnRView);
    connect(&m_process, &Process::runningChanged, [this](bool isRunning){
        m_bnRunPnRView->setEnabled(!isRunning);
        m_isFirstTimeConnectedToParticularPnRViewInstance = true; // to get new path list on next PnRView run
        m_bnRequestPathList->markDirty();
        m_bnRequestPathList->setEnabled(isRunning);
        emit PnRViewProcessRunningStatus(isRunning);
    });

#ifdef STANDALONE_APP
    setWindowTitle("Client");

    QPushButton* bnFOEDAGProj = new QPushButton("FOEDAG proj...");
    layout->addWidget(bnFOEDAGProj);
    setupProjectMenu(bnFOEDAGProj);
#endif

#ifdef PRINT_PROC_LOGS
    connect(&m_process, &QProcess::readyReadStandardOutput, [this](){
        QByteArray output = m_process.readAllStandardOutput();
        QList<QByteArray> d = output.split('\n');
        for (const auto& e: d) {
            qDebug() << "proc:" << e;
        }
    });
#endif

#ifndef STANDALONE_APP
    show();
#endif

#ifndef BYPASS_AUTO_VPR_VIEW_RUN
    runPnRView(); 
#endif

    onConnectionStatusChanged(false);
}

void ClientToolsWidget::onGotPathList()
{
    m_bnRequestPathList->clearDirty();
}

QString ClientToolsWidget::projectLocation()
{
#ifdef STANDALONE_APP
    return m_leProj->text();
#else
    return m_compiler->ProjManager()->getProjectPath();
#endif
}

QString ClientToolsWidget::vprBaseCommand()
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

void ClientToolsWidget::setupPathsOptionsMenu(QPushButton* caller)
{
    assert(m_bnRequestPathList);
    if (m_pathsOptionsMenu) {
        return;
    }

    m_bnAutoRefreshPathList = new QCheckBox("Auto refresh path list");

    m_pathsOptionsMenu = new CustomMenu(caller);
    connect(m_pathsOptionsMenu, &CustomMenu::hidden, [this](){
        if (m_bnRequestPathList->isDirty() && m_bnAutoRefreshPathList->isChecked()) {
            m_bnRequestPathList->click();
        }
    });

    QVBoxLayout* mainLayout = new QVBoxLayout;
    m_pathsOptionsMenu->setLayout(mainLayout);

    QFormLayout* formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);

    //
    m_cbHighlightMode = new QComboBox;
    m_cbHighlightMode->addItem("Flylines");
    m_cbHighlightMode->addItem("Flylines Delays");
    m_cbHighlightMode->addItem("Routing");
    m_cbHighlightMode->addItem("Routing Delays");

    m_cbHighlightMode->setCurrentText(NCriticalPathSettings::instance().getHighLightMode());
    connect(m_cbHighlightMode, &QComboBox::currentTextChanged, this, &ClientToolsWidget::highLightModeChanged);

    formLayout->addRow(new QLabel(tr("Hightlight mode:")), m_cbHighlightMode);

    //
    m_cbPathType = new QComboBox;
    m_cbPathType->addItem("setup");
    m_cbPathType->addItem("hold");
    //    m_cbPathType->addItem("skew");
    m_cbPathType->setCurrentText(NCriticalPathSettings::instance().getPathType());
    connect(m_cbPathType, &QComboBox::currentTextChanged, m_bnRequestPathList, &PushButton::markDirty);
    formLayout->addRow(new QLabel(tr("Type:")), m_cbPathType);

    //
    m_cbDetailes = new QComboBox;
    m_cbDetailes->addItem("netlist");
    m_cbDetailes->addItem("aggregated");
    m_cbDetailes->addItem("detailed routing");
    m_cbDetailes->addItem("debug");
    m_cbDetailes->setCurrentText(NCriticalPathSettings::instance().getPathDetailLevel());
    connect(m_cbDetailes, &QComboBox::currentTextChanged, m_bnRequestPathList, &PushButton::markDirty);
    formLayout->addRow(new QLabel(tr("Report detail:")), m_cbDetailes);

    //
    m_leNCriticalPathNum = new QLineEdit();
    QIntValidator intValidator(m_leNCriticalPathNum);
    m_leNCriticalPathNum->setValidator(&intValidator);

    m_leNCriticalPathNum->setText(NCriticalPathSettings::instance().getCriticalPathNum());
    connect(m_leNCriticalPathNum, &QLineEdit::textChanged, m_bnRequestPathList, &PushButton::markDirty);
    formLayout->addRow(new QLabel(tr("Paths num limit:")), m_leNCriticalPathNum);

    m_bnAutoRefreshPathList->setChecked(NCriticalPathSettings::instance().getAutoRefreshPathList());
    formLayout->addRow(new QWidget, m_bnAutoRefreshPathList);

    QHBoxLayout* hLayout = new QHBoxLayout;
    mainLayout->addLayout(hLayout);

    QPushButton* bnSaveSettings = new QPushButton("Save Settings");
    hLayout->addStretch(1);
    hLayout->addWidget(bnSaveSettings);
    hLayout->addStretch(1);

    connect(bnSaveSettings, &QPushButton::clicked, this, &ClientToolsWidget::savePathsOptionsSettings);
}

void ClientToolsWidget::savePathsOptionsSettings()
{
    auto& settings = NCriticalPathSettings::instance();
    settings.setHighLightMode(m_cbHighlightMode->currentText());
    settings.setPathType(m_cbPathType->currentText());
    settings.setPathDetailLevel(m_cbDetailes->currentText());
    settings.setCriticalPathNum(m_leNCriticalPathNum->text());
    settings.setAutoRefreshPathList(m_bnAutoRefreshPathList->isChecked());
}

#ifdef STANDALONE_APP
void ClientToolsWidget::setupProjectMenu(QPushButton* caller)
{
    if (m_FOEDAGProjMenu) {
        return;
    }
    m_FOEDAGProjMenu = new CustomMenu(caller);

    QFormLayout* layout = new QFormLayout;
    m_FOEDAGProjMenu->setLayout(layout);

    //
    m_leProj = new QLineEdit;
    m_leProj->setPlaceholderText("set valid project path here...");

    QSettings settings;
    QVariant valProjPath = settings.value("projPath");
    if (valProjPath.isValid()) {
        m_leProj->setText(valProjPath.toString());
    }
    connect(m_leProj, &QLineEdit::textChanged, [](const QString& projPath){
        QSettings settings;
        settings.setValue("projPath", projPath);
    });
    layout->addRow(new QLabel(tr("FOEDAG project location:")), m_leProj);

    m_cbIsFlatRouting = new QCheckBox("is_flat");
    layout->addRow(new QLabel(tr("Flat routing:")), m_cbIsFlatRouting);
}
#endif

int ClientToolsWidget::nCriticalPathNum() const
{
    return m_leNCriticalPathNum->text().toInt();
}

int ClientToolsWidget::isFlatRouting() const
{
#ifdef STANDALONE_APP
    return m_cbIsFlatRouting->isChecked();
#else
    // reload QLSettingsManager() to ensure we account for dynamic changes in the settings/power json:
    FOEDAG::QLSettingsManager::reloadJSONSettings();

    // check if settings were loaded correctly before proceeding:
    if((FOEDAG::QLSettingsManager::getInstance()->settings_json).empty()) {
        qCritical() << "Project Settings JSON is missing, please check <project_name> and corresponding <project_name>.json exists: " << m_compiler->ProjManager()->projectName().c_str();
        return false;
    }

    if( FOEDAG::QLSettingsManager::getStringValue("vpr", "route", "flat_routing") == "checked" ) {
        return true;
    }
    return false;
#endif
}

QString ClientToolsWidget::pathType() const
{
    return m_cbPathType->currentText();
}

int ClientToolsWidget::detailesLevel() const
{
    return m_cbDetailes->currentIndex();
}

int ClientToolsWidget::highlightMode() const
{
    // "None" 0
    // "Crit Path Flylines" 1
    // "Crit Path Flylines Delays" 2
    // "Crit Path Routing" 3
    // "Crit Path Routing Delays" 4

    return m_cbHighlightMode->currentIndex() + 1; // +1 here is to shift item "None"
}

void ClientToolsWidget::runPnRView()
{
    m_process.setWorkingDirectory(projectLocation());
    qInfo() << "set working dir" << projectLocation();

    QString fullCmd = vprBaseCommand() + " --server --analysis --disp on"; // TODO: add --server key
    m_process.start(fullCmd);
}

void ClientToolsWidget::onConnectionStatusChanged(bool isConnected)
{
    emit connectionStatusChanged(isConnected);
    if (isConnected && m_isFirstTimeConnectedToParticularPnRViewInstance) {
#ifndef BYPASS_AUTO_PATH_LIST_FETCH
        emit getPathListRequested("socket connection resumed");
#endif
        m_isFirstTimeConnectedToParticularPnRViewInstance = false;
    }
}
