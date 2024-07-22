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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QSettings>

#include "Main/AboutWidget.h"
#include "NewProject/new_project_dialog.h"
#include "TopLevelInterface.h"
class QAction;
class QLabel;
class QProgressBar;
class QStackedWidget;

namespace FOEDAG {

class Session;
class TclInterpreter;
class ProjectFileLoader;
/** Main window of the program */
class MainWindow : public QMainWindow, public TopLevelInterface {
  Q_OBJECT

 public: /*-- Constructor --*/
  MainWindow(Session* session);

  void Tcl_NewProject(int argc, const char* argv[]);
  newProjectDialog* NewProjectDialog() { return newProjdialog; }
  void Info(const ProjectInfo& info);
  ProjectInfo Info() const;
  void SetWindowTitle(const QString& filename, const QString& project,
                      QString& projectInfo);
  void CloseOpenedTabs();
  void ProgressVisible(bool visible) override;

  void openProject(const QString& project, bool delayedOpen, bool run) override;
  bool isRunning() const override;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void ScriptFinished() override;

 private slots: /* slots */
  void newFile();
  void newProjectDlg();
  void openProjectDialog(const QString& dir = QString{});
  void openExampleProject();
  bool closeProject(bool force = false);
  void openFileSlot();
  void newDesignCreated(const QString& design);
  void reloadSettings();
  void updatePRViewButton(int state);
  bool saveActionTriggered();
  void pinAssignmentActionTriggered();
  void pinAssignmentChanged();
  void ipConfiguratorActionTriggered();
  void newDialogAccepted();
  void recentProjectOpen();
  void openProjectSettings();
  void slotTabChanged(int index);
  void handleProjectOpened();
  void onShowWelcomePage(bool show);
  void onRunProjectRequested(const QString& project);
  void startProject(bool simulation);
  void onShowStopMessage(bool showStopCompilationMsg);
  void bitstreamEnable(bool enable);
  void onShowLicenses();
  void stopCompilation();
  void forceStopCompilation();
  void showMessagesTab();
  void showReportsTab();
  void fileModified(const QString& file);
  void refreshPinPlanner();
  void defaultProjectPath();
  void pinPlannerPinName();
  void onDesignFilesChanged();
  void onDesignCreated();

 public slots:
  void updateSourceTree();
  void handleIpTreeSelectionChanged();
  void handleIpReConfigRequested(const QString& ipName,
                                 const QString& moduleName,
                                 const QStringList& paramList);
  void handleRemoveIpRequested(const QString& moduleName);
  void handleDeleteIpRequested(const QString& moduleName);
  void resetIps();

 signals:
  void projectOpened();
  void runProjectRequested(const QString& project);

 private: /* Menu bar builders */
  void updateViewMenu();
  void updateTaskTable();
  void updateBitstream();
  void createMenus();
  void createToolBars();
  void createActions();
  void createProgressBar();
  void createRecentMenu();
  void connectProjectManager();
  void gui_start(bool showWP) override;

