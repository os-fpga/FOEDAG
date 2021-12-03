#include "TclController.h"

#include <QDebug>

#include "Tcl/TclInterpreter.h"
#include "string.h"

TclController::TclController(FOEDAG::TclInterpreter *interpreter)
    : m_interpreter(interpreter) {}

TclController::~TclController() {}

void TclController::runCommand(const QString &command) {
  auto cmd = command.toStdString();
  auto output = m_interpreter->evalCmd(cmd);
  emit sendOutput(QString(output.c_str()));
}