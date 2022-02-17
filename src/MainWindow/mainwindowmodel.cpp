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
    : m_interpreter(interp) {
  setStatusBarMessage("Ready");
}

void MainWindowModel::Tcl_NewProject(int argc, const char* argv[]) {
  ProjectManager* projectManager = new ProjectManager(this);
  projectManager->Tcl_CreateProject(argc, argv);
}

QStringList MainWindowModel::fileDialogFilters() {
  const QString _FILTER_VERILOG("Verilog HDL Files(*.v)");
  const QString _FILTER_VHDL("VHDL Files(*.vhd)");
  const QString _FILTER_TCL("Tcl Script Files(*.tcl)");
  const QString _FILTER_CONSTR("Synopsys Design Constraints Files(*.sdc)");
  const QString _FILTER_ALL("All Files(*.*)");
  QStringList filters{_FILTER_VERILOG, _FILTER_VHDL, _FILTER_TCL,
                      _FILTER_CONSTR, _FILTER_ALL};
  return filters;
}

bool MainWindowModel::isVisible() const { return m_isVisible; }

void MainWindowModel::setIsVisible(bool newIsVisible) {
  if (m_isVisible == newIsVisible) return;

  m_isVisible = newIsVisible;
  emit visibleChanged();
}

const QString& MainWindowModel::statusBarMessage() const {
  return m_statusBarMessage;
}

void MainWindowModel::setStatusBarMessage(const QString& newStatusBarMessage) {
  if (m_statusBarMessage == newStatusBarMessage) return;

  m_statusBarMessage = newStatusBarMessage;
  emit statusBarMessageChanged();
}

bool MainWindowModel::createNewFile(const QUrl& fileName) {
  QFile file(fileName.toString());
  if (file.exists()) return false;

  if (!file.open(QFile::WriteOnly | QFile::Text)) return true;

  file.close();
  return false;
}
