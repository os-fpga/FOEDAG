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
#include "Main/CompilerNotifier.h"
#include "Main/Foedag.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "NewFile/new_file.h"
#include "NewProject/Main/registerNewProjectCommands.h"
#include "NewProject/new_project_dialog.h"
#include "ProjNavigator/PropertyWidget.h"
#include "ProjNavigator/sources_form.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "TextEditor/text_editor.h"

using namespace FOEDAG;

MainWindow::MainWindow(Session* session) : m_session(session) {
  /* Window settings */
  setWindowTitle(tr("FOEDAG"));
  resize(350, 250);
  m_compiler = session->GetCompiler();
  m_interpreter = session->TclInterp();
  QDesktopWidget dw;
  setGeometry(dw.width() / 6, dw.height() / 6, dw.width() * 2 / 3,
              dw.height() * 2 / 3);

  setDockNestingEnabled(true);

  setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  /* Create actions that can be added to menu/tool bars */
  createActions();

  /* Create menu bars */
  createMenus();

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
}

void MainWindow::Tcl_NewProject(int argc, const char* argv[]) {
  ProjectManager* projectManager = new ProjectManager(this);
  projectManager->Tcl_CreateProject(argc, argv);
}

void MainWindow::newFile() {
  //  QTextStream out(stdout);
  //  out << "New file is requested\n";
  NewFile* newfile = new NewFile(this);
  newfile->StartNewFile();
}

void MainWindow::newProjectDlg() {
  int ret = newProjdialog->exec();
  newProjdialog->close();
  if (ret) {
    QString strproject = newProjdialog->getProject();
    newProjectAction->setEnabled(false);
    ReShowWindow(strproject);
  }
}

void MainWindow::openProject() {
  QString fileName = "";
  fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), "",
                                          "FOEDAG Project File(*.ospr)");
  if ("" != fileName) {
    ReShowWindow(fileName);
  }
}

void MainWindow::closeProject() {
  if (m_projectManager && m_projectManager->HasDesign()) {
    Project::Instance()->InitProject();
    newProjdialog->Reset();
    ReShowWindow("");
    newProjectAction->setEnabled(true);
  }
}

void MainWindow::openFileSlot() {
  const QString file = QFileDialog::getOpenFileName(this, tr("Open file"));
  auto editor = findChild<TextEditor*>("textEditor");
  if (editor) editor->SlotOpenFile(file);
}

void MainWindow::newDesignCreated(const QString& design) {
  setWindowTitle(tr(mainWindowName.c_str()) + " - " + design);
}

void MainWindow::startStopButtonsState() {
  const bool inProgress = m_taskManager->status() == TaskStatus::InProgress;
  const bool consoleInProgress = m_console->isRunning();
  startAction->setEnabled(!inProgress && !consoleInProgress);
  stopAction->setEnabled(inProgress && consoleInProgress);
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openFile);
  fileMenu->addSeparator();
  fileMenu->addAction(newProjectAction);
  fileMenu->addAction(openProjectAction);
  fileMenu->addAction(closeProjectAction);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  processMenu = menuBar()->addMenu(tr("&Processing"));
  processMenu->addAction(startAction);
  processMenu->addAction(stopAction);
}

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->addAction(newAction);

  debugToolBar = addToolBar(tr("Debug"));
  debugToolBar->addAction(startAction);
  debugToolBar->addAction(stopAction);
}

void MainWindow::createActions() {
  newAction = new QAction(tr("&New"), this);
  newAction->setIcon(QIcon(":/images/icon_newfile.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setStatusTip(tr("Create a new source file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  openProjectAction = new QAction(tr("&Open Project"), this);
  openProjectAction->setStatusTip(tr("Open a new project"));
  connect(openProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));

  closeProjectAction = new QAction(tr("&Close Project"), this);
  closeProjectAction->setStatusTip(tr("Close current project"));
  connect(closeProjectAction, SIGNAL(triggered()), this, SLOT(closeProject()));

  newProjdialog = new newProjectDialog(this);
  newProjectAction = new QAction(tr("&New Project"), this);
  newProjectAction->setStatusTip(tr("Create a new project"));
  connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProjectDlg()));

  openFile = new QAction(tr("&Open File"), this);
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
    m_compiler->SetHardError(false);
    m_compiler->start();
    m_taskManager->startAll();
    m_compiler->finish();
  });
  connect(stopAction, &QAction::triggered, this, [this]() {
    m_console->terminate();
    m_compiler->Stop();
  });

  connect(exitAction, &QAction::triggered, qApp, [this]() {
    Command cmd("gui_stop; exit");
    GlobalSession->CmdStack()->push_and_exec(&cmd);
  });
}

