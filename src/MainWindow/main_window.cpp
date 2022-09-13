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
#include "IpConfigurator/IpConfiguratorCreator.h"
#include "Main/CompilerNotifier.h"
#include "Main/Foedag.h"
#include "Main/ProjectFile/ProjectFileLoader.h"
#include "Main/Tasks.h"
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
#include "foedag_version.h"

using namespace FOEDAG;
extern const char* foedag_version_number;
extern const char* foedag_git_hash;
extern const char* foedag_build_type;

const QString MainWindow::WELCOME_PAGE_CONFIG_FILE = "WelcomePageConfig";

MainWindow::MainWindow(Session* session) : m_session(session) {
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

void MainWindow::openProject() { openProject(QString{}); }

void MainWindow::openExampleProject() {
  auto currentDir = GlobalSession->Context()->DataPath();
  std::filesystem::path examplesPath = currentDir / "examples";
  openProject(QString::fromStdString(examplesPath.string()));
}

void MainWindow::openProject(const QString& dir) {
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), dir,
                                          "FOEDAG Project File(*.ospr)");
  if (!fileName.isEmpty()) {
    ReShowWindow(fileName);
    loadFile(fileName);
  }
}

void MainWindow::closeProject() {
  if (m_projectManager && m_projectManager->HasDesign()) {
    Project::Instance()->InitProject();
    newProjdialog->Reset();
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
}

void MainWindow::startStopButtonsState() {
  const bool inProgress = m_taskManager->status() == TaskStatus::InProgress;
  const bool consoleInProgress = m_console->isRunning();
  startAction->setEnabled(!inProgress && !consoleInProgress);
  stopAction->setEnabled(inProgress && consoleInProgress);
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
    removeDockWidget(dock);
    delete dock->widget();
    delete dock;
  }
  dockWidgets.clear();
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
    auto answer = QMessageBox::question(
        this, "Save constraint file...",
        "Do you want to rewrite current constraint file?", btns);
    if (answer == QMessageBox::Cancel) return false;
    rewrite = (answer == QMessageBox::Yes);
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

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openFile);
  fileMenu->addSeparator();
  fileMenu->addAction(newProjectAction);
  fileMenu->addAction(openProjectAction);
  fileMenu->addAction(openExampleAction);
  fileMenu->addAction(closeProjectAction);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

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
  connect(openProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));

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
    AboutWidget w(m_projectInfo, this);
    w.exec();
  });

  connect(exitAction, &QAction::triggered, qApp, [this]() {
    Command cmd("gui_stop; exit");
    GlobalSession->CmdStack()->push_and_exec(&cmd);
  });

  pinAssignmentAction = new QAction(tr("Pin Assignment"), this);
  pinAssignmentAction->setCheckable(true);
  pinAssignmentAction->setEnabled(false);
  connect(pinAssignmentAction, &QAction::triggered, this,
          &MainWindow::pinAssignmentActionTriggered);

  ipConfiguratorAction = new QAction(tr("IP Configurator"), this);
  ipConfiguratorAction->setCheckable(true);
  connect(ipConfiguratorAction, &QAction::triggered, this, [this]() {
    if (ipConfiguratorAction->isChecked()) {
      IpConfiguratorCreator creator;
      auto availableIpsDockWidget = PrepareTab(
          tr("IPs"), "availableIpsWidget", creator.GetAvailableIpsWidget(),
          nullptr, Qt::RightDockWidgetArea);
      m_ipConfiguratorDocks = {availableIpsDockWidget};
      m_console->showPrompt();
    } else {
      cleanUpDockWidgets(m_ipConfiguratorDocks);
    }
  });

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

  showMenus(true);

  QDockWidget* sourceDockWidget = new QDockWidget(tr("Source"), this);
  sourceDockWidget->setObjectName("sourcedockwidget");
  sourcesForm = new SourcesForm(this);
  connect(sourcesForm, &SourcesForm::CloseProject, this,
          &MainWindow::closeProject, Qt::QueuedConnection);
  sourceDockWidget->setWidget(sourcesForm);
  addDockWidget(Qt::LeftDockWidgetArea, sourceDockWidget);
  m_projectManager = sourcesForm->ProjManager();
  // If the project manager path changes, reload settings
  QObject::connect(m_projectManager, &ProjectManager::projectPathChanged, this,
                   &MainWindow::reloadSettings, Qt::UniqueConnection);

  delete m_projectFileLoader;
  m_projectFileLoader = new ProjectFileLoader{Project::Instance()};
  m_projectFileLoader->registerComponent(
      new ProjectManagerComponent{sourcesForm->ProjManager()},
      ComponentId::ProjectManager);
  reloadSettings();  // This needs to be after
                     // sourForm->InitSourcesForm(strProject); so the project
                     // info exists

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
    m_progressWidget->hide();
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
          "Pin assignment data were modified. Do you want to save it?",
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

void MainWindow::newDialogAccepted() {
  const QString strproject = newProjdialog->getProject();
  newProjectAction->setEnabled(false);
  ReShowWindow(strproject);
}

void MainWindow::slotTabChanged(int index) {
  QString strName = TextEditorForm::Instance()->GetTabWidget()->tabText(index);
  SetWindowTitle((index == -1) ? QString() : strName,
                 m_projectManager->getProjectName(), m_projectInfo.name);
}

void MainWindow::saveWelcomePageConfig() {
  // So far the only configuration is boolean, indicating whether welcome page
  // should we shown. To store it we just save a file in a working directory -
  // if it's there, we don't show the welcome page.
  QFile file(WELCOME_PAGE_CONFIG_FILE);
  if (file.open(QIODevice::WriteOnly)) file.close();
}
