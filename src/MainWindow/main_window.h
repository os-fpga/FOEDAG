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

#include "NewProject/new_project_dialog.h"
#include "TopLevelInterface.h"

class QAction;
class QLabel;

namespace FOEDAG {

class TclInterpreter;
/** Main window of the program */
class MainWindow : public QMainWindow, public TopLevelInterface {
  Q_OBJECT

 public: /*-- Constructor --*/
  MainWindow(TclInterpreter* interp);

  void Tcl_NewProject(int argc, const char* argv[]);
  newProjectDialog* NewProjectDialog() { return newProjdialog; }
  void MainWindowTitle(const std::string& name) { mainWindowName = name; }
 private slots: /* slots */
  void newFile();
  void newProjectDlg();
  void openProject();
  void openFileSlot();

 private: /* Menu bar builders */
  void createMenus();
  void createToolBars();
  void createActions();
  void gui_start() override;

  void ReShowWindow(QString strProject);
  void clearDockWidgets();

 private: /* Objects/Widgets under the main window */
  /* Menu bar objects */
  QMenu* fileMenu = nullptr;
  QAction* newAction = nullptr;
  QAction* newProjectAction = nullptr;
  QAction* openProjectAction = nullptr;
  QAction* exitAction = nullptr;
  QAction* openFile = nullptr;
  newProjectDialog* newProjdialog = nullptr;
  /* Tool bar objects */
  QToolBar* fileToolBar = nullptr;

  TclInterpreter* m_interpreter = nullptr;
  std::string mainWindowName = "FOEDAG";

};

}  // namespace FOEDAG

#endif
