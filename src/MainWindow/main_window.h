#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class QAction;
class QLabel;

/** Main window of the program */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public: /*-- Constructor --*/
  MainWindow();

 private slots: /* slots */
  void newFile();

 private: /* Menu bar builders */
  void createMenus();
  void createToolBars();
  void createActions();

 private: /* Objects/Widgets under the main window */
  /* Menu bar objects */
  QMenu* fileMenu;
  QAction* newAction;
  QAction* exitAction;

  /* Tool bar objects */
  QToolBar* fileToolBar;
};

#endif
