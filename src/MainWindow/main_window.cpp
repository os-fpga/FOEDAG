/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "main_window.h"

#include <QTextStream>
#include <QtWidgets>
#include <fstream>

#include "Compiler/Compiler.h"
#include "Compiler/CompilerDefines.h"
#include "Compiler/TaskManager.h"
#include "Console/DummyParser.h"
#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "Console/TclErrorParser.h"
#include "DesignRuns/runs_form.h"
#include "IpConfigurator/IpCatalogTree.h"
#include "IpConfigurator/IpConfigWidget.h"
#include "IpConfigurator/IpConfigurator.h"
#include "IpConfigurator/IpConfiguratorCreator.h"
#include "Main/CompilerNotifier.h"
#include "Main/Foedag.h"
#include "Main/ProjectFile/ProjectFileLoader.h"
#include "MainWindow/Session.h"
#include "MainWindow/WelcomePageWidget.h"
#include "NewFile/new_file.h"
#include "NewProject/Main/registerNewProjectCommands.h"
#include "NewProject/new_project_dialog.h"
#include "PinAssignment/PinAssignmentCreator.h"
#include "ProjNavigator/PropertyWidget.h"
#include "ProjNavigator/sources_form.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "TextEditor/text_editor.h"
#include "Utils/FileUtils.h"
#include "foedag_version.h"

using namespace FOEDAG;
extern const char* foedag_version_number;
extern const char* foedag_git_hash;
extern const char* foedag_build_type;

const QString MainWindow::WELCOME_PAGE_CONFIG_FILE = "WelcomePageConfig";
const QString RECENT_PROJECT_KEY{"recent/proj%1"};
constexpr uint RECENT_PROJECT_COUNT{10};
constexpr uint RECENT_PROJECT_COUNT_WP{5};

MainWindow::MainWindow(Session* session)
    : m_session(session), m_settings("settings", QSettings::IniFormat) {
  /* Window settings */
  m_compiler = session->GetCompiler();
  m_interpreter = session->TclInterp();

  QFile file(WELCOME_PAGE_CONFIG_FILE);
  m_showWelcomePage = !file.exists();  // Prevent welcome page from appearance
                                       // if the file exists.

  auto screenGeometry = qApp->primaryScreen()->availableGeometry();

  // Take 2/3 part of the screen.
  auto mainWindowSize =
      QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3);
  // Center main window on the screen. It will get this geometry after switching
  // from maximized mode.
  setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                  mainWindowSize, screenGeometry));
  // Initially, main window should be maximized.
  showMaximized();

  setDockNestingEnabled(true);

  setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  /* Create actions that can be added to menu/tool bars */
  createActions();

  /* Create menu bars */
  createMenus();

  /* Create progress bar */
  createProgressBar();

  /* Create tool bars */
  createToolBars();

  /* Create status bar */
  statusBar();

  /* Create actions for recent projects */
  createRecentMenu();

  //  /* Add dummy text editors */
  //  QTextEdit* editor1 = new QTextEdit;
  //  QTextEdit* editor2 = new QTextEdit;
  //  QTextEdit* editor3 = new QTextEdit;

  //  /* Add widgets into floorplanning */
  //  QSplitter* leftSplitter = new QSplitter(Qt::Horizontal);
  //  leftSplitter->addWidget(editor1);
  //  leftSplitter->setStretchFactor(1, 1);

  //  QDockWidget* texteditorDockWidget = new QDockWidget(tr("Text Editor"));
  //  texteditorDockWidget->setObjectName("texteditorDockWidget");
  //  texteditorDockWidget->setWidget(editor2);
  //  texteditorDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
  //                                        Qt::RightDockWidgetArea);
  //  addDockWidget(Qt::RightDockWidgetArea, texteditorDockWidget);

  //  QSplitter* mainSplitter = new QSplitter(Qt::Vertical);
  //  mainSplitter->addWidget(leftSplitter);
  //  mainSplitter->addWidget(editor3);
  //  mainSplitter->setStretchFactor(1, 1);

  //  setCentralWidget(mainSplitter);
  statusBar()->showMessage("Ready");
  m_projectInfo = {"FOEDAG", foedag_version_number, foedag_git_hash,
                   "https://github.com/os-fpga/FOEDAG/commit/",
                   foedag_build_type};

  connect(this, &MainWindow::projectOpened, this,
          &MainWindow::handleProjectOpened);
}

void MainWindow::Tcl_NewProject(int argc, const char* argv[]) {
  ProjectManager* projectManager = new ProjectManager(this);
  projectManager->Tcl_CreateProject(argc, argv);
}

void MainWindow::Info(const ProjectInfo& info) { m_projectInfo = info; }

ProjectInfo MainWindow::Info() const { return m_projectInfo; }

