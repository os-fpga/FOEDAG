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

#include <QLabel>
#include <QListView>
#include <QTextStream>
#include <QtWidgets>
#include <fstream>

#include "ChatGptWidget.h"
#include "Compiler/Compiler.h"
#include "Compiler/CompilerDefines.h"
#include "Compiler/Constraints.h"
#include "Compiler/TaskManager.h"
#include "Compiler/TaskModel.h"
#include "Compiler/TaskTableView.h"
#include "CompressProject.h"
#include "Console/DummyParser.h"
#include "Console/FileNameParser.h"
#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "Console/TclErrorParser.h"
#include "DesignRuns/runs_form.h"
#include "DockWidget.h"
#include "EditorSettings.h"
#include "IpConfigurator/IpCatalogTree.h"
#include "IpConfigurator/IpConfigWidget.h"
#include "IpConfigurator/IpConfigurator.h"
#include "IpConfigurator/IpConfiguratorCreator.h"
#include "LicenseManagerWidget.h"
#include "Main/CompilerNotifier.h"
#include "Main/DialogProvider.h"
#include "Main/Foedag.h"
#include "Main/ProjectFile/ProjectFileLoader.h"
#include "Main/licenseviewer.h"
#include "MainWindow/Session.h"
#include "MainWindow/WelcomePageWidget.h"
#include "MessagesTabWidget.h"
#include "NewFile/new_file.h"
#include "NewProject/Main/registerNewProjectCommands.h"
#include "NewProject/ProjectManager/DesignFileWatcher.h"
#include "NewProject/new_project_dialog.h"
#include "PathEdit.h"
#include "PinAssignment/PinAssignmentCreator.h"
#include "ProjNavigator/PropertyWidget.h"
#include "ProjNavigator/sources_form.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "ReportsTreeWidget.h"
#include "Tasks.h"
#include "TextEditor/text_editor.h"
#include "TextEditor/text_editor_form.h"
#include "Utils/FileUtils.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "WidgetFactory.h"
#include "foedag_version.h"

using namespace FOEDAG;
extern const char* foedag_version_number;
extern const char* foedag_git_hash;
extern const char* foedag_build_type;

static constexpr const char* LICENSES_DIR{"licenses"};

namespace {
const QString RECENT_PROJECT_KEY{"recent/proj%1"};
const QString EDITOR_KEY{"editors/editor%1"};
const QString SHOW_WELCOMEPAGE_KEY{"showWelcomePage"};
const QString SHOW_STOP_COMPILATION_MESSAGE_KEY{"showStopCompilationMessage"};
const QString SHOW_MESSAGE_ON_EXIT_KEY{"showMessageOnExit"};
constexpr uint RECENT_PROJECT_COUNT{10};
constexpr uint RECENT_PROJECT_COUNT_WP{5};
constexpr const char* WELCOME_PAGE_MENU_PROP{"showOnWelcomePage"};
constexpr const char* DEFAULT_PROJECT_PATH{"defaultProjectPath"};
constexpr const char* PIN_PLANNER_PIN_NAME{"pinPlannerPinName"};

void centerWidget(QWidget& widget) {
  auto screenGeometry = qApp->primaryScreen()->availableGeometry();

  // Take 2/3 part of the screen.
  auto mainWindowSize =
      QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3);
  // Center main window on the screen. It will get this geometry after switching
  // from maximized mode.
  widget.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                         mainWindowSize, screenGeometry));
}
}  // namespace

MainWindow::MainWindow(Session* session)
    : m_session(session), m_settings("settings", QSettings::IniFormat) {
  /* Window settings */
  m_compiler = session->GetCompiler();
  m_interpreter = session->TclInterp();

  m_showWelcomePage = m_settings.value(SHOW_WELCOMEPAGE_KEY, true).toBool();
  m_askStopCompilation =
      m_settings.value(SHOW_STOP_COMPILATION_MESSAGE_KEY, true).toBool();
  m_askShowMessageOnExit =
      m_settings.value(SHOW_MESSAGE_ON_EXIT_KEY, true).toBool();

  centerWidget(*this);

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

  //  QDockWidget* texteditorDockWidget = new DockWidget(tr("Text Editor"));
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
  connect(this, &MainWindow::runProjectRequested, this,
          &MainWindow::onRunProjectRequested, Qt::QueuedConnection);
  connect(DesignFileWatcher::Instance(), &DesignFileWatcher::designFilesChanged,
          this, &MainWindow::onDesignFilesChanged);
  connect(DesignFileWatcher::Instance(), &DesignFileWatcher::designCreated,
          this, &MainWindow::onDesignCreated);
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

bool MainWindow::CloseOpenedTabs() {
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  while (tabWidget && (tabWidget->count() != 0)) {
    if (!TextEditorForm::Instance()->TabCloseRequested(
            tabWidget->currentIndex()))
      return false;
  }
  return true;
}

void MainWindow::ProgressVisible(bool visible) {
  m_progressVisible = visible;
  m_progressBar->setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (confirmExitProgram()) {
    forceStopCompilation();
    event->accept();
  } else {
    event->ignore();
  }
}

void MainWindow::ScriptFinished() {
  ProgressVisible(false);
  const QSignalBlocker signalBlocker{DesignFileWatcher::Instance()};
  DesignFileWatcher::Instance()->updateDesignFileWatchers(m_projectManager);
  saveSettings();
}

void MainWindow::newFile() {
  //  QTextStream out(stdout);
  //  out << "New file is requested\n";
  NewFile* newfile = new NewFile(this);
  newfile->StartNewFile();
}

void MainWindow::newProjectDlg() {
  if (lastProjectClosed()) {
    newProjdialog->Reset();
    newProjdialog->open();
  }
}

void MainWindow::openExampleProject() {
  if (!lastProjectClosed()) return;
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

    if (src != dest) {
      // QT doesn't have a convenience function for recursively copying a folder
      // so we'll use std::filesystem instead
      std::filesystem::path srcPath = std::filesystem::path(src.toStdString());
      std::filesystem::path destPath =
          std::filesystem::path(dest.toStdString());
      std::filesystem::copy(srcPath, destPath,
                            std::filesystem::copy_options::recursive);
    }

    // open the newly copied example project
    openProject(dest + QDir::separator() + file, false, false);
  }
}

void MainWindow::openProjectDialog(const QString& dir) {
  if (!lastProjectClosed()) return;
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), dir,
                                          "FOEDAG Project File(*.ospr)");
  if (!fileName.isEmpty()) openProject(fileName, false, false);
}

