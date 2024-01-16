#include "NCriticalPathWidget.h"

#include "NCriticalPathModel.h"
#include "NCriticalPathFilterModel.h"
#include "NCriticalPathView.h"
#include "NCriticalPathStatusBar.h"
#include "NCriticalPathToolsWidget.h"
#include "NCriticalPathTheme.h"
#include "SimpleLogger.h"

#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QFileInfo>

#include "../Compiler/QLSettingsManager.h"

#include <QDir>

NCriticalPathWidget::NCriticalPathWidget(FOEDAG::Compiler* compiler, QWidget* parent)
    : QWidget(parent)
    , m_model(new NCriticalPathModel(this))
    , m_filterModel(new NCriticalPathFilterModel(this))
    , m_view(new NCriticalPathView(this))
    , m_toolsWidget(new NCriticalPathToolsWidget(compiler, this))
    , m_statusBar(new NCriticalPathStatusBar(this))
    , m_gateIO(m_toolsWidget->parameters())
{
    m_prevIsFlatRoutingFlag = m_toolsWidget->parameters()->getIsFlatRouting(); // int prev value

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);

    /// models setup
    m_filterModel->setSourceModel(m_model);

    /// viewer setup
    m_view->setModel(m_filterModel);
    m_view->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_view->header()->setHidden(true);
    ///

    /// toolbox setup
    QHBoxLayout* toolBarLayout = new QHBoxLayout;
    int borderSize = NCriticalPathTheme::instance().borderSize();
    toolBarLayout->setContentsMargins(0,borderSize,borderSize,borderSize);

    toolBarLayout->addWidget(m_toolsWidget);
    toolBarLayout->addStretch();
    ///

    ///
    layout->addLayout(toolBarLayout);
    layout->addWidget(m_view);
    layout->addWidget(m_statusBar);


    // model connections
    connect(m_model, &NCriticalPathModel::loadFinished, this, [this](){
        m_view->fillInputOutputData(m_model->inputNodes(), m_model->outputNodes());
        m_view->onDataLoaded();
        m_statusBar->setMessage(tr("Got path list"));
    });
    connect(m_model, &NCriticalPathModel::cleared, this, [this](){
        m_filterModel->clear();
        m_view->onDataCleared();
    });

    // view connections
    connect(m_view, &NCriticalPathView::pathElementSelectionChanged, &m_gateIO, &client::GateIO::requestPathItemsHighLight);
    connect(m_view, &NCriticalPathView::criteriaFilterChanged, this, [this](const FilterCriteriaConf& inputCriteriaConf, const FilterCriteriaConf& outputCriteriaConf){
        m_filterModel->setFilterCriteria(inputCriteriaConf, outputCriteriaConf);
    });

    // toolswidget connections
    connect(m_toolsWidget, &NCriticalPathToolsWidget::PnRViewRunStatusChanged, this, [this](bool isRunning){
        if (!isRunning) {
            m_model->clear();
            m_statusBar->setMessage(tr("P&R View is not running"));
        } else {
            m_statusBar->setMessage(tr("P&R View is starting..."));
        }
        m_gateIO.setServerIsRunning(isRunning);
    });
    connect(m_toolsWidget, &NCriticalPathToolsWidget::pathListRequested, this, &NCriticalPathWidget::requestPathList);
    connect(m_toolsWidget, &NCriticalPathToolsWidget::highLightModeChanged, &m_gateIO, &client::GateIO::onHightLightModeChanged);
    connect(m_toolsWidget, &NCriticalPathToolsWidget::vprProcessErrorOccured, this, [this](QString errorMsg){
        notifyError("VPR", errorMsg);
    });
    connect(m_toolsWidget, &NCriticalPathToolsWidget::serverPortNumDetected, &m_gateIO, &client::GateIO::onServerPortDetected);

    // client connections
    connect(&m_gateIO, &client::GateIO::pathListDataReceived, m_model, &NCriticalPathModel::loadFromString);
    connect(&m_gateIO, &client::GateIO::connectedChanged, this, [this](bool isConnected){
        m_toolsWidget->onConnectionStatusChanged(isConnected);
        m_statusBar->onConnectionStatusChanged(isConnected);
    });

    connect(m_toolsWidget, &NCriticalPathToolsWidget::isFlatRoutingOnDetected, this, &NCriticalPathWidget::onFlatRoutingOnDetected);
    connect(FOEDAG::QLSettingsManager::getInstance(), &FOEDAG::QLSettingsManager::settingsChanged, this, [this](){
        const auto& parameters = m_toolsWidget->parameters();
        parameters->resetChangedFlags();
        if (bool foundChanges = parameters->loadFromFile()) {
            m_toolsWidget->refreshCritPathContextOnSettingsChanged();
            m_toolsWidget->resetConfigurationUI();
        }
    });

    m_toolsWidget->tryRunPnRView(); // startup run
}

NCriticalPathWidget::~NCriticalPathWidget()
{}

void NCriticalPathWidget::onFlatRoutingOnDetected()
{
    m_toolsWidget->deactivatePlaceAndRouteViewProcess();
    QString msg = tr("Place&Route View is disabled since flat_routing is enabled in VPR!");
    m_statusBar->setMessage(msg);
    notifyError("VPR", msg);
}

void NCriticalPathWidget::requestPathList(const QString& initiator)
{
    if (m_gateIO.isConnected()) {
        m_gateIO.requestPathList(initiator);
        m_statusBar->setMessage(tr("Getting path list..."));
    } else {
        SimpleLogger::instance().error("cannot requestPathList by", initiator, "because client is not connected");
    }
}

void NCriticalPathWidget::notifyError(QString title, QString errorMsg)
{
    QMessageBox::warning(this, title, errorMsg, QMessageBox::Ok);
}