void MainWindow::SetWindowTitle(const QString& filename, const QString& project,
                                QString& projectInfo) {
  if (project.isEmpty())
    setWindowTitle(projectInfo);
  else if (filename.isEmpty()) {
    setWindowTitle(QString("%1 - %2").arg(project, projectInfo));
  } else {
    setWindowTitle(QString("%1 - %2 - %3").arg(filename, project, projectInfo));
  }
}

void MainWindow::CloseOpenedTabs() {
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  while (tabWidget->count() != 0) {
    tabWidget->tabCloseRequested(tabWidget->currentIndex());
  }
}

void MainWindow::ProgressVisible(bool visible) {
  m_progressVisible = visible;
  m_progressWidget->setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (confirmExitProgram()) {
    event->accept();
  } else {
    event->ignore();
  }
}

void MainWindow::newFile() {
  //  QTextStream out(stdout);
  //  out << "New file is requested\n";
  NewFile* newfile = new NewFile(this);
  newfile->StartNewFile();
}

void MainWindow::newProjectDlg() {
  newProjdialog->Reset();
  newProjdialog->open();
}

void MainWindow::openExampleProject() {
  auto currentDir = GlobalSession->Context()->DataPath();
  std::filesystem::path examplesPath = currentDir / "examples";
  auto fileName = QFileDialog::getOpenFileName(
      this, tr("Open Project"), QString::fromStdString(examplesPath.string()),
      "FOEDAG Project File(*.ospr)");

  if (!fileName.isEmpty()) {
    QFileInfo info(fileName);
    QString file = info.fileName();
    QString src = info.dir().path();
    QString sampleName = info.dir().dirName();
    QString dest = QDir::homePath() + QDir::separator() + sampleName;

    // Warn user if they've already copied this sample before
    if (QDir(dest).exists()) {
      auto result =
          QMessageBox::question(this, "Copying Example",
                                "Copying this example will overwrite " + dest +
                                    ". Do you want to continue?",
                                QMessageBox::Yes | QMessageBox::No);
      if (result == QMessageBox::Yes) {
        QDir(dest).removeRecursively();
      } else {
        return;
      }
    }

    // QT doesn't have a convenience function for recursively copying a folder
    // so we'll use std::filesystem instead
    std::filesystem::path srcPath = std::filesystem::path(src.toStdString());
    std::filesystem::path destPath = std::filesystem::path(dest.toStdString());
    std::filesystem::copy(srcPath, destPath,
                          std::filesystem::copy_options::recursive);

    // open the newly copied example project
    openProject(dest + QDir::separator() + file);
  }
}

void MainWindow::openProjectDialog(const QString& dir) {
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), dir,
                                          "FOEDAG Project File(*.ospr)");
  if (!fileName.isEmpty()) openProject(fileName);
}

void MainWindow::closeProject() {
  if (m_projectManager && m_projectManager->HasDesign() &&
      confirmCloseProject()) {
    Project::Instance()->InitProject();
    newProjdialog->Reset();
    CloseOpenedTabs();
    m_showWelcomePage ? showWelcomePage() : ReShowWindow({});
    newProjectAction->setEnabled(true);
  }
}

void MainWindow::openFileSlot() {
  const QString file = QFileDialog::getOpenFileName(this, tr("Open file"));
  auto editor = findChild<TextEditor*>("textEditor");
  if (editor) editor->SlotOpenFile(file);
}

void MainWindow::newDesignCreated(const QString& design) {
  const QFileInfo path(design);
  SetWindowTitle(QString(), path.baseName(), m_projectInfo.name);
  pinAssignmentAction->setEnabled(!design.isEmpty());
  pinAssignmentAction->setChecked(false);
  ipConfiguratorAction->setEnabled(!design.isEmpty());
  ipConfiguratorAction->setChecked(false);
  saveToRecentSettings(design);
  if (sourcesForm)
    sourcesForm->ProjectSettingsActions()->setEnabled(!design.isEmpty());
}

void MainWindow::startStopButtonsState() {
  const bool inProgress = m_taskManager->status() == TaskStatus::InProgress;
  const bool consoleInProgress = m_console->isRunning();
  startAction->setEnabled(!inProgress && !consoleInProgress);
  // Enable Stop action when there is something to stop
  stopAction->setEnabled(inProgress || consoleInProgress);
}

QDockWidget* MainWindow::PrepareTab(const QString& name, const QString& objName,
                                    QWidget* widget, QDockWidget* tabToAdd,
                                    Qt::DockWidgetArea area) {
  QDockWidget* dock = new QDockWidget(name, this);
  dock->setObjectName(objName);
  dock->setWidget(widget);
  addDockWidget(area, dock);
  if (tabToAdd != nullptr) {
    tabifyDockWidget(tabToAdd, dock);
  }
  return dock;
}