void MainWindow::closeProject(bool force) {
  if (!lastProjectClosed()) return;
  if (m_projectManager && m_projectManager->HasDesign()) {
    if (!force && !confirmCloseProject()) return;
    ipConfiguratorAction->setChecked(false);
    forceStopCompilation();
    Project::Instance()->InitProject();
    newProjdialog->Reset();
    m_showWelcomePage ? showWelcomePage() : ReShowWindow({});
    newProjectAction->setEnabled(true);
    setStatusAndProgressText(QString{});
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
  simulationMenu->setEnabled(!design.isEmpty());
  updateTaskTable();
  compressProjectAction->setEnabled(!design.isEmpty());
}

void MainWindow::chatGpt(const QString& request, const QString& content) {
  if (!m_progressVisible) m_progressBar->hide();
  setStatusAndProgressText("ChatGPT: done");
  auto reportName = "Chat GPT";
  const bool reset{request.isEmpty()};
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  auto addItem = [this](const QString& request, const QString& content) {
    QStandardItem* item = new QStandardItem();
    item->setData("Chat GPT", ListViewDelegate::HeaderRole);
    item->setData(content, ListViewDelegate::SubheaderRole);
    if (m_chatgptModel) {
      m_chatgptModel->insertRow(0, item);
      item = new QStandardItem();
      item->setData("User", ListViewDelegate::HeaderRole);
      item->setData(request, ListViewDelegate::SubheaderRole);
      m_chatgptModel->insertRow(0, item);
    }
  };
  if (reset) {
    if (m_chatgptModel) {
      m_chatgptModel->clear();
    }
  }
  for (int i = 0; i < tabWidget->count(); i++) {
    if (tabWidget->tabText(i) == reportName) {
      if (reset) {
        tabWidget->removeTab(i);
        return;
      }
      tabWidget->setCurrentIndex(i);
      addItem(request, content);
      return;
    }
  }

  if (reset) return;

  if (m_chatGptListView) {
    tabWidget->addTab(m_chatGptListView, reportName);
    tabWidget->setCurrentWidget(m_chatGptListView);
    addItem(request, content);
    return;
  }

  // tab doesn't exist yet
  m_chatgptModel = new QStandardItemModel();

  m_chatGptListView = new QListView;
  m_chatGptListView->setItemDelegate(new ListViewDelegate{});
  m_chatGptListView->setModel(m_chatgptModel);
  m_chatGptListView->setEditTriggers(
      QAbstractItemView::EditTrigger::NoEditTriggers);
  m_chatGptListView->setResizeMode(QListView::ResizeMode::Adjust);
  m_chatGptListView->setVerticalScrollMode(QListView::ScrollPerPixel);
  m_chatGptListView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_chatGptListView, &QListView::customContextMenuRequested, this,
          [this](const QPoint& pos) {
            QMenu menu{m_chatGptListView};
            auto copy = new QAction{"Copy"};
            connect(copy, &QAction::triggered, m_chatGptListView,
                    [this, pos]() {
                      auto index = m_chatGptListView->indexAt(pos);
                      if (index.isValid()) {
                        auto data = m_chatGptListView->model()->data(
                            index, ListViewDelegate::SubheaderRole);
                        QClipboard* clipboard = QGuiApplication::clipboard();
                        clipboard->setText(data.toString());
                      }
                    });
            menu.addAction(copy);
            menu.exec(m_chatGptListView->mapToGlobal(pos));
          });

  QStandardItem* item = new QStandardItem();
  item->setData("User", ListViewDelegate::HeaderRole);
  item->setData(request, ListViewDelegate::SubheaderRole);
  m_chatgptModel->appendRow(item);

  item = new QStandardItem();
  item->setData("Chat GPT", ListViewDelegate::HeaderRole);
  item->setData(content, ListViewDelegate::SubheaderRole);
  m_chatgptModel->appendRow(item);

  tabWidget->addTab(m_chatGptListView, reportName);
  tabWidget->setCurrentWidget(m_chatGptListView);

  connect(tabWidget, &TabWidget::resized, this,
          [this](const QSize& s) { m_chatGptListView->resize(s); });
}

void MainWindow::chatGptStatus(bool status) {
  if (status) {
    m_progressBar->setMaximum(0);
    m_progressBar->setValue(0);
    m_progressBar->show();
    setStatusAndProgressText("ChatGPT: pending response");
  } else {
    if (!m_progressVisible) m_progressBar->hide();
    setStatusAndProgressText("ChatGPT: failed");
  }
}

void MainWindow::startStopButtonsState() {
  const bool startEn{m_taskManager->status() != TaskStatus::InProgress &&
                     !m_console->isRunning()};
  startAction->setEnabled(startEn);
  startSimAction->setEnabled(startEn);
  // Enable Stop action when there is something to stop
  stopAction->setEnabled(isRunning());
  if (m_taskView) m_taskView->setViewDisabled(isRunning());
  const QList<QAction*> actions =
      QList<QAction*>{} << simulationMenu->actions() << projectMenu->actions()
                        << QList<QAction*>{
                               newProjectAction,     openProjectAction,
                               openExampleAction,    closeProjectAction,
                               ipConfiguratorAction, pinAssignmentAction};
  for (auto action : actions) action->setDisabled(isRunning());
  recentMenu->setDisabled(isRunning());
  if (m_reportsDockWidget && m_reportsDockWidget->widget())
    m_reportsDockWidget->widget()->setDisabled(isRunning());
}

DockWidget* MainWindow::PrepareTab(const QString& name, const QString& objName,
                                   QWidget* widget, QDockWidget* tabToAdd,
                                   Qt::DockWidgetArea area) {
  DockWidget* dock = new DockWidget(name, this);
  dock->setObjectName(objName);
  dock->setWidget(widget);
  addDockWidget(area, dock);
  if (tabToAdd != nullptr) {
    tabifyDockWidget(tabToAdd, dock);
  }
  return dock;
}

void MainWindow::addPinPlannerRefreshButton(QDockWidget* dock) {
  auto btn = new QPushButton{dock};
  btn->setObjectName("refreshButton");
  connect(btn, &QPushButton::clicked, this, &MainWindow::refreshPinPlanner);
  btn->setSizePolicy(QSizePolicy{QSizePolicy::Maximum, QSizePolicy::Maximum});
  btn->setText("Refresh");
  QWidget* w = new QWidget;
  auto layout = new QHBoxLayout;
  layout->addWidget(new QLabel{dock->windowTitle()});

  auto saveButton = new QPushButton{dock};
  saveButton->setObjectName("saveButton");
  connect(saveButton, &QPushButton::clicked, this,
          &MainWindow::saveActionTriggered);
  saveButton->setSizePolicy(
      QSizePolicy{QSizePolicy::Maximum, QSizePolicy::Maximum});
  saveButton->setIcon(QIcon{":/images/save-action.png"});
  saveButton->setToolTip("Save to *.pin file");
  saveButton->setEnabled(false);
  m_saveButtons.push_back(saveButton);
  layout->addWidget(saveButton);

  layout->addWidget(btn);
  layout->addSpacerItem(
      new QSpacerItem{10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding});
  layout->setContentsMargins(9, 9, 9, 0);
  w->setLayout(layout);
  dock->setTitleBarWidget(w);
  btn->hide();
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
  m_saveButtons.clear();
}

void MainWindow::openProject(const QString& project, bool delayedOpen,
                             bool run) {
  if (delayedOpen) {
    emit runProjectRequested(project);
    return;
  }

  ReShowWindow(project);
  loadFile(project);
  // need load settings again in case we have settings depends on the selected
  // device. E.g. DSP or BRAM
  reloadSettings();
  emit projectOpened();

  // this should be first in order to keep console visible.
  // Otherwise message or repost tab become visible.
  m_dockConsole->setVisible(true);
  showMessagesTab();
  showReportsTab();

  if (run) startProject(false);
  setStatusAndProgressText(QString{});
}

