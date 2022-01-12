#include "TclWorker.h"

#include <QDebug>

#include "Tcl/TclInterpreter.h"

int DriverClose2Proc(ClientData instanceData, Tcl_Interp *interp, int flags) {
  return 0;
}

void DriverWatchProc(ClientData instanceData, int mask) {}

int DriverInputProc(ClientData instanceData, char *buf, int bufSize,
                    int *errorCodePtr) {
  return 0;
}

int DriverOutputProc(ClientData instanceData, const char *buf, int toWrite,
                     int *errorCodePtr) {
  return toWrite;
}

int DriverBlockModeProc(ClientData instanceData, int mode) { return 0; }

TclWorker::TclWorker(FOEDAG::TclInterpreter *interpreter, QObject *parent)
    : QObject(parent), m_interpreter(interpreter) {
  init();
}

void TclWorker::runCommand(const QString &command) {
  m_putsOutput.clear();
  auto cmd = command.toStdString();
  QString output = m_interpreter->evalCmd(cmd).c_str();
  if (m_putsOutput.isEmpty()) {
    setOutput(output);
  }
}

void TclWorker::abort() {
  auto resultObjPtr = Tcl_NewObj();
  int ret = Tcl_CancelEval(m_interpreter->getInterp(), resultObjPtr, NULL, 0);
  if (ret != TCL_OK) {
    m_output = Tcl_GetString(resultObjPtr);
  } else {
    runCommand("error \"aborted by user\"");
  }
}

QString TclWorker::output() const { return m_output; }

FOEDAG::TclInterpreter *TclWorker::getInterp() { return m_interpreter; }

void TclWorker::setOutput(const QString &out) {
  m_output = out;
  emit ready();
}

void TclWorker::init() {
  Tcl_ChannelType ChannelType = {
      "MyChannelOut",
      (Tcl_ChannelTypeVersion)TCL_CHANNEL_VERSION_5,
      NULL,
      DriverInputProc,
      DriverOutputProc,
      NULL /*ChannelSeek*/,
      NULL,
      NULL,
      DriverWatchProc,
      NULL, /*ChannelGetHandle,*/
      DriverClose2Proc /*ChannelClose2*/,
      NULL, /*ChannelBlockMode,*/
      NULL /*ChannelFlush*/,
      NULL /*ChannelHandler*/,
      NULL /*ChannelWideSeek*/
  };

  void *chData = reinterpret_cast<void *>(this);
  auto stdout_ = Tcl_GetStdChannel(TCL_STDOUT);
  Tcl_Channel m_channel = Tcl_CreateChannel(
      &ChannelType, "MyChannelOut", chData, TCL_WRITABLE | TCL_READABLE);
  Tcl_RegisterChannel(m_interpreter->getInterp(), m_channel);
  Tcl_SetStdChannel(m_channel, TCL_STDOUT);
  Tcl_UnregisterChannel(m_interpreter->getInterp(), stdout_);

  auto puts = [](ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const objv[]) -> int {
    QString putOut = QString(Tcl_GetString(objv[1]));
    TclWorker *worker = reinterpret_cast<TclWorker *>(clientData);
    if (worker) {
      worker->m_putsOutput = putOut;
      worker->setOutput(putOut);
    }
    return 0;
  };
  Tcl_CreateObjCommand(m_interpreter->getInterp(), "puts", puts, chData, 0);
}
