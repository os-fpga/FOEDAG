#include "ncriticalpathwidget.h"

#include "ncriticalpathmodel.h"
#include "ncriticalpathview.h"
#include "ncriticalpathfilterwidget.h"
#include "ncriticalpathstatusbar.h"

#include "client/clienttoolswidget.h"

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
#include <QDebug>

NCriticalPathWidget::NCriticalPathWidget(
#ifndef STANDALONE_APP
    FOEDAG::Compiler* compiler,
#endif
    QWidget* parent)
    : QWidget(parent)
    , m_model(new NCriticalPathModel(this))
    , m_view(new NCriticalPathView(this))
    , m_client(
#ifndef STANDALONE_APP
        compiler
#endif
          )
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);

    /// viewer setup
    m_view->setModel(m_model);

    m_view->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    m_view->header()->setHidden(true);
    ///

    /// toolbox setup
    QHBoxLayout* toolBarLayout = new QHBoxLayout;
    toolBarLayout->setContentsMargins(0,5,5,5);

#ifdef ENABLE_OPEN_FILE_FEATURE
    QPushButton* bnOpenFile = new QPushButton("Open *.rpt file");
    toolBarLayout->addWidget(bnOpenFile);
    QObject::connect(bnOpenFile, &QPushButton::clicked, [this](){
        QString selectedFile = QFileDialog::getOpenFileName(nullptr, "Open *.rpt file", QDir::homePath(), "*.rpt");
        openFile(selectedFile);
    });
#endif

#ifdef ENABLE_MULTISELECTION_MODE
    QPushButton* bnClrSelection = new QPushButton("clear selection");
    toolBarLayout->addWidget(bnClrSelection);
    QObject::connect(bnClrSelection, &QPushButton::clicked, m_view, &NCriticalPathView::clearSelection);
#endif

    toolBarLayout->addWidget(m_client.toolsWidget());
    toolBarLayout->addStretch();
    ///

    ///
    m_statusBar = new NCriticalPathStatusBar;

    layout->addLayout(toolBarLayout);
    layout->addWidget(m_view);
    layout->addWidget(m_statusBar);

    connect(&m_client, SIGNAL(critPathsDataReady(QString)), m_model, SLOT(load(QString)));
#ifdef ENABLE_SELECTION_RESTORATION
    connect(m_model, SIGNAL(loadFinished()), m_view, SLOT(refreshSelection()));
#endif
    connect(m_model, &NCriticalPathModel::loadFinished, [this](std::map<QString, int> inputs, std::map<QString, int> outputs){
        m_view->fillInputOutputData(inputs, outputs);
        m_client.toolsWidget()->onGotPathList();
        m_statusBar->setMessage(tr("Got path list"));
        m_view->onDataLoaded();
    });
    connect(m_view, &NCriticalPathView::pathSelected, &m_client, &Client::onPathSelected);
    connect(m_client.toolsWidget(), &ClientToolsWidget::PnRViewProcessRunningStatus, this, [this](bool isRunning){
        if (!isRunning) {
            m_model->clear();
            m_view->onDataCleared();
            m_statusBar->setMessage(tr("P&R View is not running"));
        } else {
            m_statusBar->setMessage(tr("P&R View is starting..."));
        }
    });
    connect(m_client.toolsWidget(), &ClientToolsWidget::connectionStatusChanged, m_statusBar, &NCriticalPathStatusBar::onConnectionStatusChanged);
    connect(m_client.toolsWidget(), &ClientToolsWidget::getPathListRequested, [this](){
        if (m_client.isConnected()) {
            m_statusBar->setMessage(tr("Getting path list..."));
        }
    });
}

NCriticalPathWidget::~NCriticalPathWidget()
{
    qDebug() << "~NCriticalPathWidget";
}

#ifdef ENABLE_OPEN_FILE_FEATURE
void NCriticalPathWidget::openFile(const QString& filePath)
{
    m_model->loadFromFile(filePath);
    setWindowTitle(QFileInfo(filePath).fileName());
}
#endif