void MainWindow::cleanUpDockWidgets(std::vector<QDockWidget*>& dockWidgets) {
  for (const auto& dock : dockWidgets) {
    if (dock) {
      removeDockWidget(dock);
      delete dock->widget();
      delete dock;
    }
  }
  dockWidgets.clear();
}

void MainWindow::openProject(const QString& project) {
  ReShowWindow(project);
  loadFile(project);
  emit projectOpened();
}

void MainWindow::openProject(const std::string& projectFile) {
  openProject(QString::fromStdString(projectFile));
}

void MainWindow::saveToRecentSettings(const QString& project) {
  if (project.isEmpty()) return;

  std::list<QString> projects;

  for (uint i = 0; i < RECENT_PROJECT_COUNT; i++) {
    auto key = RECENT_PROJECT_KEY.arg(QString::number(i));
    auto pr = m_settings.value(key).toString();
    if (pr.isEmpty()) break;
    projects.push_back(pr);
  }

  // check if exists
  auto it = std::find_if(projects.cbegin(), projects.cend(),
                         [&project](const QString& p) { return p == project; });
  if (it != projects.cend()) projects.erase(it);

  projects.push_back(project);
  while (projects.size() > RECENT_PROJECT_COUNT) projects.pop_front();

  uint projCounter = 0;
  for (const auto& pr : projects) {
    m_settings.setValue(RECENT_PROJECT_KEY.arg(QString::number(projCounter++)),
                        pr);
  }
  createRecentMenu();
}

bool MainWindow::saveConstraintFile() {
  auto pinAssignment = findChild<PinAssignmentCreator*>();
  auto constrFiles = m_projectManager->getConstrFiles();
  if (constrFiles.empty()) {
    auto form = findChild<SourcesForm*>();
    form->CreateConstraint();
  }
  constrFiles = m_projectManager->getConstrFiles();
  if (constrFiles.empty()) {
    QMessageBox::warning(this, "No constraint file...",
                         "Please create constraint file.");
    return false;
  }
  bool rewrite = false;
  QFile file{constrFiles[0].c_str()};  // TODO @volodymyrk, need to fix issue
                                       // with target constraint
  QFile::OpenMode openFlags = QFile::ReadWrite;
  if (file.size() != 0) {
    auto btns = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
    auto msgBox = QMessageBox(
        QMessageBox::Question, tr("Save constraint file..."),
        tr("Do you want to rewrite current constraint file?"), btns, this);
    msgBox.button(QMessageBox::Yes)->setText("Rewrite");
    msgBox.button(QMessageBox::No)->setText("Append");
    msgBox.exec();
    auto answer = msgBox.buttonRole(msgBox.clickedButton());
    if (answer == QMessageBox::RejectRole) return false;
    rewrite = (answer == QMessageBox::YesRole);
    if (!rewrite) openFlags = QFile::ReadWrite | QIODevice::Append;
  }
  file.open(openFlags);
  if (rewrite) file.resize(0);  // clean content
  file.write(pinAssignment->generateSdc().toLatin1());
  file.close();
  return true;
}

void MainWindow::loadFile(const QString& file) {
  if (m_projectFileLoader) {
    m_projectFileLoader->Load(file);
    if (sourcesForm) sourcesForm->InitSourcesForm();
    updatePRViewButton(static_cast<int>(m_compiler->CompilerState()));
  }
}

void MainWindow::createProgressBar() {
  m_progressWidget = new QWidget;
  QProgressBar* progress = new QProgressBar(m_progressWidget);
  progress->setFixedHeight(menuBar()->sizeHint().height() - 2);
  progress->setFormat("%v/%m");
  QHBoxLayout* layout = new QHBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  QLabel* label = new QLabel{"Progress: "};
  layout->addWidget(label);
  layout->addWidget(progress);
  m_progressWidget->setLayout(layout);
  menuBar()->setCornerWidget(m_progressWidget, Qt::Corner::TopRightCorner);
  m_progressWidget->hide();
}

void MainWindow::createRecentMenu() {
  for (auto it{m_recentProjectsActions.begin()};
       it != m_recentProjectsActions.end(); ++it)
    delete it->first;
  m_recentProjectsActions.clear();

  for (int projCounter = RECENT_PROJECT_COUNT - 1; projCounter >= 0;
       projCounter--) {
    QString key = RECENT_PROJECT_KEY.arg(QString::number(projCounter));
    QString project = m_settings.value(key).toString();
    if (!project.isEmpty()) {
      QFileInfo info{project};
      auto projAction = new QAction(info.baseName());
      connect(projAction, &QAction::triggered, this,
              &MainWindow::recentProjectOpen);
      recentMenu->addAction(projAction);
      m_recentProjectsActions.push_back(std::make_pair(projAction, project));
    }
  }
}