  void ReShowWindow(QString strProject);
  void clearDockWidgets();
  void startStopButtonsState();
  void loadFile(const QString& file);
  QDockWidget* PrepareTab(const QString& name, const QString& objName,
                          QWidget* widget, QDockWidget* tabToAdd,
                          Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

  void addPinPlannerRefreshButton(QDockWidget* dock);

  void cleanUpDockWidgets(std::vector<QDockWidget*>& dockWidgets);
  void saveToRecentSettings(const QString& project);
  void popRecentSetting();

  // Shows or hides menus depending on welcome page visibility
  void updateMenusVisibility(bool welcomePageShown);
  // Recursively goes through actions and their children and hides/shows them
  // depending on set property
  void updateActionsVisibility(const QList<QAction*>& actions,
                               bool welcomePageShown);
  void showWelcomePage();

  bool saveConstraintFile();
  // Creates the new file in a working directory holding welcome page
  // configuration
  void saveWelcomePageConfig();
  void replaceIpConfigDockWidget(QWidget* widget);
  bool confirmCloseProject();
  bool confirmExitProgram();
  void setVisibleRefreshButtons(bool visible);
  void pinPlannerSaved();
  void setStatusAndProgressText(const QString& text);
  void saveSettings();
  void showEULADialog();

 private: /* Objects/Widgets under the main window */
  /* Enum holding different states of actions visibility on the welcome page.
     Actually we can just have a single state - visible. But in this case each
     action would have to be inspected. This enum allows to indicate the menu
     as fully visible and not inspect all its sub actions.*/
  enum WelcomePageActionVisibility {
    PARTIAL,  // Menu is visible but some of its child actions aren't
    FULL      // Menu and all its child actions are visible
  };
  bool m_showWelcomePage{true};
  /* Menu bar objects */
  QMenu* fileMenu = nullptr;
  QMenu* processMenu = nullptr;
  QMenu* helpMenu = nullptr;
  QMenu* viewMenu = nullptr;
  QMenu* recentMenu = nullptr;
  QMenu* preferencesMenu = nullptr;
  QMenu* projectMenu = nullptr;
  QMenu* simulationMenu = nullptr;
  QAction* newAction = nullptr;
  QAction* newProjectAction = nullptr;
  QAction* openProjectAction = nullptr;
  QAction* openExampleAction = nullptr;
  QAction* closeProjectAction = nullptr;
  QAction* exitAction = nullptr;
  QAction* openFile = nullptr;
  QAction* startAction = nullptr;
  QAction* startSimAction = nullptr;
  QAction* stopAction = nullptr;
  QAction* aboutAction = nullptr;
  QAction* documentationAction = nullptr;
  QAction* releaseNotesAction = nullptr;
  QAction* licensesAction = nullptr;
  QAction* pinAssignmentAction = nullptr;
  QAction* ipConfiguratorAction = nullptr;
  QAction* saveAction = nullptr;
  QAction* showWelcomePageAction = nullptr;
  QAction* stopCompileMessageAction = nullptr;
  QAction* bitstreamAction = nullptr;
  QAction* simRtlAction = nullptr;
  QAction* simGateAction = nullptr;
  QAction* simPnrAction = nullptr;
  QAction* simBitstreamAction = nullptr;
  QAction* defualtProjectPathAction = nullptr;
  QAction* pinPlannerPinNameAction = nullptr;
  std::vector<std::pair<QAction*, QString>> m_recentProjectsActions;
  newProjectDialog* newProjdialog = nullptr;
  /* Tool bar objects */
  QToolBar* fileToolBar = nullptr;
  QToolBar* debugToolBar = nullptr;
  QToolBar* saveToolBar = nullptr;
  Session* m_session = nullptr;
  TclInterpreter* m_interpreter = nullptr;
  std::string mainWindowName = "AURORA";
  ProjectInfo m_projectInfo;
  class TaskManager* m_taskManager{nullptr};
  class Compiler* m_compiler{nullptr};
  class TclConsoleWidget* m_console{nullptr};
  class ProjectManager* m_projectManager{nullptr};
  class ProjectFileLoader* m_projectFileLoader{nullptr};
  class SourcesForm* sourcesForm{nullptr};
  class IpCatalogTree* m_ipCatalogTree{nullptr};
  QWidget* m_progressWidget{nullptr};
  QLabel* m_progressWidgetLbl{nullptr};
  QProgressBar* m_progressBar{nullptr};
  QDockWidget* m_dockConsole{nullptr};
  std::vector<QDockWidget*> m_pinAssignmentDocks;
  QDockWidget* m_ipConfigDockWidget{nullptr};
  QDockWidget* m_availableIpsgDockWidget{nullptr};
  QDockWidget* m_messagesDockWidget{nullptr};
  QDockWidget* m_reportsDockWidget{nullptr};
  QSettings m_settings;
  bool m_progressVisible{false};
  bool m_askStopCompilation{true};
  bool m_blockRefereshEn{false};
  QTableView* m_taskView{nullptr};
  class TaskModel* m_taskModel{nullptr};
  QDialog* m_EULADialog;
  QStackedWidget* licensesStackedWidget;
  QPushButton* m_EULADialogBackButton;
  QPushButton* m_EULADialogNextButton;
  QPushButton* m_EULADialogDeclineButton;
  QPushButton* m_EULADialogAcceptButton;
};

}  // namespace FOEDAG

#endif
