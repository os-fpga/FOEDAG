#include "TclController.h"

#include <QDebug>
#include <iostream>

#include "Tcl/TclInterpreter.h"
#include "string.h"

TclController::TclController(FOEDAG::TclInterpreter *interpreter,
                             QObject *parent)
    : m_tclWorker(interpreter, parent) {
  m_tclWorker.moveToThread(&m_tclThread);
  connect(&m_tclWorker, &TclWorker::ready, this,
          &TclController::tclCommandDone);
  connect(this, &TclController::sendCommand, &m_tclWorker,
          &TclWorker::runCommand);
  connect(this, &TclController::abort_, &m_tclWorker, &TclWorker::abort,
          Qt::DirectConnection);

  m_tclThread.start();
}

TclController::~TclController() {
  m_tclThread.quit();
  m_tclThread.wait();
}

void TclController::runCommand(const QString &command) {
  if (!m_tclThread.isRunning()) m_tclThread.start();
  emit sendCommand(command);
}

void TclController::abort() { emit abort_(); }

void TclController::tclCommandDone() { emit sendOutput(m_tclWorker.output()); }

FOEDAG::TclInterpreter *TclController::getInterp() {
  return m_tclWorker.getInterp();
}
