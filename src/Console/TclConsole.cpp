#include "TclConsole.h"

#include <iostream>

#include "ConsoleDefines.h"
#include "FileInfo.h"

namespace FOEDAG {

TclConsole::TclConsole(TclInterp *interpreter, std::ostream &out,
                       QObject *parent)
    : ConsoleInterface(parent),
      m_tclWorker(new TclWorker{interpreter, out, parent}),
      m_out(out) {
  connect(this, &TclConsole::sendCommand, m_tclWorker, &TclWorker::runCommand);
  //  connect(this, &TclController::abort_, &m_tclWorker, &TclWorker::abort,
  //          Qt::DirectConnection);
  connect(m_tclWorker, &TclWorker::tclFinished, this, &TclConsole::done);
}

void TclConsole::registerInterpreter(TclInterp *interpreter) {
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
  prefix = QString();
  int i = cmd.lastIndexOf(QRegExp("[\[{;\n]"));
  if (i != -1) {
    commandToComplete = cmd.right(cmd.length() - i - 1);
    prefix = cmd.left(i + 1);
  }
  auto interp = m_tclWorker->getInterpreter();
  int res = Tcl_Eval(
      interp, qPrintable("info commands [join {" + commandToComplete + "*}]"));
  if (res == TCL_OK) {
    // Get the string result of the executed command
    QString result = Tcl_GetString(Tcl_GetObjResult(interp));
    if (!result.isEmpty()) {
      suggestions = result.split(" ");
    }
  }

  if (suggestions.isEmpty()) {
    suggestions +=
        getFilesCompletion(m_tclWorker->getInterpreter(), cmd, prefix);
  }

  return suggestions;
}

bool TclConsole::isCommandComplete(const QString &command) {
  return Tcl_CommandComplete(qPrintable(command));
}

void TclConsole::abort() {
  QMetaObject::invokeMethod(m_tclWorker, "abort", Qt::DirectConnection);
}

void TclConsole::tclFinished() {
  //
}

QStringList TclConsole::getFilesCompletion(TclInterp *interpreter,
                                           const QString &cmd,
                                           QString &prefix) const {
  QStringList suggestions;
  if (cmd.startsWith("source ")) {
    int res = Tcl_Eval(interpreter, qPrintable("pwd"));
    if (res == TCL_OK) {
      // Get the string result of the executed command
      QString currPath = Tcl_GetString(Tcl_GetObjResult(interpreter));
      currPath += FileInfo::separator();
      auto args = cmd.split(" ");
      if (args.count() > 1) {
        currPath += args.at(1);

        auto filter =
            currPath.mid(currPath.lastIndexOf(FileInfo::separator()) + 1);

        currPath = currPath.mid(0, currPath.lastIndexOf(FileInfo::separator()));
        auto files = FileInfo::getFileList(currPath, {"*.tcl"});
        for (const auto &fileName : files) {
          if (fileName.startsWith(filter)) suggestions.append(fileName);
        }

        if (!suggestions.isEmpty()) {
          auto cutSeperator =
              cmd.mid(0, cmd.lastIndexOf(FileInfo::separator()) + 1);
          if (cutSeperator.isEmpty())
            cutSeperator = cmd.mid(0, cmd.lastIndexOf(" ") + 1);
          prefix = cutSeperator.isEmpty() ? cmd : cutSeperator;
        }
      }
    }
  }
  return suggestions;
}

}  // namespace FOEDAG
