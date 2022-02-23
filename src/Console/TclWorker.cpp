#include "TclWorker.h"

#include <QDebug>
#include <iostream>

namespace FOEDAG {

int DriverClose2Proc(ClientData instanceData, Tcl_Interp *interp, int flags) {
  Q_UNUSED(instanceData)
  Q_UNUSED(interp)
  Q_UNUSED(flags)
  return 0;
}

void DriverWatchProc(ClientData instanceData, int mask) {
  Q_UNUSED(instanceData)
  Q_UNUSED(mask)
}

int DriverOutputProc(ClientData instanceData, const char *buf, int toWrite,
                     int *errorCodePtr) {
  Q_UNUSED(errorCodePtr)
  Tcl_SetErrno(0);
  TclWorker *worker = static_cast<TclWorker *>(instanceData);
  worker->out() << buf;
  return toWrite;
}

int DriverBlockModeProc(ClientData instanceData, int mode) {
  Q_UNUSED(instanceData)
  Q_UNUSED(mode)
  return 0;
}

TclWorker::TclWorker(TclInterp *interpreter, std::ostream &out, QObject *parent)
    : QObject(parent), m_interpreter(interpreter), m_out(out) {
  channelOut = new Tcl_ChannelType{
      "outconsole",
      CHANNEL_VERSION_5,
      nullptr,
      nullptr,  // DriverInputProc,
      DriverOutputProc,
      nullptr /*ChannelSeek*/,
      nullptr,
      nullptr,
      DriverWatchProc,
      nullptr, /*ChannelGetHandle,*/
      DriverClose2Proc /*ChannelClose2*/,
      nullptr, /*ChannelBlockMode,*/
      nullptr /*ChannelFlush*/,
      nullptr /*ChannelHandler*/,
      nullptr /*ChannelWideSeek*/,
      nullptr /*DriverThreadAction*/,
      nullptr /*DriverTruncate*/,
  };
}

void TclWorker::runCommand(const QString &command) { m_cmd = command; }

void TclWorker::abort() {
  if (m_evalInProgress) {
    auto resultObjPtr = Tcl_NewObj();
    Tcl_CancelEval(m_interpreter, resultObjPtr, nullptr, 0);
    setOutput(Tcl_GetString(resultObjPtr));
  }
}

void TclWorker::run() {
  m_evalInProgress = true;
  init();

  m_returnCode = 0;
  m_returnCode = TclEval(m_interpreter, qPrintable(m_cmd));

  QString output = TclGetStringResult(m_interpreter);
  setOutput(output);
  emit tclFinished();
  m_evalInProgress = false;
}

int TclWorker::returnCode() const { return m_returnCode; }

TclInterp *TclWorker::getInterpreter() { return m_interpreter; }

void TclWorker::setOutput(const QString &out) {
  m_out << out.toLatin1().data();
  if (!out.isEmpty()) m_out << std::endl;
}

void TclWorker::init() {
  static Tcl_Channel m_channel{nullptr};
  static Tcl_Channel errConsoleChannel{nullptr};

  if (!m_channel) {
    m_channel = Tcl_CreateChannel(channelOut, "stdout",
                                  static_cast<void *>(this), TCL_WRITABLE);
    if (m_channel) {
      Tcl_SetChannelOption(nullptr, m_channel, "-translation", "lf");
      Tcl_SetChannelOption(nullptr, m_channel, "-buffering", "none");
      Tcl_RegisterChannel(m_interpreter, m_channel);
      Tcl_SetStdChannel(m_channel, TCL_STDOUT);
    }
  } else {
    Tcl_SetStdChannel(m_channel, TCL_STDOUT);
  }

  if (!errConsoleChannel) {
    errConsoleChannel = Tcl_CreateChannel(
        channelOut, "stderr", static_cast<void *>(this), TCL_WRITABLE);
    if (errConsoleChannel) {
      Tcl_SetChannelOption(nullptr, errConsoleChannel, "-translation", "lf");
      Tcl_SetChannelOption(nullptr, errConsoleChannel, "-buffering", "none");
      Tcl_RegisterChannel(m_interpreter, errConsoleChannel);
      Tcl_SetStdChannel(errConsoleChannel, TCL_STDERR);
    }
  } else {
    Tcl_SetStdChannel(errConsoleChannel, TCL_STDERR);
  }
}

}  // namespace FOEDAG