void MainWindow::createMenus() {
  recentMenu = new QMenu("Recent Projects");
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openFile);
  fileMenu->addSeparator();
  fileMenu->addAction(newProjectAction);
  fileMenu->addAction(openProjectAction);
  fileMenu->addAction(openExampleAction);
  fileMenu->addAction(closeProjectAction);
  fileMenu->addMenu(recentMenu);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  projectMenu = menuBar()->addMenu("Project");

  viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction(ipConfiguratorAction);
  viewMenu->addAction(pinAssignmentAction);

  processMenu = menuBar()->addMenu(tr("&Processing"));
  processMenu->addAction(startAction);
  processMenu->addAction(stopAction);

  helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->addAction(newAction);

  saveToolBar = addToolBar(tr("Save"));
  saveToolBar->addAction(saveAction);
  saveToolBar->setHidden(true);

  debugToolBar = addToolBar(tr("Debug"));
  debugToolBar->addAction(startAction);
  debugToolBar->addAction(stopAction);
}

void MainWindow::showMenus(bool show) {
  menuBar()->setVisible(show);
  fileToolBar->setVisible(show);
  debugToolBar->setVisible(show);
}

void MainWindow::createActions() {
  newAction = new QAction(tr("&New..."), this);
  newAction->setIcon(QIcon(":/images/icon_newfile.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setStatusTip(tr("Create a new source file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  openProjectAction = new QAction(tr("Open &Project..."), this);
  openProjectAction->setStatusTip(tr("Open a new project"));
  connect(openProjectAction, SIGNAL(triggered()), this,
          SLOT(openProjectDialog()));

  openExampleAction = new QAction(tr("Open &Example Design..."), this);
  openExampleAction->setStatusTip(tr("Open example design"));
  connect(openExampleAction, SIGNAL(triggered()), this,
          SLOT(openExampleProject()));

  closeProjectAction = new QAction(tr("&Close Project"), this);
  closeProjectAction->setStatusTip(tr("Close current project"));
  connect(closeProjectAction, SIGNAL(triggered()), this, SLOT(closeProject()));

  newProjdialog = new newProjectDialog(this);
  connect(newProjdialog, SIGNAL(accepted()), this, SLOT(newDialogAccepted()));
  newProjectAction = new QAction(tr("&New Project..."), this);
  newProjectAction->setStatusTip(tr("Create a new project"));
  connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProjectDlg()));

  openFile = new QAction(tr("&Open File..."), this);
  openFile->setStatusTip(tr("Open file"));
  openFile->setIcon(QIcon(":/images/open-file.png"));
  connect(openFile, SIGNAL(triggered()), this, SLOT(openFileSlot()));

  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setStatusTip(tr("Exit the application"));

  startAction = new QAction(tr("Start"), this);
  startAction->setIcon(QIcon(":/images/play.png"));
  startAction->setStatusTip(tr("Start compilation tasks"));

  stopAction = new QAction(tr("Stop"), this);
  stopAction->setIcon(QIcon(":/images/stop.png"));
  stopAction->setStatusTip(tr("Stop compilation tasks"));
  stopAction->setEnabled(false);
  connect(startAction, &QAction::triggered, this, [this]() {
    m_progressWidget->show();
    m_compiler->start();
    m_taskManager->startAll();
  });
  connect(stopAction, &QAction::triggered, this, [this]() {
    m_compiler->Stop();
    m_progressWidget->hide();
  });

  aboutAction = new QAction(tr("About"), this);
  connect(aboutAction, &QAction::triggered, this, [this]() {
    AboutWidget w(m_projectInfo, GlobalSession->Context()->DataPath(), this);
    w.exec();
  });

  connect(exitAction, &QAction::triggered, qApp, [this]() {
    if (this->confirmExitProgram()) {
      Command cmd("gui_stop; exit");
      GlobalSession->CmdStack()->push_and_exec(&cmd);
    }
  });

  pinAssignmentAction = new QAction(tr("Pin Planner"), this);
  pinAssignmentAction->setCheckable(true);
  pinAssignmentAction->setEnabled(false);
  connect(pinAssignmentAction, &QAction::triggered, this,
          &MainWindow::pinAssignmentActionTriggered);

  ipConfiguratorAction = new QAction(tr("IP Configurator"), this);
  ipConfiguratorAction->setCheckable(true);
  ipConfiguratorAction->setEnabled(false);
  connect(ipConfiguratorAction, &QAction::triggered, this,
          &MainWindow::ipConfiguratorActionTriggered);

  saveAction = new QAction(tr("Save"), this);
  connect(saveAction, &QAction::triggered, this,
          &MainWindow::saveActionTriggered);
  saveAction->setIcon(QIcon(":/images/save.png"));
  saveAction->setEnabled(false);
}

void MainWindow::gui_start(bool showWP) {
  ReShowWindow({});
  if (showWP && m_showWelcomePage) showWelcomePage();
}

