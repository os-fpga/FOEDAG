#include "main_window.h"

#include <QTextStream>
#include <QtWidgets>
#include <fstream>

MainWindow::MainWindow() {
  /* Window settings */
  setWindowTitle(tr("FOEDAG"));
  resize(350, 250);

  /* Create actions that can be added to menu/tool bars */
  createActions();

  /* Create menu bars */
  createMenus();

  /* Create tool bars */
  createToolBars();

  /* Create status bar */
  statusBar();

  /* Add dummy text editors */
  QTextEdit* editor1 = new QTextEdit;
  QTextEdit* editor2 = new QTextEdit;
  QTextEdit* editor3 = new QTextEdit;

  /* Add widgets into floorplanning */
  QSplitter* leftSplitter = new QSplitter(Qt::Horizontal);
  leftSplitter->addWidget(editor1);
  leftSplitter->setStretchFactor(1, 1);

  QDockWidget* texteditorDockWidget = new QDockWidget(tr("Text Editor"));
  texteditorDockWidget->setObjectName("texteditorDockWidget");
  texteditorDockWidget->setWidget(editor2);
  texteditorDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
                                        Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, texteditorDockWidget);

  QSplitter* mainSplitter = new QSplitter(Qt::Vertical);
  mainSplitter->addWidget(leftSplitter);
  mainSplitter->addWidget(editor3);
  mainSplitter->setStretchFactor(1, 1);

  setCentralWidget(mainSplitter);

  statusBar()->showMessage("Ready");
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

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->addAction(newAction);
}

void MainWindow::createActions() {
  newAction = new QAction(tr("&New"), this);
  newAction->setIcon(QIcon(":/images/icon_newfile.png"));
  newAction->setShortcut(QKeySequence::New);
  newAction->setStatusTip(tr("Create a new source file"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(tr("Ctrl+Q"));
  exitAction->setStatusTip(tr("Exit the application"));
  connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
}
