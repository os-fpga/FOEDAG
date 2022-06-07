#include "TclWorker.h"

#include <QDebug>
#include <iostream>

#include "Compiler/Log.h"

extern FOEDAG::Session *GlobalSession;

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
  worker->out().write(buf, toWrite);
  worker->out().flush();
  return toWrite;
}

int DriverInputProc(ClientData instanceData, char *buf, int bufSize,
                    int *errorCodePtr) {
  if (bufSize < 0) {
    *errorCodePtr = -1;
    return -1;
  }
  std::string in;
  getline(std::cin, in);
  LOG_CMD(in);
  in.push_back('\n');
  size_t count = in.size();
  size_t bSize = static_cast<size_t>(bufSize);
  if (in.size() > bSize) count = bSize;
  strncpy(buf, in.c_str(), count);
  *errorCodePtr = 0;
  return count;
}

int DriverErrorProc(ClientData instanceData, const char *buf, int toWrite,
                    int *errorCodePtr) {
  Q_UNUSED(errorCodePtr)
  TclWorker *worker = static_cast<TclWorker *>(instanceData);
  worker->err()->write(buf, toWrite);
  worker->err()->flush();
  return toWrite;
}

int DriverBlockModeProc(ClientData instanceData, int mode) {
  Q_UNUSED(instanceData)
  Q_UNUSED(mode)
  return 0;
}

TclWorker::TclWorker(TclInterp *interpreter, std::ostream &out,
                     std::ostream *err, bool batchMode, QObject *parent)
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

  if (batchMode) {
    channelIn = new Tcl_ChannelType{
        "stdin",
        CHANNEL_VERSION_5,
        DriverCloseProc /*ChannelClose */,
        DriverInputProc,  // DriverInputProc,  // DriverInputProc,
        nullptr,
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

  init(batchMode);
}

void TclWorker::runCommand(const QString &command) {
  init(false);

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

void TclWorker::init(bool batchMode) {
  static Tcl_Channel m_channel{nullptr};
  static Tcl_Channel m_channelIn{nullptr};
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

  if (!m_channelIn && batchMode) {
    m_channelIn = Tcl_CreateChannel(channelIn, "stdin",
                                    static_cast<void *>(this), TCL_READABLE);
    if (m_channelIn) {
      Tcl_RegisterChannel(m_interpreter, m_channelIn);
    }
  }
  Tcl_SetStdChannel(m_channelIn, TCL_STDIN);

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
