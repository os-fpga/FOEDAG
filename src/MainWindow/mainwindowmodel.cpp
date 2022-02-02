#include "mainwindowmodel.h"

#include <fstream>

#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "DesignRuns/runs_form.h"
#include "Main/Foedag.h"
#include "NewFile/new_file.h"
#include "NewProject/new_project_dialog.h"
#include "ProjNavigator/sources_form.h"
#include "TextEditor/text_editor.h"

using namespace FOEDAG;

MainWindowModel::MainWindowModel(TclInterpreter* interp, QObject* parent)
    : QObject{parent}, m_interpreter{interp} {}

void MainWindow::Tcl_NewProject(int argc, const char* argv[]) {
  ProjectManager* projectManager = new ProjectManager(this);
  projectManager->Tcl_CreateProject(argc, argv);
}

// void MainWindow::newFile() {
//  //  QTextStream out(stdout);
//  //  out << "New file is requested\n";
//  NewFile* newfile = new NewFile(this);
//  newfile->StartNewFile();
//}

// void MainWindow::newProjectDlg() {
//  newProjectDialog* m_dialog = new newProjectDialog(this);
//  int ret = m_dialog->exec();
//  m_dialog->close();
//  if (ret) {
//    QString strproject = m_dialog->getProject();
//    newProjectAction->setEnabled(false);
//    ReShowWindow(strproject);
//  }
//}

// void MainWindow::openProject() {
//  QString fileName = "";
//  fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), "",
//                                          "FOEDAG Project File(*.ospr)");
//  if ("" != fileName) {
//    ReShowWindow(fileName);
//  }
//}