bool MainWindow::isRunning() const {
  return m_taskManager->status() == TaskStatus::InProgress ||
         m_console->isRunning();
}

void MainWindow::onRunProjectRequested(const QString& project) {
  openProject(project, false, true);
}

void MainWindow::stopCompilation() {
  bool stop{true};
  if (m_askStopCompilation) {
    QMessageBox question{QMessageBox::Question, "Stop compilation",
                         "Do you want stop compilation?",
                         QMessageBox::No | QMessageBox::Yes, this};
    auto combo = new QCheckBox("Do not show this message again");
    connect(combo, &QCheckBox::stateChanged, this, [this](int state) {
      stopCompileMessageAction->setChecked(state != Qt::Checked);
    });
    question.setCheckBox(combo);
    auto res{question.exec()};
    stop = (res == QMessageBox::Yes);
  }

  if (stop) {
    forceStopCompilation();
  }
}

void MainWindow::forceStopCompilation() {
  m_compiler->Stop();
  m_progressBar->hide();
}

void MainWindow::showMessagesTab() {
  auto newWidget = new MessagesTabWidget(*m_taskManager,
                                         GlobalSession->Context()->DataPath());

  auto oldWidget = m_messagesDockWidget->widget();
  if (oldWidget) {
    delete oldWidget;
  } else {
    // Show messages tab for the first time compilation is finished
    m_messagesDockWidget->toggleViewAction()->setChecked(true);
  }
  // set new messages widget
  m_messagesDockWidget->setWidget(newWidget);
  if (m_messagesDockWidget->toggleViewAction()->isChecked())
    m_messagesDockWidget->show();
}

void MainWindow::showReportsTab() {
  auto newReportsWidget = new ReportsTreeWidget(m_compiler, *m_taskManager);
  // remove old config widget
  auto oldWidget = m_reportsDockWidget->widget();
  if (oldWidget) {
    delete oldWidget;
  } else {
    // Show reports tab for the first time compilation is finished
    m_reportsDockWidget->toggleViewAction()->setChecked(true);
  }
  // set new config widget
  m_reportsDockWidget->setWidget(newReportsWidget);
  if (m_reportsDockWidget->toggleViewAction()->isChecked())
    m_reportsDockWidget->show();
}

void MainWindow::fileModified(const QString& file) {
  if (m_blockRefereshEn) return;
  auto pinAssignment = findChild<PinAssignmentCreator*>();
  if (!pinAssignment) return;

  if (file == pinAssignment->data().pinFile) {
    setVisibleRefreshButtons(true);
  }
}

void MainWindow::refreshPinPlanner() {
  auto pinAssignment = findChild<PinAssignmentCreator*>();
  if (!pinAssignment) return;

  if (isEnableSaveButtons()) {  // changes from pin planner not saved to file
    auto answer = QMessageBox::question(
        this, "Pin planner",
        "Some changes are not saved. Do you want to continue?");
    if (answer == QMessageBox::No) return;
  }
  pinAssignment->refresh();
  pinPlannerSaved();
}

void MainWindow::defaultProjectPath() {
  auto defaultPath{QDir::homePath()};
  auto path = m_settings.value(DEFAULT_PROJECT_PATH, defaultPath).toString();
  PathEdit* edit = new PathEdit;
  edit->setText(path);

  QDialog dialog{this};
  dialog.setWindowTitle(defualtProjectPathAction->text());
  auto layout = new QGridLayout;
  dialog.setLayout(layout);
  auto buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  auto ok = buttons->button(QDialogButtonBox::Ok);
  auto cancel = buttons->button(QDialogButtonBox::Cancel);
  connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
  layout->addWidget(edit);
  layout->addWidget(buttons, 1, 0);
  dialog.setModal(true);
  auto result = dialog.exec();
  if (result == QDialog::Accepted) {
    auto newPath = edit->text();
    if (QDir{newPath}.exists()) {
      m_settings.setValue(DEFAULT_PROJECT_PATH, newPath);
      newProjdialog->SetDefaultPath(newPath);
    }
  }
}

void MainWindow::pinPlannerPinName() {
  bool save{false};
  bool clean{false};
  if (isEnableSaveButtons()) {  // changes from pin planner not saved to file
    auto answer = QMessageBox::question(
        this, "Pin planner",
        "Some changes are not saved. Do you want to save them?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (answer == QMessageBox::Yes) save = true;
    if (answer == QMessageBox::No) clean = true;
    if (answer == QMessageBox::Cancel) return;
  }
  QDialog dialog{this};
  dialog.setWindowTitle(pinPlannerPinNameAction->text());
  auto layout = new QGridLayout;
  dialog.setLayout(layout);
  auto buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  auto ok = buttons->button(QDialogButtonBox::Ok);
  auto cancel = buttons->button(QDialogButtonBox::Cancel);
  connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);

  QLabel* label = new QLabel{"Selection type:"};
  layout->addWidget(label);

  QComboBox* combo = new QComboBox{};
  combo->addItem("Use Ball Names");
  combo->addItem("Use Ball IDs");
  if (m_settings.value(PIN_PLANNER_PIN_NAME, false).toBool())
    combo->setCurrentIndex(1);
  layout->addWidget(combo, 0, 1);
  layout->addWidget(buttons, 1, 0, 1, 2);

  dialog.setModal(true);
  auto result = dialog.exec();
  if (result == QDialog::Accepted) {
    if (save)
      if (!saveActionTriggered()) return;
    if (clean) pinPlannerSaved();
    bool useBallId = combo->currentIndex() == 1;
    m_settings.setValue(PIN_PLANNER_PIN_NAME, useBallId);
    auto pinAssignment = findChild<PinAssignmentCreator*>();
    if (pinAssignment) {
      pinAssignment->setUseBallId(useBallId);
    }
  }
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

void MainWindow::popRecentSetting() {
  std::list<QString> projects;

  for (uint i = 0; i < RECENT_PROJECT_COUNT; i++) {
    auto key = RECENT_PROJECT_KEY.arg(QString::number(i));
    auto pr = m_settings.value(key).toString();
    if (pr.isEmpty()) break;
    projects.push_back(pr);
  }

  projects.pop_back();

  uint projCounter = 0;
  for (const auto& pr : projects) {
    m_settings.setValue(RECENT_PROJECT_KEY.arg(QString::number(projCounter++)),
                        pr);
  }
  // clear last
  m_settings.setValue(RECENT_PROJECT_KEY.arg(QString::number(projCounter)),
                      QString{});
  createRecentMenu();
}

void MainWindow::onDesignFilesChanged() {
  QString msg = "Design files changed. Recompile might be needed.";
  setStatusAndProgressText(msg);
}

void MainWindow::onDesignCreated() {
  QString msg = "New Design, compile needed.";
  setStatusAndProgressText(msg);
}

bool MainWindow::saveConstraintFile() {
  auto pinAssignment = findChild<PinAssignmentCreator*>();
  if (!pinAssignment) return false;
  auto constrFile = m_projectManager->getConstrPinFile();
  if (constrFile.empty()) {
    newProjdialog->Reset(Mode::ProjectSettings);
    newProjdialog->SetPageActive(FormIndex::INDEX_ADDCONST);
    newProjdialog->exec();
  }
  constrFile = m_projectManager->getConstrPinFile();
  if (constrFile.empty()) {
    QMessageBox::warning(this, "No *.pin constraint file...",
                         "Please create *.pin constraint file.");
    return false;
  }
  bool rewrite = false;
  auto constraint = QString::fromStdString(constrFile);
  QFile file{constraint};  // TODO @volodymyrk, need to fix
                           // issue with target constraint
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
  pinAssignment->setPinFile(constraint);
  file.open(openFlags);
  QString sdc{pinAssignment->generateSdc()};
  if (rewrite)
    file.resize(0);  // clean content
  else if (!sdc.isEmpty() && file.size() != 0)
    sdc.push_front('\n');  // make sure start with new line
  file.write(sdc.toLatin1());
  file.close();
  return true;
}

void MainWindow::loadFile(const QString& file) {
  if (m_projectFileLoader) {
    m_projectFileLoader->Load(file);
    if (sourcesForm) sourcesForm->InitSourcesForm();
    updatePRViewButton(static_cast<int>(m_compiler->CompilerState()));
    updateTaskTable();
  }
}

void MainWindow::createProgressBar() {
  m_progressWidget = new QWidget;
  m_progressBar = new QProgressBar(m_progressWidget);
  m_progressBar->setFixedHeight(menuBar()->sizeHint().height() - 2);
  m_progressBar->setFormat("%v/%m");
  QHBoxLayout* layout = new QHBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  m_progressWidgetLbl = new QLabel{};
  m_progressWidgetLbl->setFixedHeight(menuBar()->sizeHint().height());
  layout->addWidget(m_progressWidgetLbl);
  layout->addWidget(m_progressBar);
  m_progressWidget->setLayout(layout);
  menuBar()->setCornerWidget(m_progressWidget, Qt::Corner::TopRightCorner);
  m_progressBar->hide();
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
      auto projAction = new QAction(info.absoluteFilePath());
      connect(projAction, &QAction::triggered, this,
              &MainWindow::recentProjectOpen);
      recentMenu->addAction(projAction);
      m_recentProjectsActions.push_back(std::make_pair(projAction, project));
    }
  }
}

