#include "NCriticalPathToolsWidget.h"
#include "NCriticalPathParameters.h"
#include "NCriticalPathTheme.h"
#include "CustomMenu.h"
#include "SimpleLogger.h"
#include "client/CommConstants.h"
#include "client/ServerFreePortDetector.h"

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

#include "../NewProject/ProjectManager/project_manager.h"
#include "../Compiler/CompilerOpenFPGA_ql.h"
#include "../Compiler/QLSettingsManager.h"

NCriticalPathToolsWidget::NCriticalPathToolsWidget(
        FOEDAG::Compiler* compiler, QWidget* parent)
    : QWidget(parent)
    , m_compiler(compiler)
    , m_vprProcess("vpr")
    , m_parameters(std::make_shared<NCriticalPathParameters>())
{
    SimpleLogger::instance().setEnabled(m_parameters->getIsLogToFileEnabled());

    m_vprProcess.addInnerErrorToBypass("gtk_label_set_text: assertion 'GTK_IS_LABEL (label)' failed");
    m_vprProcess.addInnerErrorToBypass("Couldn't register with accessibility bus: An AppArmor policy prevents this sender from sending this message to this recipient; type=\"method_call\", sender=\"(null)\" (inactive) interface=\"org.freedesktop.DBus\" member=\"Hello\" error name=\"(unset)\" requested_reply=\"0\" destination=\"org.freedesktop.DBus\" (bus)");

    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(NCriticalPathTheme::instance().borderSize());
    setLayout(layout);

    QPushButton* bnPathsOptions = new QPushButton("Configuration");
    bnPathsOptions->setToolTip(tr("grouped project settings relevant for the Interactive Path Analysis Tool"));
    layout->addWidget(bnPathsOptions);
    setupCriticalPathsOptionsMenu(bnPathsOptions);

    // bnRunPnRView
    m_bnRunPnRView = new QPushButton("Run P&&R View");
    m_bnRunPnRView->setToolTip(tr("run P&R View manually if it was terminated or closed"));
    layout->addWidget(m_bnRunPnRView);
    connect(m_bnRunPnRView, &QPushButton::clicked, this, &NCriticalPathToolsWidget::tryRunPnRView);
    connect(&m_vprProcess, &Process::runStatusChanged, this, [this](bool isRunning){
        m_bnRunPnRView->setEnabled(!isRunning && !m_parameters->getIsFlatRouting());
        emit PnRViewRunStatusChanged(isRunning);
    });
    connect(&m_vprProcess, &Process::innerErrorOccurred, this, &NCriticalPathToolsWidget::vprProcessErrorOccured);

    onConnectionStatusChanged(false);
}

void NCriticalPathToolsWidget::deactivatePlaceAndRouteViewProcess()
{
    m_bnRunPnRView->setEnabled(false);
    m_vprProcess.stop();
}

QString NCriticalPathToolsWidget::projectLocation()
{
    return m_compiler->ProjManager()->getProjectPath();
}

QString NCriticalPathToolsWidget::vprBaseCommand()
{
    return static_cast<FOEDAG::CompilerOpenFPGA_ql*>(m_compiler)->BaseVprCommand().c_str();
}

void NCriticalPathToolsWidget::refreshCritPathContextOnSettingsChanged()
{
    if (m_parameters->isLogToFileChanged()) {
        SimpleLogger::instance().setEnabled(m_parameters->getIsLogToFileEnabled());
    }
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
        if (m_parameters->isHightLightModeChanged() || m_parameters->isDrawCriticalPathContourChanged()) {
            emit highLightModeChanged();
        }
    }
    m_parameters->resetChangedFlags();
}