void MainWindow::showWelcomePage() {
  clearDockWidgets();
  takeCentralWidget()->hide();  // we can't delete it because of singleton

  newDesignCreated({});
  showMenus(false);

  auto exeName =
      QString::fromStdString(GlobalSession->Context()->ExecutableName());
  auto centralWidget = new WelcomePageWidget(
      exeName, GlobalSession->Context()->DataPath(), this);

  centralWidget->addAction(*newProjectAction);
  centralWidget->addAction(*openProjectAction);
  centralWidget->addAction(*openExampleAction);
  uint counter{0};
  for (auto it{m_recentProjectsActions.cbegin()};
       it != m_recentProjectsActions.cend() &&
       counter < RECENT_PROJECT_COUNT_WP;
       ++it, counter++) {
    centralWidget->addRecentProject(*it->first);
  }

  connect(centralWidget, &WelcomePageWidget::welcomePageClosed,
          [&](bool permanently) {
            m_showWelcomePage = !permanently;
            if (permanently) saveWelcomePageConfig();
            ReShowWindow({});
          });

  setCentralWidget(centralWidget);

  centralWidget->show();
  centralWidget->setFocus();
}

void MainWindow::ReShowWindow(QString strProject) {
  clearDockWidgets();
  takeCentralWidget();

  newDesignCreated(strProject);

  resetIps();

  showMenus(true);

  QDockWidget* sourceDockWidget = new QDockWidget(tr("Source"), this);
  sourceDockWidget->setObjectName("sourcedockwidget");
  sourcesForm = new SourcesForm(this);
  connect(sourcesForm, &SourcesForm::CloseProject, this,
          &MainWindow::closeProject, Qt::QueuedConnection);
  connect(sourcesForm, &SourcesForm::OpenProjectSettings, this,
          &MainWindow::openProjectSettings);
  sourceDockWidget->setWidget(sourcesForm);
  addDockWidget(Qt::LeftDockWidgetArea, sourceDockWidget);
  m_projectManager = sourcesForm->ProjManager();
  projectMenu->clear();
  sourcesForm->ProjectSettingsActions()->setEnabled(!strProject.isEmpty());
  projectMenu->addAction(sourcesForm->ProjectSettingsActions());
  // If the project manager path changes, reload settings
  QObject::connect(m_projectManager, &ProjectManager::projectPathChanged, this,
                   &MainWindow::reloadSettings, Qt::UniqueConnection);

  delete m_projectFileLoader;
  m_projectFileLoader = new ProjectFileLoader{Project::Instance()};
  m_projectFileLoader->registerComponent(
      new ProjectManagerComponent{sourcesForm->ProjManager()},
      ComponentId::ProjectManager);
  reloadSettings();

  QDockWidget* propertiesDockWidget = new QDockWidget(tr("Properties"), this);
  PropertyWidget* propertyWidget =
      new PropertyWidget{sourcesForm->ProjManager()};
  connect(sourcesForm, &SourcesForm::ShowProperty, propertyWidget,
          &PropertyWidget::ShowProperty);
  connect(sourcesForm, &SourcesForm::ShowPropertyPanel, propertiesDockWidget,
          &QDockWidget::show);
  propertiesDockWidget->setWidget(propertyWidget->Widget());
  addDockWidget(Qt::LeftDockWidgetArea, propertiesDockWidget);
  propertiesDockWidget->hide();
  connect(sourcesForm, &SourcesForm::IpReconfigRequested, this,
          &MainWindow::handleIpReConfigRequested);
  connect(sourcesForm, &SourcesForm::IpRemoveRequested, this,
          &MainWindow::handleRemoveIpRequested);
  connect(sourcesForm, &SourcesForm::IpDeleteRequested, this,
          &MainWindow::handleDeleteIpRequested);

  TextEditor* textEditor = new TextEditor(this);
  textEditor->RegisterCommands(GlobalSession);
  textEditor->setObjectName("textEditor");
  connect(sourcesForm, SIGNAL(OpenFile(QString)), textEditor,
          SLOT(SlotOpenFile(QString)));
  connect(textEditor, SIGNAL(CurrentFileChanged(QString)), sourcesForm,
          SLOT(SetCurrentFileItem(QString)));

  connect(TextEditorForm::Instance()->GetTabWidget(),
          SIGNAL(currentChanged(int)), this, SLOT(slotTabChanged(int)));

  QWidget* centralWidget = new QWidget(this);
  QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(textEditor->GetTextEditor());
  centralWidget->setLayout(layout);

  setCentralWidget(centralWidget);

  // console
  QDockWidget* consoleDocWidget = new QDockWidget(tr("Console"), this);
  consoleDocWidget->setObjectName("consoledocwidget");
  m_dockConsole = consoleDocWidget;

  StreamBuffer* buffer = new StreamBuffer;
  auto tclConsole = std::make_unique<FOEDAG::TclConsole>(
      m_interpreter->getInterp(), buffer->getStream());
  FOEDAG::TclConsole* c = tclConsole.get();
  TclConsoleWidget* console{nullptr};
  QWidget* w =
      FOEDAG::createConsole(m_interpreter->getInterp(), std::move(tclConsole),
                            buffer, nullptr, &console);
  consoleDocWidget->setWidget(w);
  connect(console, &TclConsoleWidget::linkActivated, this,
          [textEditor](const ErrorInfo& eInfo) {
            textEditor->SlotOpenFileWithLine(eInfo.file, eInfo.line.toInt());
          });
  console->addParser(new DummyParser{});
  console->addParser(new TclErrorParser{});
  m_console = console;

  m_compiler->SetInterpreter(m_interpreter);
  m_compiler->SetOutStream(&buffer->getStream());
  m_compiler->SetErrStream(&console->getErrorBuffer()->getStream());
  auto compilerNotifier = new FOEDAG::CompilerNotifier{c};
  m_compiler->SetTclInterpreterHandler(compilerNotifier);
  auto tclCommandIntegration = sourcesForm->createTclCommandIntegarion();
  m_compiler->setGuiTclSync(tclCommandIntegration);
  connect(tclCommandIntegration, &TclCommandIntegration::newDesign, this,
          &MainWindow::newDesignCreated);

  addDockWidget(Qt::BottomDockWidgetArea, consoleDocWidget);

  // QDockWidget* runDockWidget = new QDockWidget(tr("Design Runs"), this);
  // runDockWidget->setObjectName("designrundockwidget");
  // RunsForm* runForm = new RunsForm(this);
  // runForm->RegisterCommands(GlobalSession);
  // runDockWidget->setWidget(runForm);
  // tabifyDockWidget(consoleDocWidget, runDockWidget);

  // compiler task view
  QWidget* view = prepareCompilerView(m_compiler, &m_taskManager);
  view->setObjectName("compilerTaskView");
  view->setParent(this);

  connect(
      m_taskManager, &TaskManager::progress, this, [this](int val, int max) {
        QProgressBar* progress = m_progressWidget->findChild<QProgressBar*>();
        progress->setMaximum(max);
        progress->setValue(val);
      });

  connect(m_taskManager, &TaskManager::done, this, [this]() {
    if (!m_progressVisible) m_progressWidget->hide();
    m_compiler->finish();
  });

  connect(m_taskManager, &TaskManager::started, this,
          [this]() { m_progressWidget->show(); });

  connect(compilerNotifier, &CompilerNotifier::compilerStateChanged, this,
          &MainWindow::updatePRViewButton);
  m_projectFileLoader->registerComponent(
      new TaskManagerComponent{m_taskManager}, ComponentId::TaskManager);
  m_projectFileLoader->registerComponent(new CompilerComponent(m_compiler),
                                         ComponentId::Compiler);
  QDockWidget* taskDockWidget = new QDockWidget(tr("Task"), this);
  taskDockWidget->setWidget(view);
  addDockWidget(Qt::LeftDockWidgetArea, taskDockWidget);

  connect(m_taskManager, &TaskManager::taskStateChanged, this,
          [this]() { startStopButtonsState(); });
  connect(console, &TclConsoleWidget::stateChanged, this,
          [this, console]() { startStopButtonsState(); });

  sourcesForm->InitSourcesForm();
  // runForm->InitRunsForm();
  updatePRViewButton(static_cast<int>(m_compiler->CompilerState()));
  updateViewMenu();
}

