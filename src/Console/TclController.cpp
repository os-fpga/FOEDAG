#include "TclController.h"

#include <QDebug>

#include "string.h"

TclController::TclController(Tcl_Interp *interpreter)
    : m_interpreter(interpreter) {}

TclController::~TclController() {}

void TclController::runCommand(const QString &command) {
  auto cmd = command.toStdString().c_str();
  int a = Tcl_EvalEx(m_interpreter, cmd, -1, 0);
  qDebug() << "Return code: " << a;
  QString data = Tcl_GetStringResult(m_interpreter);
  emit sendOutput(data);
}