void NCriticalPathToolsWidget::setupCriticalPathsOptionsMenu(QPushButton* caller)
{
    if (m_pathsOptionsMenu) {
        return;
    }

    m_pathsOptionsMenu = new CustomMenu(caller);
    m_pathsOptionsMenu->setButtonToolTips(
        tr("apply interactive path analysis configuration and save it to main project settings file"), 
        tr("discard settings")
    );
    connect(m_pathsOptionsMenu, &CustomMenu::declined, this, [this](){
        resetConfigurationUI();
    });
    connect(m_pathsOptionsMenu, &CustomMenu::accepted, this, [this](){
        FOEDAG::QLSettingsManager::reloadJSONSettings(); // to refresh project settings

        m_parameters->resetChangedFlags();

        m_parameters->setHighLightMode(m_cbHighlightMode->currentText().toStdString());
        m_parameters->setPathType(m_cbPathType->currentText().toStdString());
        m_parameters->setPathDetailLevel(m_cbDetail->currentText().toStdString());
        m_parameters->setCriticalPathNum(m_leNCriticalPathNum->text().toInt());
        m_parameters->setIsFlatRouting(m_cbIsFlatRouting->isChecked());
        m_parameters->setIsLogToFileEnabled(m_cbIsLogToFileEnabled->isChecked());
        m_parameters->setIsDrawCriticalPathContourEnabled(m_cbDrawCritPathContour->isChecked());

        if (m_parameters->hasChanges()) {
            if (bool foundChanges = m_parameters->saveToFile()) {
                FOEDAG::QLSettingsManager::reloadJSONSettings(); // to refresh project settings
                refreshCritPathContextOnSettingsChanged();
            }
        }
    });

    QFormLayout* formLayout = new QFormLayout;
    m_pathsOptionsMenu->addContentLayout(formLayout);

    //
    m_cbHighlightMode = new QComboBox;
    m_cbHighlightMode->setToolTip(m_parameters->getHighLightModeToolTip().c_str());
    for (const std::string& item: m_parameters->getHighLightAvailableOptions()) {
        m_cbHighlightMode->addItem(item.c_str());
    }

    addRowToFormLayout(formLayout, tr("Hight light mode:"), m_cbHighlightMode);

    //
    m_cbDrawCritPathContour = new QCheckBox;
    m_cbDrawCritPathContour->setToolTip(m_parameters->getIsDrawCriticalPathContourEnabledToolTip().c_str());
    addRowToFormLayout(formLayout, tr("Draw path contour:"), m_cbDrawCritPathContour);

    //
    m_cbPathType = new QComboBox;
    m_cbPathType->setToolTip(m_parameters->getPathTypeToolTip().c_str());
    for (const std::string& item: m_parameters->getCritPathTypeAvailableOptions()) {
        m_cbPathType->addItem(item.c_str());
    }
    addRowToFormLayout(formLayout, tr("Path type:"), m_cbPathType);

    //
    m_cbDetail = new QComboBox;
    m_cbDetail->setToolTip(m_parameters->getPathDetailLevelToolTip().c_str());
    for (const std::string& item: m_parameters->getPathDetailAvailableOptions()) {
        m_cbDetail->addItem(item.c_str());
    }
    addRowToFormLayout(formLayout, tr("Timing report detail:"), m_cbDetail);

    //
    m_leNCriticalPathNum = new QLineEdit();
    m_leNCriticalPathNum->setToolTip(m_parameters->getCriticalPathNumToolTip().c_str());
    QIntValidator intValidator(m_leNCriticalPathNum);
    m_leNCriticalPathNum->setValidator(&intValidator);

    addRowToFormLayout(formLayout, tr("Timing report npaths:"), m_leNCriticalPathNum);

    m_cbIsFlatRouting = new QCheckBox("");
    m_cbIsFlatRouting->setMinimumHeight(25);
    m_cbIsFlatRouting->setToolTip(m_parameters->getIsFlatRoutingToolTip().c_str());
    addRowToFormLayout(formLayout, tr("Flat routing:"), m_cbIsFlatRouting);

    m_cbIsLogToFileEnabled = new QCheckBox("");
    m_cbIsLogToFileEnabled->setMinimumHeight(25);
    m_cbIsLogToFileEnabled->setToolTip(m_parameters->getIsLogToFileEnabledToolTip().c_str());
    addRowToFormLayout(formLayout, tr("Enable file log:"), m_cbIsLogToFileEnabled);

    resetConfigurationUI();
}

void NCriticalPathToolsWidget::resetConfigurationUI()
{
    m_parameters->loadFromFile();

    m_cbHighlightMode->setCurrentText(m_parameters->getHighLightMode().c_str());
    m_cbPathType->setCurrentText(m_parameters->getPathType().c_str());
    m_cbDetail->setCurrentText(m_parameters->getPathDetailLevel().c_str());
    m_leNCriticalPathNum->setText(QString::number(m_parameters->getCriticalPathNum()));
    m_cbIsFlatRouting->setChecked(m_parameters->getIsFlatRouting());
    m_cbIsLogToFileEnabled->setChecked(m_parameters->getIsLogToFileEnabled());
    m_cbDrawCritPathContour->setChecked(m_parameters->getIsDrawCriticalPathContourEnabled());
}

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

        int portNum = client::ServerFreePortDetector().detectAvailablePortNum();
        emit serverPortNumDetected(portNum);

        QString fullCmd = vprBaseCommand();

        QString rrGraphFileName{FOEDAG::QLSettingsManager::getStringValue("vpr", "filename", "write_rr_graph").c_str()};

        fullCmd = fullCmd.replace(QString(" --write_rr_graph %1").arg(rrGraphFileName), "");
        if (QFile::exists(projectLocation() + "/" + rrGraphFileName)) {
            fullCmd += QString(" --read_rr_graph %1").arg(rrGraphFileName);  
        }
#ifdef _WIN32
        // under WIN32, running the analysis stage alone causes issues, hence we call the
        // route and analysis stages together
        fullCmd += " --route";
#endif
        fullCmd += " --analysis";
        fullCmd += " --server";
        fullCmd += QString(" --port %1").arg(portNum);
        fullCmd += " --disp on";

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

void NCriticalPathToolsWidget::addRowToFormLayout(QFormLayout* formLayout, const QString& labelText, QWidget* widget) const
{
    QLabel* label = new QLabel(labelText);
    label->setToolTip(widget->toolTip());
    formLayout->addRow(label, widget);
}