void MainWindow::createMenus() {
  recentMenu = new QMenu("Recent Projects");
  recentMenu->menuAction()->setProperty(WELCOME_PAGE_MENU_PROP,
                                        WelcomePageActionVisibility::FULL);
  preferencesMenu = new QMenu{"Preferences"};
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->menuAction()->setProperty(WELCOME_PAGE_MENU_PROP,
                                      WelcomePageActionVisibility::PARTIAL);
  fileMenu->addAction(newAction);
  fileMenu->addAction(openFile);
  fileMenu->addSeparator();
  fileMenu->addAction(newProjectAction);
  fileMenu->addAction(openProjectAction);
  fileMenu->addAction(openExampleAction);
  fileMenu->addAction(closeProjectAction);

  newProjectAction->setProperty(WELCOME_PAGE_MENU_PROP,
                                WelcomePageActionVisibility::FULL);
  openProjectAction->setProperty(WELCOME_PAGE_MENU_PROP,
                                 WelcomePageActionVisibility::FULL);
  openExampleAction->setProperty(WELCOME_PAGE_MENU_PROP,
                                 WelcomePageActionVisibility::FULL);

  fileMenu->addMenu(recentMenu);
  fileMenu->addSeparator();
  fileMenu->addMenu(preferencesMenu);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  projectMenu = menuBar()->addMenu("Project");
  simulationMenu = menuBar()->addMenu("Simulation");
  simulationMenu->addAction(simRtlAction);
  simulationMenu->addAction(simGateAction);
  simulationMenu->addAction(simPnrAction);
  simulationMenu->addAction(simBitstreamAction);
  simulationMenu->setEnabled(false);

  viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction(ipConfiguratorAction);
  viewMenu->addAction(pinAssignmentAction);

  processMenu = menuBar()->addMenu(tr("&Processing"));
  processMenu->addAction(startAction);
  processMenu->addAction(startSimAction);
  processMenu->addAction(stopAction);

  helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction(aboutAction);
  helpMenu->addSeparator();
  helpMenu->addAction(documentationAction);
  helpMenu->addAction(releaseNotesAction);
  helpMenu->addSeparator();
  helpMenu->addAction(manageLicenseAction);
  helpMenu->addAction(licensesAction);
#ifndef PRODUCTION_BUILD
  helpMenu->addAction(compressProjectAction);
#endif

  preferencesMenu->addAction(defualtProjectPathAction);
#ifndef PRODUCTION_BUILD
  preferencesMenu->addAction(pinPlannerPinNameAction);
#endif
  preferencesMenu->addAction(editorSettingsAction);
  preferencesMenu->addAction(showWelcomePageAction);
  preferencesMenu->addAction(stopCompileMessageAction);
  preferencesMenu->addAction(showMessageOnExitAction);

  helpMenu->menuAction()->setProperty(WELCOME_PAGE_MENU_PROP,
                                      WelcomePageActionVisibility::FULL);
}

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->addAction(newAction);

  debugToolBar = addToolBar(tr("Debug"));
  debugToolBar->addAction(startAction);
  debugToolBar->addAction(startSimAction);
  debugToolBar->addAction(stopAction);
#ifndef PRODUCTION_BUILD
  debugToolBar->addAction(programmerAction);
#endif
}

void MainWindow::updateMenusVisibility(bool welcomePageShown) {
  updateActionsVisibility(menuBar()->actions(), welcomePageShown);

  fileToolBar->setVisible(!welcomePageShown);
  debugToolBar->setVisible(!welcomePageShown);
}

void MainWindow::updateActionsVisibility(const QList<QAction*>& actions,
                                         bool welcomePageShown) {
  for (auto menuAction : actions) {
    auto visibilityProp = menuAction->property(WELCOME_PAGE_MENU_PROP);
    if (auto actionMenu = menuAction->menu()) {
      // PARTIAL visibility indicates that some child items should be hidden
      if (visibilityProp.isValid() &&
          visibilityProp.toInt() == WelcomePageActionVisibility::PARTIAL)
        updateActionsVisibility(actionMenu->actions(), welcomePageShown);
    }

    if (welcomePageShown)
      menuAction->setVisible(
          menuAction->property(WELCOME_PAGE_MENU_PROP).isValid());
    else
      menuAction->setVisible(true);
  }
}

