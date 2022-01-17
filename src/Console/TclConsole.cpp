#include "TclConsole.h"

#include <iostream>

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

QString TclConsole::startWith() const { return "# "; }

void TclConsole::abort() {
  //
}

void TclConsole::tclFinished() {
  //
}