void MainWindow::gui_start() { ReShowWindow(""); }

void MainWindow::ReShowWindow(QString strProject) {
  clearDockWidgets();
  takeCentralWidget();

  setWindowTitle(tr(mainWindowName.c_str()) + " - " + strProject);

  QDockWidget* sourceDockWidget = new QDockWidget(tr("Source"), this);
  sourceDockWidget->setObjectName("sourcedockwidget");
  SourcesForm* sourForm = new SourcesForm(this);
  connect(sourForm, &SourcesForm::CloseProject, this, &MainWindow::closeProject,
          Qt::QueuedConnection);
  sourForm->InitSourcesForm(strProject);
  sourceDockWidget->setWidget(sourForm);
  addDockWidget(Qt::LeftDockWidgetArea, sourceDockWidget);
  m_projectManager = sourForm->ProjManager();
  // If the project manager path changes, reload settings
  QObject::connect(m_projectManager, &ProjectManager::projectPathChanged, this,
                   &MainWindow::reloadSettings, Qt::UniqueConnection);

  reloadSettings();  // This needs to be after
                     // sourForm->InitSourcesForm(strProject); so the project
                     // info exists

  QDockWidget* propertiesDockWidget = new QDockWidget(tr("Properties"), this);
  PropertyWidget* propertyWidget = new PropertyWidget{sourForm->ProjManager()};
  connect(sourForm, &SourcesForm::ShowProperty, propertyWidget,
          &PropertyWidget::ShowProperty);
  connect(sourForm, &SourcesForm::ShowPropertyPanel, propertiesDockWidget,
          &QDockWidget::show);
  propertiesDockWidget->setWidget(propertyWidget->Widget());
  addDockWidget(Qt::LeftDockWidgetArea, propertiesDockWidget);
  propertiesDockWidget->hide();

  TextEditor* textEditor = new TextEditor(this);
  textEditor->RegisterCommands(GlobalSession);
  textEditor->setObjectName("textEditor");

  connect(sourForm, SIGNAL(OpenFile(QString)), textEditor,
          SLOT(SlotOpenFile(QString)));
  connect(textEditor, SIGNAL(CurrentFileChanged(QString)), sourForm,
          SLOT(SetCurrentFileItem(QString)));

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
  m_compiler->SetTclInterpreterHandler(new FOEDAG::CompilerNotifier{c});
  auto tclCommandIntegration = sourForm->createTclCommandIntegarion();
  m_compiler->setGuiTclSync(tclCommandIntegration);
  connect(tclCommandIntegration, &TclCommandIntegration::newDesign, this,
          &MainWindow::newDesignCreated);

  addDockWidget(Qt::BottomDockWidgetArea, consoleDocWidget);

  QDockWidget* runDockWidget = new QDockWidget(tr("Design Runs"), this);
  runDockWidget->setObjectName("designrundockwidget");
  RunsForm* runForm = new RunsForm(this);
  runForm->InitRunsForm(strProject);
  runForm->RegisterCommands(GlobalSession);
  runDockWidget->setWidget(runForm);
  tabifyDockWidget(consoleDocWidget, runDockWidget);

  // compiler task view
  QWidget* view = prepareCompilerView(m_compiler, &m_taskManager);
  QDockWidget* taskDocWidget = new QDockWidget(tr("Task"), this);
  taskDocWidget->setWidget(view);
  tabifyDockWidget(sourceDockWidget, taskDocWidget);

  connect(m_taskManager, &TaskManager::taskStateChanged, this,
          [this]() { startStopButtonsState(); });
  connect(console, &TclConsoleWidget::stateChanged, this,
          [this, console]() { startStopButtonsState(); });
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
    addFilesFromDir(FOEDAG::getTaskUserSettingsPath());

    // Load and merge all our json files
    settings->loadSettings(settingsFiles);
  }
}
