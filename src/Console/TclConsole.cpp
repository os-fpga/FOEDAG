#include "TclConsole.h"

#include <QDir>
#include <iostream>

#include "ConsoleDefines.h"
#include "FileInfo.h"

namespace FOEDAG {

TclConsole::TclConsole(TclInterp *interpreter, std::ostream &out,
                       QObject *parent)
    : ConsoleInterface(parent),
      m_tclWorker(new TclWorker{interpreter, out, &std::cerr, false, parent}),
      m_out(out) {
  connect(m_tclWorker, &TclWorker::tclFinished, this,
          &TclConsole::tclWorkerFinished);
}

void TclConsole::registerInterpreter(TclInterp *interpreter) {
  m_tclWorkers.push_back(new TclWorker{interpreter, m_out, m_tclWorker->err()});
}

TclConsole::~TclConsole() { qDeleteAll(m_tclWorkers); }

void TclConsole::run(const QString &command) {
  m_tclWorker->runCommand(command);
}

QStringList TclConsole::suggestCommand(const QString &cmd, QString &prefix) {
  QString commandToComplete = cmd;
  QStringList suggestions;
  prefix = QString();
  int i = cmd.lastIndexOf(QRegExp("[[{;\n]"));
  if (i != -1) {
    commandToComplete = cmd.right(cmd.length() - i - 1);
    prefix = cmd.left(i + 1);
  }
  auto interp = m_tclWorker->getInterpreter();
  int res = TclEval(
      interp, qPrintable("info commands [join {" + commandToComplete + "*}]"));
  if (res == TCL_OK) {
    // Get the string result of the executed command
    QString result = TclGetString(Tcl_GetObjResult(interp));
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
  m_tclWorker->abort();
  emit aborted();
}

void TclConsole::setErrorStream(std::ostream *err) {
  m_tclWorker->setErrStream(err);
}

void TclConsole::setTclCommandInProggress(bool inProgress) {
  m_commandInProggress = inProgress;
  if (m_commandInProggress) emit inProgressState();
  tclWorkerFinished();
}

void TclConsole::tclWorkerFinished() {
  if (!m_commandInProggress) emit done();
}

QStringList TclConsole::getFilesCompletion(TclInterp *interpreter,
                                           const QString &cmd,
                                           QString &prefix) const {
  QStringList suggestions;
  if (cmd.startsWith("source ")) {
    int res = TclEval(interpreter, qPrintable("pwd"));
    if (res == TCL_OK) {
      // Get the string result of the executed command
      QString currPath = TclGetString(Tcl_GetObjResult(interpreter));
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
  } else if (cmd.startsWith("cd ")) {
    const int res = TclEval(interpreter, qPrintable("pwd"));
    if (res == TCL_OK) {
      const QString userInput = cmd.right(cmd.size() - 3);
      QString currPath = TclGetString(Tcl_GetObjResult(interpreter));
      currPath += QDir::separator() + userInput;
      QString path =
          userInput.startsWith(QDir::separator())
              ? userInput.mid(0, userInput.lastIndexOf(QDir::separator()) + 1)
              : currPath.mid(0, currPath.lastIndexOf(QDir::separator()) + 1);
      QString filter = QString("%1*").arg(currPath.remove(
          currPath.mid(0, currPath.lastIndexOf(QDir::separator()) + 1)));
      QDir dir{path};
      suggestions += dir.entryList({filter}, QDir::Dirs | QDir::CaseSensitive);
      if (!suggestions.isEmpty()) {
        auto cutSeperator = cmd.mid(0, cmd.lastIndexOf(QDir::separator()) + 1);
        if (cutSeperator.isEmpty())
          cutSeperator = cmd.mid(0, cmd.lastIndexOf(" ") + 1);
        prefix = cutSeperator.isEmpty() ? cmd : cutSeperator;
      }
    }
  }
  return suggestions;
}

}  // namespace FOEDAG
