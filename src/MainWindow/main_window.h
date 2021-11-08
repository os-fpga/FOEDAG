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

class QAction;
class QLabel;

namespace FOEDAG {

/** Main window of the program */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public: /*-- Constructor --*/
  MainWindow();

 private slots: /* slots */
  void newFile();
  void newProjectDlg();

 private: /* Menu bar builders */
  void createMenus();
  void createToolBars();
  void createActions();

 private: /* Objects/Widgets under the main window */
  /* Menu bar objects */
  QMenu* fileMenu;
  QAction* newAction;
  QAction* newProjectAction;
  QAction* exitAction;

  /* Tool bar objects */
  QToolBar* fileToolBar;
};

}  // namespace FOEDAG

#endif
