#include "TclWorker.h"

#include <QDebug>

#include "Tcl/TclInterpreter.h"

static const char *FOEDAG_Channel{"FOEDAG_Channel"};

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

TclWorker::TclWorker(FOEDAG::TclInterpreter *interpreter, std::ostream &out,
                     QObject *parent)
    : QThread(parent), m_interpreter(interpreter), m_out(out) {
  init();
}

void TclWorker::runCommand(const QString &command) { m_cmd = command; }

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

void TclWorker::run() {
  m_putsOutput.clear();
  auto cmd = m_cmd.toStdString();
  QString output = m_interpreter->evalCmd(cmd, &m_returnCode).c_str();
  if (m_putsOutput.isEmpty()) {
    setOutput(output);
  }
  emit tclFinished();
}

int TclWorker::returnCode() const { return m_returnCode; }

FOEDAG::TclInterpreter *TclWorker::getInterpreter() { return m_interpreter; }

void TclWorker::setOutput(const QString &out) {
  m_output = out;
  m_out << m_output.toLatin1().data();
  if (!m_output.isEmpty()) m_out << std::endl;
}

void TclWorker::init() {
  Tcl_Channel channel =
      Tcl_GetChannel(m_interpreter->getInterp(), FOEDAG_Channel, nullptr);
  if (channel != nullptr)  // already registered
    return;

  Tcl_ChannelType *ChannelType = new Tcl_ChannelType{
      FOEDAG_Channel,
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
      NULL /*ChannelWideSeek*/,
      NULL /*DriverThreadAction*/,
      NULL /*DriverTruncate*/,
  };

  void *chData = reinterpret_cast<void *>(this);
  auto stdout_ = Tcl_GetStdChannel(TCL_STDOUT);
  Tcl_Channel m_channel = Tcl_CreateChannel(ChannelType, FOEDAG_Channel, chData,
                                            TCL_WRITABLE | TCL_READABLE);
  Tcl_RegisterChannel(m_interpreter->getInterp(), m_channel);
  Tcl_SetStdChannel(m_channel, TCL_STDOUT);
  Tcl_UnregisterChannel(m_interpreter->getInterp(), stdout_);

  auto puts = [](ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const objv[]) -> int {
    QString putOut = QString(Tcl_GetString(objv[1]));
    TclWorker *worker = reinterpret_cast<TclWorker *>(clientData);
    if (worker) {
      worker->m_putsOutput = putOut;
      worker->m_returnCode = TCL_OK;
      worker->setOutput(putOut);
    }
    return 0;
  };
  Tcl_CreateObjCommand(m_interpreter->getInterp(), "puts", puts, chData, 0);
}
