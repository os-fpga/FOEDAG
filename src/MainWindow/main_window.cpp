#include <QtWidgets>

#include "main_window.h"

MainWindow::MainWindow() {
  createActions();
  createMenus();
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(exitAction);
}

void MainWindow::createActions() {
  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setStatusTip(tr("Exit the application"));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(closeAllWindows()));
}
