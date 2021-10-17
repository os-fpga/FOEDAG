#include <fstream>
#include <QTextStream>
#include <QtWidgets>

#include "main_window.h"

MainWindow::MainWindow() {
  createActions();
  createMenus();
}

void MainWindow::newFile() {
  QTextStream out(stdout);
  out << "New file is requested" << endl;
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);
}

void MainWindow::createActions() {
  newAction = new QAction(tr("&New"), this);
  //newAction->setIcon(QIcon(":images/new.png"));
  newAction->setShortcut(QKeySequence::New); 
  newAction->setStatusTip(tr("Create a new source file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setStatusTip(tr("Exit the application"));
  connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
}