void MainWindow::createActions() {
  newAction = new QAction(tr("&New..."), this);
  newAction->setIcon(QIcon(":/images/icon_newfile.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setToolTip(tr("Create a new source file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  openProjectAction = new QAction(tr("Open &Project..."), this);
  openProjectAction->setToolTip(tr("Open a new project"));
  connect(openProjectAction, SIGNAL(triggered()), this,
          SLOT(openProjectDialog()));

  openExampleAction = new QAction(tr("Open &Example Design..."), this);
  openExampleAction->setToolTip(tr("Open example design"));
  connect(openExampleAction, SIGNAL(triggered()), this,
          SLOT(openExampleProject()));

  closeProjectAction = new QAction(tr("&Close Project"), this);
  closeProjectAction->setToolTip(tr("Close current project"));
  connect(closeProjectAction, SIGNAL(triggered()), this, SLOT(closeProject()));

  newProjdialog = new newProjectDialog(this);
  newProjdialog->SetDefaultPath(
      m_settings.value(DEFAULT_PROJECT_PATH, QString{}).toString());
  connect(newProjdialog, SIGNAL(accepted()), this, SLOT(newDialogAccepted()));
  newProjectAction = new QAction(tr("&New Project..."), this);
  newProjectAction->setToolTip(tr("Create a new project"));
  connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProjectDlg()));

  openFile = new QAction(tr("&Open File..."), this);
  openFile->setToolTip(tr("Open file"));
  openFile->setIcon(QIcon(":/images/open-file.png"));
  connect(openFile, SIGNAL(triggered()), this, SLOT(openFileSlot()));

  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setToolTip(tr("Exit the application"));

  startAction = new QAction(tr("Start"), this);
  startAction->setIcon(QIcon(":/images/play.png"));
  startAction->setToolTip(tr("Start compilation tasks"));

  startSimAction = new QAction(tr("Start with Simulation"), this);
  startSimAction->setIcon(QIcon(":/images/playSim.png"));
  startSimAction->setToolTip(tr("Start compilation tasks with simulation"));

  stopAction = new QAction(tr("Stop"), this);
  stopAction->setIcon(QIcon(":/images/stop.png"));
  stopAction->setToolTip(tr("Stop compilation tasks"));
  stopAction->setEnabled(false);
  connect(startAction, &QAction::triggered, this,
          [this]() { startProject(false); });
  connect(startSimAction, &QAction::triggered, this,
          [this]() { startProject(true); });
  connect(stopAction, &QAction::triggered, this, &MainWindow::stopCompilation);

  aboutAction = new QAction(tr("About"), this);
  connect(aboutAction, &QAction::triggered, this, [this]() {
    AboutWidget w(m_projectInfo, GlobalSession->Context()->DataPath(), this);
    w.exec();
  });

  documentationAction = new QAction(tr("User Guide"), this);
  connect(documentationAction, &QAction::triggered, this,
          &MainWindow::documentationClicked);
  releaseNotesAction = new QAction(tr("Release Notes"), this);
  connect(releaseNotesAction, &QAction::triggered, this,
          &MainWindow::releaseNodesClicked);
  licensesAction = new QAction(tr("Licenses"), this);
  connect(licensesAction, &QAction::triggered, this,
          &MainWindow::onShowLicenses);

  manageLicenseAction = new QAction{tr("Manage license..."), this};
  connect(manageLicenseAction, &QAction::triggered, this,
          &MainWindow::manageLicense);

  compressProjectAction = new QAction{tr("Save Diagnostics"), this};
  connect(compressProjectAction, &QAction::triggered, this,
          &MainWindow::compressProject);

  programmerAction = new QAction{tr("Programmer"), this};
  connect(programmerAction, &QAction::triggered, this, [this]() {
    auto programmer = GlobalSession->Context()->ProgrammerGuiPath();
    auto result = FileUtils::ExecuteSystemCommand(
        programmer.string(), {}, nullptr, -1, {}, nullptr, true);
    if (result.code != 0) m_compiler->Message(result.message);
  });

  connect(exitAction, &QAction::triggered, qApp, [this]() {
    if (this->confirmExitProgram()) {
      forceStopCompilation();
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
  connect(ipConfiguratorAction, &QAction::toggled, this,
          &MainWindow::ipConfiguratorActionTriggered);

  showWelcomePageAction = new QAction(tr("Show welcome page"), this);
  showWelcomePageAction->setCheckable(true);
  showWelcomePageAction->setChecked(m_showWelcomePage);
  connect(showWelcomePageAction, &QAction::triggered, this,
          &MainWindow::onShowWelcomePage);

  defualtProjectPathAction = new QAction{tr("Default project path"), this};
  connect(defualtProjectPathAction, &QAction::triggered, this,
          &MainWindow::defaultProjectPath);

  pinPlannerPinNameAction = new QAction{tr("Pin planner pin selection"), this};
  connect(pinPlannerPinNameAction, &QAction::triggered, this,
          &MainWindow::pinPlannerPinName);

  editorSettingsAction = new QAction{tr("3rd party editors..."), this};
  connect(editorSettingsAction, &QAction::triggered, this,
          &MainWindow::editorSettings);

  stopCompileMessageAction =
      new QAction(tr("Show message on stop compilation"), this);
  stopCompileMessageAction->setCheckable(true);
  stopCompileMessageAction->setChecked(m_askStopCompilation);
  connect(stopCompileMessageAction, &QAction::toggled, this,
          &MainWindow::onShowStopMessage);

  showMessageOnExitAction = new QAction(tr("Show message on exit"), this);
  showMessageOnExitAction->setCheckable(true);
  showMessageOnExitAction->setChecked(m_askShowMessageOnExit);
  connect(showMessageOnExitAction, &QAction::toggled, this,
          &MainWindow::onShowMessageOnExit);

  simRtlAction = new QAction(tr("Simulate RTL"), this);
  connect(simRtlAction, &QAction::triggered, this, [this]() {
    GlobalSession->CmdStack()->push_and_exec(new Command("simulate rtl"));
  });
  simPnrAction = new QAction(tr("Simulate PNR"), this);
  connect(simPnrAction, &QAction::triggered, this, [this]() {
    GlobalSession->CmdStack()->push_and_exec(new Command("simulate pnr"));
  });
  simGateAction = new QAction(tr("Simulate Gate"), this);
  connect(simGateAction, &QAction::triggered, this, [this]() {
    GlobalSession->CmdStack()->push_and_exec(new Command("simulate gate"));
  });
  simBitstreamAction = new QAction(tr("Simulate Bitstream"), this);
  connect(simBitstreamAction, &QAction::triggered, this, [this]() {
    GlobalSession->CmdStack()->push_and_exec(new Command("simulate bitstream"));
  });
}

void MainWindow::gui_start(bool showWP) {
  ReShowWindow({});
  if (showWP && m_showWelcomePage) showWelcomePage();
}

void MainWindow::showWelcomePage() {
  clearDockWidgets();
  takeCentralWidget()->hide();  // we can't delete it because of singleton

  newDesignCreated({});
  updateMenusVisibility(true);

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

  updateMenusVisibility(false);

  QDockWidget* sourceDockWidget = new DockWidget(tr("Source"), this);
  sourceDockWidget->setObjectName("sourcedockwidget");
  sourcesForm = new SourcesForm(&m_settings, this);
  connect(
      sourcesForm, &SourcesForm::CloseProject, this,
      [this]() { closeProject(); }, Qt::QueuedConnection);
  connect(sourcesForm, &SourcesForm::OpenProjectSettings, this,
          &MainWindow::openProjectSettings);
  sourceDockWidget->setWidget(sourcesForm);
  addDockWidget(Qt::LeftDockWidgetArea, sourceDockWidget);
  // make fixed max width, because dock widgets layout breaks after file open
  sourceDockWidget->setMaximumWidth(350);
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

  QDockWidget* propertiesDockWidget = new DockWidget(tr("Properties"), this);
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
  connect(sourcesForm, &SourcesForm::IpSimulationRequested, this,
          &MainWindow::handleSimulationIpRequested);
  connect(sourcesForm, &SourcesForm::IpWaveFormRequest, this,
          &MainWindow::handlewaveFormRequested);

  TextEditor* textEditor = new TextEditor(this);
  textEditor->RegisterCommands(GlobalSession);
  textEditor->setObjectName("textEditor");
  connect(sourcesForm, SIGNAL(OpenFile(QString)), textEditor,
          SLOT(SlotOpenFile(QString)));
  connect(sourcesForm, SIGNAL(OpenFileWith(QString, int)), this,
          SLOT(openFileWith(QString, int)));
  connect(textEditor, SIGNAL(CurrentFileChanged(QString)), sourcesForm,
          SLOT(SetCurrentFileItem(QString)));
  connect(textEditor, &TextEditor::FileChanged, this,
          &MainWindow::fileModified);

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
  QDockWidget* consoleDocWidget = new DockWidget(tr("Console"), this);
  consoleDocWidget->setObjectName("consoledocwidget");
  m_dockConsole = consoleDocWidget;

  TclConsoleBuffer* buffer = new TclConsoleBuffer{};
  auto tclConsole = std::make_unique<FOEDAG::TclConsole>(
      m_interpreter->getInterp(), buffer->getStream());
  FOEDAG::TclConsole* c = tclConsole.get();
  TclConsoleWidget* console{nullptr};
  QWidget* w =
      FOEDAG::createConsole(m_interpreter->getInterp(), std::move(tclConsole),
                            buffer, nullptr, &console);
  consoleDocWidget->setWidget(w);
  connect(console, &TclConsoleWidget::linkActivated, this,
          &MainWindow::openFileFromConsole);
  console->addParser(new DummyParser{});
  console->addParser(new TclErrorParser{});
  console->addParser(new FileNameParser{});
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
  connect(tclCommandIntegration, &TclCommandIntegration::showChatGpt, this,
          &MainWindow::chatGpt);
  connect(tclCommandIntegration, &TclCommandIntegration::chatGptStatus, this,
          &MainWindow::chatGptStatus);
  connect(tclCommandIntegration, &TclCommandIntegration::closeDesign, this,
          [this]() { closeProject(true); });
  connect(tclCommandIntegration, &TclCommandIntegration::saveSettingsSignal,
          this, [this]() { saveSettings(); });

  addDockWidget(Qt::BottomDockWidgetArea, consoleDocWidget);

  // QDockWidget* runDockWidget = new DockWidget(tr("Design Runs"), this);
  // runDockWidget->setObjectName("designrundockwidget");
  // RunsForm* runForm = new RunsForm(this);
  // runForm->RegisterCommands(GlobalSession);
  // runDockWidget->setWidget(runForm);
  // tabifyDockWidget(consoleDocWidget, runDockWidget);

  m_messagesDockWidget = PrepareTab(tr("Messages"), "messagesWidget", nullptr,
                                    m_dockConsole, Qt::BottomDockWidgetArea);
  m_messagesDockWidget->hide();

  m_reportsDockWidget = PrepareTab(tr("Reports"), "reportsTreeWidget", nullptr,
                                   m_dockConsole, Qt::BottomDockWidgetArea);
  m_reportsDockWidget->hide();

  // compiler task view
  delete m_taskView;
  m_taskView = prepareCompilerView(m_compiler, &m_taskManager);
  m_taskManager->setDialogProvider(new DialogProvider{this});
  m_taskView->setObjectName("compilerTaskView");
  m_taskView->setParent(this);
  m_taskModel = dynamic_cast<TaskModel*>(m_taskView->model());

  connect(m_taskManager, &TaskManager::progress, this,
          [this](int val, int max, const QString& statusMsg) {
            if (max < 2) {
              // If only 1 task is running, then change progress bar to
              // permaloading since 0/1 will have no visual progress until it
              // finishes
              m_progressBar->setMaximum(0);
              m_progressBar->setValue(0);
            } else {
              m_progressBar->setMaximum(max);
              m_progressBar->setValue(val);
              showMessagesTab();
              showReportsTab();
            }
            m_progressBar->show();
            setStatusAndProgressText(statusMsg);
          });

  connect(m_taskManager, &TaskManager::done, this, [this]() {
    if (!m_progressVisible) m_progressBar->hide();
    m_compiler->finish();
    showMessagesTab();
    showReportsTab();
  });

  connect(m_taskManager, &TaskManager::started, this,
          [this]() { m_progressWidget->show(); });
  connect(m_taskManager, &TaskManager::taskReportCreated, this,
          [this](auto taskName) {
            statusBar()->showMessage(tr("%1 generated").arg(taskName));
          });

  connect(compilerNotifier, &CompilerNotifier::compilerStateChanged, this,
          &MainWindow::updatePRViewButton);
  m_projectFileLoader->registerComponent(
      new TaskManagerComponent{m_taskManager}, ComponentId::TaskManager);
  m_projectFileLoader->registerComponent(new CompilerComponent(m_compiler),
                                         ComponentId::Compiler);
  QDockWidget* taskDockWidget = new DockWidget(tr("Task"), this);
  taskDockWidget->setWidget(m_taskView);
  addDockWidget(Qt::LeftDockWidgetArea, taskDockWidget);

  connect(m_taskManager, &TaskManager::taskStateChanged, this,
          [this]() { startStopButtonsState(); });
  connect(console, &TclConsoleWidget::stateChanged, this,
          [this, console]() { startStopButtonsState(); });

  sourcesForm->InitSourcesForm();
  // runForm->InitRunsForm();
  updatePRViewButton(static_cast<int>(m_compiler->CompilerState()));
  updateViewMenu();
  updateTaskTable();
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
    if (m_compiler) m_compiler->reloadSettings();

    connect(settings, &Settings::sync, this, &MainWindow::saveSetting,
            Qt::UniqueConnection);
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

bool MainWindow::saveActionTriggered() {
  // This blocking need because of Refresh button blinks. Button become visible
  // after file save and then we hide it.
  m_blockRefereshEn = true;
  // here file update will cause generating a signal from file watcher. After
  // this signal we will show refresh button on pin planner. So this signal
  // should be blocked.
  if (saveConstraintFile()) {
    QtUtils::AppendToEventQueue([this]() { m_blockRefereshEn = false; });
    pinPlannerSaved();
    return true;
  } else {
    m_blockRefereshEn = false;
  }
  return false;
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

    PinAssignmentData data;
    data.context = GlobalSession->Context();
    data.pinMapFile =
        QString::fromStdString(m_compiler->PinmapCSVFile().string());
    data.projectPath = m_projectManager->getProjectPath();
    data.target = QString::fromStdString(m_projectManager->getTargetDevice());
    data.pinFile = QString::fromStdString(m_projectManager->getConstrPinFile());
    QFile file{data.pinFile};
    if (file.open(QFile::ReadOnly)) {
      data.commands = QtUtils::StringSplit(QString{file.readAll()}, '\n');
    }
    data.useBallId = m_settings.value(PIN_PLANNER_PIN_NAME, false).toBool();

    PinAssignmentCreator* creator = new PinAssignmentCreator{data, this};
    connect(creator, &PinAssignmentCreator::changed, this,
            &MainWindow::pinAssignmentChanged);

    auto portsDockWidget = PrepareTab(tr("IO Ports"), "portswidget",
                                      creator->GetPortsWidget(), m_dockConsole);
    addPinPlannerRefreshButton(portsDockWidget);
    auto packagePinDockWidget =
        PrepareTab(tr("Package Pins"), "packagepinwidget",
                   creator->GetPackagePinsWidget(), portsDockWidget);
    addPinPlannerRefreshButton(packagePinDockWidget);
    m_pinAssignmentDocks = {portsDockWidget, packagePinDockWidget};
  } else {
    if (isEnableSaveButtons()) {
      auto answer = QMessageBox::question(
          this, "Warning",
          "Pin planner data were modified. Do you want to save it?",
          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
          QMessageBox::Yes);
      if (answer == QMessageBox::No) {
        setEnableSaveButtons(false);
        cleanUpDockWidgets(m_pinAssignmentDocks);
      } else if (answer == QMessageBox::Yes) {
        saveActionTriggered();
        const bool saved = !isEnableSaveButtons();
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
  if (!pinAssignmentAction->isChecked()) {
    // cleanup pin planner
    auto pinAssignment = findChild<PinAssignmentCreator*>();
    if (pinAssignment) delete pinAssignment;
  }
}

void MainWindow::pinAssignmentChanged() {
  for (auto& dock : m_pinAssignmentDocks) {
    if (!dock->windowTitle().endsWith("*")) {
      dock->setWindowTitle(dock->windowTitle() + "*");
    }
  }
  setEnableSaveButtons(true);
}

void MainWindow::ipConfiguratorActionTriggered() {
  if (ipConfiguratorAction->isChecked()) {
    IpConfiguratorCreator creator;
    // Available IPs DockWidget
    auto availableIpsgDockWidget = PrepareTab(tr("IPs"), "availableIpsWidget",
                                              creator.GetAvailableIpsWidget(),
                                              nullptr, Qt::RightDockWidgetArea);
    connect(availableIpsgDockWidget, &DockWidget::closed, ipConfiguratorAction,
            &QAction::trigger);

    m_availableIpsgDockWidget = availableIpsgDockWidget;

    // Get the actual IpCatalogTree
    auto ipsWidgets = m_availableIpsgDockWidget->findChildren<IpCatalogTree*>();
    if (ipsWidgets.count() > 0) {
      m_ipCatalogTree = ipsWidgets[0];

      // Update the IP Config widget when the Available IPs selection changes
      QObject::connect(m_ipCatalogTree, &IpCatalogTree::ipReady, this,
                       &MainWindow::handleIpTreeSelectionChanged);
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
  if (m_ipConfigDockWidget) {
    // remove old config widget
    auto oldWidget = m_ipConfigDockWidget->widget();
    if (oldWidget) delete m_ipConfigDockWidget->widget();
  }
  IpConfigWidget* configWidget =
      new IpConfigWidget(this, ipName, moduleName, paramList);

  // Listen for IpInstance selection changes in the source tree
  QObject::connect(configWidget, &IpConfigWidget::ipInstancesUpdated, this,
                   &MainWindow::updateSourceTree);

  // If dock widget has already been created
  if (m_ipConfigDockWidget) {
    // set new config widget
    m_ipConfigDockWidget->setWidget(configWidget);
    m_ipConfigDockWidget->show();
  } else {  // If dock widget hasn't been created
    // Create and place new dockwidget
    m_ipConfigDockWidget =
        PrepareTab(tr("Configure IP"), "configureIpsWidget", configWidget,
                   nullptr, Qt::RightDockWidgetArea);
  }
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

void MainWindow::handleSimulationIpRequested(const QString& moduleName) {
  Compiler* compiler{};
  IPGenerator* ipGen{};

  if ((compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    auto module = moduleName.toStdString();
    auto [supported, message] = ipGen->IsSimulateIpSupported(module);
    if (!supported) {
      QMessageBox::critical(this, "IP Simulation",
                            QString::fromStdString(message));
      return;
    }
    ipGen->SimulateIp(module);
  }
  updateSourceTree();
}

void MainWindow::handlewaveFormRequested(const QString& moduleName) {
  Compiler* compiler{};
  IPGenerator* ipGen{};

  if ((compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    auto module = moduleName.toStdString();
    auto [supported, message] = ipGen->IsSimulateIpSupported(module);
    const QString title{"View waveform"};
    if (!supported) {
      QMessageBox::critical(this, title, QString::fromStdString(message));
      return;
    }
    auto [ok, mes] = ipGen->OpenWaveForm(module);
    if (!ok) QMessageBox::critical(this, title, QString::fromStdString(mes));
  }
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
#ifndef PRODUCTION_BUILD
  viewMenu->addAction(pinAssignmentAction);
#endif
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

void MainWindow::updateTaskTable() {
  if (!m_taskManager) return;
  const bool isPostSynthPure{m_projectManager->projectType() == PostSynth};
  m_taskManager->task(IP_GENERATE)->setValid(!isPostSynthPure);
  m_taskManager->task(ANALYSIS)->setValid(!isPostSynthPure);
  m_taskManager->task(SIMULATE_RTL)->setValid(!isPostSynthPure);
  m_taskManager->task(SYNTHESIS)->setValid(!isPostSynthPure);
  if (m_taskView && m_taskModel) {
    for (auto taskId : {IP_GENERATE, ANALYSIS, ANALYSIS_CLEAN, SIMULATE_RTL,
                        SIMULATE_RTL_CLEAN, SIMULATE_RTL_SETTINGS, SYNTHESIS,
                        SYNTHESIS_CLEAN, SYNTHESIS_SETTINGS}) {
      int row = m_taskModel->ToRowIndex(taskId);
      m_taskView->setRowHidden(row, isPostSynthPure);
    }
    for (auto taskId : {SIMULATE_BITSTREAM, SIMULATE_BITSTREAM_CLEAN,
                        SIMULATE_BITSTREAM_SETTINGS, POWER, POWER_CLEAN}) {
      int row = m_taskModel->ToRowIndex(taskId);
      m_taskView->setRowHidden(row, true);
    }
  }
  m_taskManager->task(SIMULATE_BITSTREAM)->setValid(false);
  m_taskManager->task(SIMULATE_BITSTREAM_CLEAN)->setValid(false);
  m_taskManager->task(POWER)->setValid(false);
  m_taskManager->task(POWER_CLEAN)->setValid(false);
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
  // Update watcher files
  DesignFileWatcher::Instance()->updateDesignFileWatchers(m_projectManager);
}

void MainWindow::saveWelcomePageConfig() {
  m_settings.setValue(SHOW_WELCOMEPAGE_KEY, m_showWelcomePage);
  showWelcomePageAction->setChecked(m_showWelcomePage);
}

void MainWindow::recentProjectOpen() {
  if (!lastProjectClosed()) return;
  auto action = qobject_cast<QAction*>(sender());
  auto project = std::find_if(m_recentProjectsActions.cbegin(),
                              m_recentProjectsActions.cend(),
                              [action](const std::pair<QAction*, QString>& p) {
                                return p.first == action;
                              });
  if (project != m_recentProjectsActions.end()) {
    const QString name = project->second;
    if (!name.isEmpty()) {
      if (!QFileInfo::exists(name)) {
        QMessageBox::critical(this, name,
                              QString{"Project %1 does not exist."}.arg(name));
        saveToRecentSettings(name);  // push on top
        popRecentSetting();
        return;
      }
      openProject(name, false, false);
    }
  }
}

void MainWindow::openProjectSettings() {
  newProjdialog->Reset(Mode::ProjectSettings);
  newProjdialog->open();
}

bool MainWindow::confirmCloseProject() {
  return (QMessageBox::question(
              this, "Close Project?",
              tr("Are you sure you want to close your project?\n"),
              QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes);
}
bool MainWindow::confirmExitProgram() {
  if (!lastProjectClosed()) return false;
  if (!m_askShowMessageOnExit) return true;
  return (QMessageBox::question(
              this, "Exit Program?", tr("Are you sure you want to exit?\n"),
              QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes);
}

void MainWindow::setVisibleRefreshButtons(bool visible) {
  for (auto dock : m_pinAssignmentDocks) {
    auto button = dock->findChild<QPushButton*>("refreshButton");
    if (button) button->setVisible(visible);
  }
}

void MainWindow::pinPlannerSaved() {
  setEnableSaveButtons(false);
  for (auto& dock : m_pinAssignmentDocks) {
    if (dock->windowTitle().endsWith("*")) {
      dock->setWindowTitle(
          dock->windowTitle().mid(0, dock->windowTitle().size() - 1));
    }
  }
  QtUtils::AppendToEventQueue([this]() { setVisibleRefreshButtons(false); });
}
void MainWindow::setStatusAndProgressText(const QString& text) {
  m_progressWidgetLbl->setText("<strong>STATUS</strong> " + text);
  m_progressWidgetLbl->setVisible(!text.isEmpty());
  menuBar()->adjustSize();

  // Duplicate status in the actual window status bar
  statusBar()->showMessage(text);
}

void MainWindow::saveSettings() {
  if (m_taskManager) {
    const auto tasks = m_taskManager->tasks();
    for (const Task* const t : tasks) {
      if (!t->settingsKey().isEmpty()) {
        saveSetting(t->settingsKey());
      }
    }
  }
}

void MainWindow::saveSetting(const QString& setting) {
  auto d = FOEDAG::createTaskDialog(setting);
  if (d) {
    QDialogButtonBox* btnBox = d->findChild<QDialogButtonBox*>(DlgBtnBoxName);
    if (btnBox) btnBox->button(QDialogButtonBox::Ok)->click();
  }
}

void MainWindow::openFileFromConsole(const ErrorInfo& eInfo) {
  QString file{eInfo.file};
  QFileInfo info{eInfo.file};
  if (!info.exists()) {  // try to search in the project folder
    info.setFile(m_projectManager->getProjectPath(), file);
    if (info.exists()) file = info.absoluteFilePath();
  }
  auto textEditor = findChild<TextEditor*>("textEditor");
  if (textEditor) textEditor->SlotOpenFileWithLine(file, eInfo.line.toInt());
}

void MainWindow::manageLicense() {
  auto path = GlobalSession->Context()->DataPath() / "etc" / "config.json";
  auto licPath{Settings::Config(path, "general", "license-path")};
  LicenseManagerWidget license{licPath, this};
  license.exec();
}

void MainWindow::compressProject() {
  CompressProject cProject{m_projectManager->projectPath(), this};
  cProject.exec();
}

void MainWindow::documentationClicked() {
  auto path = GlobalSession->Context()->DataPath() / "etc" / "config.json";
  auto userGuide{Settings::Config(path, "general", "user-guide")};
  if (!userGuide.isEmpty()) QDesktopServices::openUrl(userGuide);
}

void MainWindow::releaseNodesClicked() {
  auto path = GlobalSession->Context()->DataPath() / "etc" / "config.json";
  auto releaseNotes{Settings::Config(path, "general", "release-notes")};
  if (!releaseNotes.isEmpty()) QDesktopServices::openUrl(releaseNotes);
}

void MainWindow::openFileWith(QString file, int editor) {
  auto editorStr =
      m_settings.value(EDITOR_KEY.arg(QString::number(editor))).toString();
  auto editorLine = QtUtils::StringSplit(editorStr, ';');
  if (editorLine.count() > 1) {
    auto command = editorLine.last();
    auto commandArgs = QtUtils::StringSplit(command, ' ');
    if (!commandArgs.isEmpty()) {
      auto commandName = commandArgs.takeFirst().toStdString();
      StringVector args{};
      for (const auto& c : commandArgs) args.push_back(c.toStdString());
      args.push_back(file.toStdString());
      FileUtils::ExecuteSystemCommand(commandName, args, nullptr, -1, {},
                                      nullptr, true);
    }
  }
}

void MainWindow::editorSettings() {
  EditorSettings setting{this};
  for (int i = 0; i < setting.editor().size(); i++) {
    auto value =
        m_settings.value(EDITOR_KEY.arg(QString::number(i))).toString();
    auto valueSplitted = QtUtils::StringSplit(value, ';');
    if (valueSplitted.size() == 2)
      setting.setEditor(
          std::make_pair(valueSplitted.first(), valueSplitted.last()), i);
  }
  auto accepted = setting.exec();
  if (accepted) {
    const auto& editors = setting.editor();
    for (int i = 0; i < editors.size(); i++) {
      m_settings.setValue(
          EDITOR_KEY.arg(QString::number(i)),
          QString{"%1;%2"}.arg(editors.at(i).first, editors.at(i).second));
    }
  }
}

void MainWindow::setEnableSaveButtons(bool enable) {
  for (const auto& b : m_saveButtons) b->setEnabled(enable);
}

bool MainWindow::isEnableSaveButtons() const {
  if (!m_saveButtons.isEmpty()) return m_saveButtons.first()->isEnabled();
  return false;
}

bool MainWindow::lastProjectClosed() {
  if (!m_projectManager) return true;
  if (m_projectManager && !m_projectManager->HasDesign()) return true;
  return CloseOpenedTabs();
}

void MainWindow::onShowWelcomePage(bool show) {
  m_showWelcomePage = show;
  saveWelcomePageConfig();
}

void MainWindow::startProject(bool simulation) {
  m_progressWidget->show();
  m_compiler->start();
  m_taskManager->startAll(simulation);
}

void MainWindow::onShowStopMessage(bool showStopCompilationMsg) {
  m_askStopCompilation = showStopCompilationMsg;
  m_settings.setValue(SHOW_STOP_COMPILATION_MESSAGE_KEY, m_askStopCompilation);
}

void MainWindow::onShowMessageOnExit(bool showMessage) {
  m_askShowMessageOnExit = showMessage;
  m_settings.setValue(SHOW_MESSAGE_ON_EXIT_KEY, m_askShowMessageOnExit);
}

void MainWindow::onShowLicenses() {
  auto licensePath = GlobalSession->Context()->DataPath() / LICENSES_DIR;
  if (!FileUtils::FileExists(licensePath)) return;

  LicenseViewer viewer(licensePath, m_projectInfo.name, this);
  centerWidget(viewer);
  viewer.exec();
}
