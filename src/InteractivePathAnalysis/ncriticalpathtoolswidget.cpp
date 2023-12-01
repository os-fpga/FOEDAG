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
    m_parameters->resetChangedFlags();
}

void NCriticalPathToolsWidget::setupCriticalPathsOptionsMenu(QPushButton* caller)
{
    if (m_pathsOptionsMenu) {
        return;
    }

    m_pathsOptionsMenu = new CustomMenu(caller);
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
        m_parameters->setFlatRouting(m_cbIsFlatRouting->isChecked());

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
    for (const std::string& item: m_parameters->getHighLightAvailableOptions()) {
        m_cbHighlightMode->addItem(item.c_str());
    }

    formLayout->addRow(new QLabel(tr("Hight light mode:")), m_cbHighlightMode);

    //
    m_cbPathType = new QComboBox;
    for (const std::string& item: m_parameters->getCritPathTypeAvailableOptions()) {
        m_cbPathType->addItem(item.c_str());
    }
    formLayout->addRow(new QLabel(tr("Path type:")), m_cbPathType);

    //
    m_cbDetail = new QComboBox;
    for (const std::string& item: m_parameters->getPathDetailAvailableOptions()) {
        m_cbDetail->addItem(item.c_str());
    }
    formLayout->addRow(new QLabel(tr("Timing report detail:")), m_cbDetail);

    //
    m_leNCriticalPathNum = new QLineEdit();
    QIntValidator intValidator(m_leNCriticalPathNum);
    m_leNCriticalPathNum->setValidator(&intValidator);

    formLayout->addRow(new QLabel(tr("Timing report npaths:")), m_leNCriticalPathNum);

    m_cbIsFlatRouting = new QCheckBox("");
    formLayout->addRow(new QLabel(tr("Flat routing:")), m_cbIsFlatRouting);

    resetConfigurationUI();
}

void NCriticalPathToolsWidget::resetConfigurationUI()
{
    m_parameters->loadFromFile();

    m_cbHighlightMode->blockSignals(true);
    m_cbHighlightMode->setCurrentText(m_parameters->getHighLightMode().c_str());
    m_cbHighlightMode->blockSignals(false);

    m_cbPathType->setCurrentText(m_parameters->getPathType().c_str());
    m_cbDetail->setCurrentText(m_parameters->getPathDetailLevel().c_str());
    m_leNCriticalPathNum->setText(QString::number(m_parameters->getCriticalPathNum()));
    m_cbIsFlatRouting->setChecked(m_parameters->getIsFlatRouting());
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
