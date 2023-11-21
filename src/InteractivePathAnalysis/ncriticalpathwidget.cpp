#include "ncriticalpathwidget.h"

#include "ncriticalpathmodel.h"
#include "ncriticalpathfiltermodel.h"
#include "ncriticalpathview.h"
#include "ncriticalpathstatusbar.h"
#include "ncriticalpathtoolswidget.h"
#include "ncriticalpaththeme.h"
#include "simplelogger.h"

#include <QLabel>
#include <QPushButton>
#include <QHeaderView>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QFileInfo>
#ifdef ENABLE_OPEN_FILE_FEATURE
#include <QFileDialog>
#endif
#include <QDir>

NCriticalPathWidget::NCriticalPathWidget(
#ifndef STANDALONE_APP
    FOEDAG::Compiler* compiler,
#endif
    QWidget* parent)
    : QWidget(parent)
    , m_model(new NCriticalPathModel(this))
    , m_filterModel(new NCriticalPathFilterModel(this))
    , m_view(new NCriticalPathView(this))
    , m_toolsWidget(new NCriticalPathToolsWidget(
#ifndef STANDALONE_APP
        compiler,
#endif
          this))
    , m_statusBar(new NCriticalPathStatusBar(this))
    , m_client(m_toolsWidget->parameters())
{
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

#ifdef ENABLE_OPEN_FILE_FEATURE
    QPushButton* bnOpenFile = new QPushButton("Open *.rpt file");
    toolBarLayout->addWidget(bnOpenFile);
    QObject::connect(bnOpenFile, &QPushButton::clicked, this, [this](){
        QString selectedFile = QFileDialog::getOpenFileName(nullptr, "Open *.rpt file", QDir::homePath(), "*.rpt");
        openFile(selectedFile);
    });
#endif

#ifdef ENABLE_MULTISELECTION_MODE
    QPushButton* bnClrSelection = new QPushButton("clear selection");
    toolBarLayout->addWidget(bnClrSelection);
    QObject::connect(bnClrSelection, &QPushButton::clicked, m_view, &NCriticalPathView::clearSelection);
#endif

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
        m_toolsWidget->onPathListReceived();
        m_view->onDataLoaded();
        m_statusBar->setMessage(tr("Got path list"));
    });

    // view connections
    connect(m_view, &NCriticalPathView::pathSelectionChanged, &m_client, &Client::requestPathHighLight);
    connect(m_view, &NCriticalPathView::criteriaFilterChanged, this, [this](const FilterCriteriaConf& inputCriteriaConf, const FilterCriteriaConf& outputCriteriaConf){
        m_filterModel->setFilterCriteria(inputCriteriaConf, outputCriteriaConf);
    });

    // toolswidget connections
    connect(m_toolsWidget, &NCriticalPathToolsWidget::PnRViewRunStatusChanged, this, [this](bool isRunning){
        if (!isRunning) {
            m_model->clear();
            m_view->onDataCleared();
            m_statusBar->setMessage(tr("P&R View is not running"));
            m_client.stopConnectionWatcher();
        } else {
            m_statusBar->setMessage(tr("P&R View is starting..."));
            m_client.startConnectionWatcher();
        }
    });
    connect(m_toolsWidget, &NCriticalPathToolsWidget::pathListRequested, this, [this](const QString& initiator){
        if (m_client.isConnected()) {
            m_client.requestPathList(initiator);
            m_statusBar->setMessage(tr("Getting path list..."));
        }
    });
    connect(m_toolsWidget, &NCriticalPathToolsWidget::highLightModeChanged, &m_client, &Client::onHightLightModeChanged);

    // client connections
    connect(&m_client, &Client::pathListDataReceived, m_model, &NCriticalPathModel::loadFromString);
    connect(&m_client, &Client::highLightModeReceived, m_toolsWidget, &NCriticalPathToolsWidget::onHightLightModeReceived);
    connect(&m_client, &Client::connectedChanged, this, [this](bool isConnected){
        m_toolsWidget->onConnectionStatusChanged(isConnected);
        m_statusBar->onConnectionStatusChanged(isConnected);
    });
}

NCriticalPathWidget::~NCriticalPathWidget()
{
    SimpleLogger::instance().log("~NCriticalPathWidget");
}

#ifdef ENABLE_OPEN_FILE_FEATURE
void NCriticalPathWidget::openFile(const QString& filePath)
{
    m_model->loadFromFile(filePath);
    setWindowTitle(QFileInfo(filePath).fileName());
}
#endif