void MainWindow::clearDockWidgets() {
  auto docks = findChildren<QDockWidget*>();
  for (auto dock : docks) {
    removeDockWidget(dock);
  }
}

void MainWindow::reloadSettings() {
  FOEDAG::Settings* settings = GlobalSession->GetSettings();
  if (settings) {
    // Clear out old settings
    settings->clear();

    QString separator = QString::fromStdString(
        std::string(1, std::filesystem::path::preferred_separator));

    // List of json files that will get loaded
    QStringList settingsFiles = {};

    // Helper function to add all json files in a dir to settingsFiles
    auto addFilesFromDir = [&settingsFiles](const QString& dirPath) {
      if (!dirPath.isEmpty()) {
        QDir dir(dirPath);
        QFileInfoList files =
            dir.entryInfoList(QStringList() << "*.json", QDir::Files);
        for (auto file : files) {
          settingsFiles << file.filePath();
        }
      }
    };

    // Add any json files from the system defaults json path
    QString settingsDir =
        GlobalSession->GetSettings()->getSystemDefaultSettingsDir();
    addFilesFromDir(settingsDir);

    // Add any json files from the [projectName].settings folder
    addFilesFromDir(Settings::getUserSettingsPath());

    // Load and merge all our json files
    settings->loadSettings(settingsFiles);
  }
}

