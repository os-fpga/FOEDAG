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

  private: /* Menu bar builders */
    void createMenus();
    void createActions();

  private: /* Objects/Widgets under the main window */
    /* Menu bar objects */
    QMenu* fileMenu;
    QAction* exitAction;
};

#endif
