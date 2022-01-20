#include "TclConsole.h"

#include <iostream>

#include "Tcl/TclInterpreter.h"

TclConsole::TclConsole(FOEDAG::TclInterpreter *interpreter, std::ostream &out,
                       QObject *parent)
    : ConsoleInterface(parent),
      m_tclWorker(new TclWorker{interpreter, out, parent}),
      m_out(out) {
  connect(this, &TclConsole::sendCommand, m_tclWorker, &TclWorker::runCommand);
  //  connect(this, &TclController::abort_, &m_tclWorker, &TclWorker::abort,
  //          Qt::DirectConnection);
  connect(m_tclWorker, &TclWorker::tclFinished, this, &TclConsole::done);
}

void TclConsole::registerInterpreter(FOEDAG::TclInterpreter *interpreter) {
  m_tclWorkers.push_back(new TclWorker{interpreter, m_out});
}

TclConsole::~TclConsole() {
  qDeleteAll(m_tclWorkers);
  if (m_tclWorker->isRunning()) m_tclWorker->quit();
  m_tclWorker->wait();
}

void TclConsole::run(const QString &command) {
  m_tclWorker->runCommand(command);
  m_tclWorker->start();
}

int TclConsole::returnCode() const { return m_tclWorker->returnCode(); }

QStringList TclConsole::suggestCommand(const QString &cmd, QString &prefix) {
  QString commandToComplete = cmd;
  QStringList suggestions;
  prefix = "";
  int i = cmd.lastIndexOf(QRegExp("[\[{;\n]"));
  if (i != -1) {
    commandToComplete = cmd.right(cmd.length() - i - 1);
    prefix = cmd.left(i + 1);
  }
  auto interp = m_tclWorker->getInterpreter()->getInterp();
  int res = Tcl_Eval(
      interp, qPrintable("info commands [join {" + commandToComplete + "*}]"));
  if (!res) {
    // Get the string result of the executed command
    QString result = Tcl_GetString(Tcl_GetObjResult(interp));
    if (!result.isEmpty()) {
      suggestions = result.split(" ");
    }
  }
  return suggestions;
}

bool TclConsole::isCommandComplete(const QString &command) {
  return Tcl_CommandComplete(qPrintable(command));
}

void TclConsole::abort() {
  //
}

void TclConsole::tclFinished() {
  //
}
