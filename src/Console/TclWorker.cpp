#include "TclWorker.h"

#include <QDebug>
#include <iostream>

namespace FOEDAG {

int DriverCloseProc(ClientData instanceData, Tcl_Interp *interp) {
  Q_UNUSED(instanceData)
  Q_UNUSED(interp)
  return 0;
}

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
  TclWorker *worker = static_cast<TclWorker *>(instanceData);
  worker->setOutput(buf);
  return toWrite;
}

int DriverErrorProc(ClientData instanceData, const char *buf, int toWrite,
                    int *errorCodePtr) {
  Q_UNUSED(errorCodePtr)
  TclWorker *worker = static_cast<TclWorker *>(instanceData);
  worker->setError(buf);
  return toWrite;
}

int DriverBlockModeProc(ClientData instanceData, int mode) {
  Q_UNUSED(instanceData)
  Q_UNUSED(mode)
  return 0;
}

TclWorker::TclWorker(TclInterp *interpreter, std::ostream &out,
                     std::ostream *err, QObject *parent)
    : QObject(parent), m_interpreter(interpreter), m_out(out), m_err(err) {
  channelOut = new Tcl_ChannelType{
      "outconsole",
      CHANNEL_VERSION_5,
      DriverCloseProc /*ChannelClose */,
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
  channelErr = new Tcl_ChannelType{
      "errconsole",
      CHANNEL_VERSION_5,
      DriverCloseProc /*ChannelClose */,
      nullptr,  // DriverInputProc,
      DriverErrorProc,
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

  // set to nullptr stdin to avoid prompt from TCL.
  Tcl_SetStdChannel(nullptr, TCL_STDIN);
  init();
}

void TclWorker::runCommand(const QString &command) {
  init();

  int returnCode = TclEval(m_interpreter, qPrintable(command));
  if (returnCode == TCL_ERROR) {
    Tcl_Obj *options = Tcl_GetReturnOptions(m_interpreter, returnCode);
    Tcl_Obj *key = Tcl_NewStringObj("-errorinfo", -1);
    Tcl_Obj *stackTrace;
    Tcl_IncrRefCount(key);
    Tcl_DictObjGet(NULL, options, key, &stackTrace);
    Tcl_DecrRefCount(key);
    setError(Tcl_GetString(stackTrace));
    Tcl_DecrRefCount(options);
  } else {
    setOutput(TclGetStringResult(m_interpreter));
  }

  emit tclFinished();
}

void TclWorker::abort() { Tcl_CancelEval(m_interpreter, nullptr, nullptr, 0); }

TclInterp *TclWorker::getInterpreter() { return m_interpreter; }

void TclWorker::setErrStream(std::ostream *err) { m_err = err; }

void TclWorker::setOutput(const QString &out) {
  m_out << out.toLatin1().data();
  if (!out.isEmpty()) m_out << std::endl;
}

void TclWorker::setError(const QString &err) {
  if (m_err) {
    *m_err << err.toLatin1().data();
    if (!err.isEmpty()) *m_err << std::endl;
  }
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
        channelErr, "stderr", static_cast<void *>(this), TCL_WRITABLE);
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