void MainWindow::updatePRViewButton(int state) {
  auto name = m_taskManager->task(PLACE_AND_ROUTE_VIEW)->title();
  auto view = findChild<QWidget*>("compilerTaskView");
  if (!view) return;

  if (auto btn = view->findChild<QPushButton*>(name)) {
    const QVector<Compiler::State> availableState{
        {Compiler::State::Routed, Compiler::State::TimingAnalyzed,
         Compiler::State::PowerAnalyzed, Compiler::State::BistreamGenerated}};
    btn->setEnabled(
        availableState.contains(static_cast<Compiler::State>(state)));
  }
}

void MainWindow::saveActionTriggered() {
  if (saveConstraintFile()) saveAction->setEnabled(false);
}

void MainWindow::pinAssignmentActionTriggered() {
  if (pinAssignmentAction->isChecked()) {
    if (PinAssignmentCreator::searchPortsFile(
            m_projectManager->getProjectPath())
            .isEmpty()) {
      auto res = Tcl_Eval(GlobalSession->TclInterp()->getInterp(), "analyze");
      if (res != TCL_OK) {
        QMessageBox::critical(this, "'analyze' command failed",
                              "Please read console logs for 'analyze' above");
        pinAssignmentAction->setChecked(false);
        return;
      }
    }

    PinAssignmentCreator* creator = new PinAssignmentCreator{
        m_projectManager, GlobalSession->Context(), this};
    connect(creator, &PinAssignmentCreator::selectionHasChanged, this,
            [this]() { saveAction->setEnabled(true); });

    auto portsDockWidget = PrepareTab(tr("IO Ports"), "portswidget",
                                      creator->GetPortsWidget(), m_dockConsole);
    auto packagePinDockWidget =
        PrepareTab(tr("Package Pins"), "packagepinwidget",
                   creator->GetPackagePinsWidget(), portsDockWidget);
    m_pinAssignmentDocks = {portsDockWidget, packagePinDockWidget};
  } else {
    if (saveAction->isEnabled()) {
      auto answer = QMessageBox::question(
          this, "Warning",
          "Pin planner data were modified. Do you want to save it?",
          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
          QMessageBox::Yes);
      if (answer == QMessageBox::No) {
        saveAction->setEnabled(false);
        cleanUpDockWidgets(m_pinAssignmentDocks);
      } else if (answer == QMessageBox::Yes) {
        saveActionTriggered();
        const bool saved = !saveAction->isEnabled();
        if (saved)
          cleanUpDockWidgets(m_pinAssignmentDocks);
        else
          pinAssignmentAction->setChecked(true);
      } else {  // Cancel or close button
        pinAssignmentAction->setChecked(true);
      }
    } else {
      cleanUpDockWidgets(m_pinAssignmentDocks);
    }
  }
  saveToolBar->setHidden(!pinAssignmentAction->isChecked());
}

void MainWindow::ipConfiguratorActionTriggered() {
  if (ipConfiguratorAction->isChecked()) {
    IpConfiguratorCreator creator;
    // Available IPs DockWidget
    m_availableIpsgDockWidget = PrepareTab(tr("IPs"), "availableIpsWidget",
                                           creator.GetAvailableIpsWidget(),
                                           nullptr, Qt::RightDockWidgetArea);

    // Get the actual IpCatalogTree
    auto ipsWidgets = m_availableIpsgDockWidget->findChildren<IpCatalogTree*>();
    if (ipsWidgets.count() > 0) {
      m_ipCatalogTree = ipsWidgets[0];

      // Update the IP Config widget when the Available IPs selection changes
      QObject::connect(m_ipCatalogTree, &IpCatalogTree::itemSelectionChanged,
                       this, &MainWindow::handleIpTreeSelectionChanged);
    }

    // update the console for input incase the IP system printed any messages
    // which can break the console currently
    m_console->showPrompt();
  } else {
    std::vector<QDockWidget*> docks = {m_availableIpsgDockWidget,
                                       m_ipConfigDockWidget};
    cleanUpDockWidgets(docks);
    m_ipConfigDockWidget = nullptr;
    m_availableIpsgDockWidget = nullptr;
    m_ipCatalogTree = nullptr;
  }
}

void MainWindow::newDialogAccepted() {
  if (newProjdialog->GetMode() == Mode::NewProject) {
    const QString strproject = newProjdialog->getProject();
    newProjectAction->setEnabled(false);
    ReShowWindow(strproject);
  } else {
    sourcesForm->UpdateSrcHierachyTree();
  }
}

void MainWindow::updateSourceTree() {
  if (sourcesForm) {
    sourcesForm->InitSourcesForm();
  }
}

void MainWindow::handleIpTreeSelectionChanged() {
  // Create a config widget based off the current AvailableIps tree selection
  if (m_ipCatalogTree != nullptr) {
    auto items = m_ipCatalogTree->selectedItems();
    if (items.count() > 0) {
      // Create a new config widget for the selected IP
      // Note: passing null for the last 2 args causes a configure instead of a
      // re-configure
      handleIpReConfigRequested(items[0]->text(0), {}, {});
    }
  }
}

void MainWindow::handleIpReConfigRequested(const QString& ipName,
                                           const QString& moduleName,
                                           const QStringList& paramList) {
  IpConfigWidget* configWidget =
      new IpConfigWidget(this, ipName, moduleName, paramList);
  replaceIpConfigDockWidget(configWidget);
}

void MainWindow::handleRemoveIpRequested(const QString& moduleName) {
  Compiler* compiler{};
  IPGenerator* ipGen{};

  if ((compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    ipGen->RemoveIPInstance(moduleName.toStdString());
  }

  updateSourceTree();
}

void MainWindow::handleDeleteIpRequested(const QString& moduleName) {
  Compiler* compiler{};
  IPGenerator* ipGen{};

  if ((compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    ipGen->DeleteIPInstance(moduleName.toStdString());
  }

  updateSourceTree();
}

void MainWindow::resetIps() {
  Compiler* compiler{};
  IPGenerator* ipGen{};

  if ((compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    ipGen->ResetIPList();
  }
}

void MainWindow::updateViewMenu() {
  viewMenu->clear();
  viewMenu->addAction(ipConfiguratorAction);
  viewMenu->addAction(pinAssignmentAction);
  const QList<QDockWidget*> dockwidgets = findChildren<QDockWidget*>();
  if (!dockwidgets.empty()) {
    viewMenu->addSeparator();
    for (int i = 0; i < dockwidgets.size(); ++i) {
      QDockWidget* dockWidget = dockwidgets.at(i);
      if (layout()->indexOf(dockWidget) == -1) continue;
      viewMenu->addAction(dockWidget->toggleViewAction());
    }
  }
}

void MainWindow::slotTabChanged(int index) {
  QString strName = TextEditorForm::Instance()->GetTabWidget()->tabText(index);
  SetWindowTitle((index == -1) ? QString() : strName,
                 m_projectManager->getProjectName(), m_projectInfo.name);
}

void MainWindow::handleProjectOpened() {
  // this fires after openProject() has been fired

  // Reloading IPs after full project load as during ReShowWindow, the project
  // path variables haven't been updated yet
  IpConfigurator::ReloadIps();
  // Update tree to show new instances
  updateSourceTree();
  // Fix command prompt if any errors were printed during load
  m_console->showPrompt();
}

void MainWindow::saveWelcomePageConfig() {
  // So far the only configuration is boolean, indicating whether welcome page
  // should we shown. To store it we just save a file in a working directory -
  // if it's there, we don't show the welcome page.
  QFile file(WELCOME_PAGE_CONFIG_FILE);
  if (file.open(QIODevice::WriteOnly)) file.close();
}

void MainWindow::recentProjectOpen() {
  auto action = qobject_cast<QAction*>(sender());
  auto project = std::find_if(m_recentProjectsActions.cbegin(),
                              m_recentProjectsActions.cend(),
                              [action](const std::pair<QAction*, QString>& p) {
                                return p.first == action;
                              });
  if (project != m_recentProjectsActions.end()) {
    const QString name = project->second;
    if (!name.isEmpty()) openProject(name);
  }
}

void MainWindow::openProjectSettings() {
  newProjdialog->Reset(Mode::ProjectSettings);
  newProjdialog->open();
}

void MainWindow::replaceIpConfigDockWidget(QWidget* newWidget) {
  IpConfigWidget* configWidget = qobject_cast<IpConfigWidget*>(newWidget);
  if (configWidget) {
    // Listen for IpInstance selection changes in the source tree
    QObject::connect(configWidget, &IpConfigWidget::ipInstancesUpdated, this,
                     &MainWindow::updateSourceTree);
  }

  // If dock widget has already been created
  if (m_ipConfigDockWidget) {
    // remove old config widget
    auto oldWidget = m_ipConfigDockWidget->widget();
    if (oldWidget) {
      delete m_ipConfigDockWidget->widget();
    }

    // set new config widget
    m_ipConfigDockWidget->setWidget(newWidget);
    m_ipConfigDockWidget->show();
  } else {  // If dock widget hasn't been created
    // Create and place new dockwidget
    m_ipConfigDockWidget =
        PrepareTab(tr("Configure IP"), "configureIpsWidget", newWidget, nullptr,
                   Qt::RightDockWidgetArea);
  }
}

bool MainWindow::confirmCloseProject() {
  return (QMessageBox::question(
              this, "Close Project?",
              tr("Are you sure you want to close your project?\n"),
              QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes);
}
bool MainWindow::confirmExitProgram() {
  return (QMessageBox::question(
              this, "Exit Program?",
              tr("Are you sure you want to exit the program?\n"),
              QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes);
}
