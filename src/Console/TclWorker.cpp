#include "TclWorker.h"

#include <QDebug>
#include <QMetaMethod>

#include "TclConsoleBuilder.h"

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
  Q_UNUSED(instanceData)
  Q_UNUSED(errorCodePtr)
  static bool done{false};
  Tcl_SetErrno(0);
  if (!done) {
    auto console = FOEDAG::TclConsoleGLobal::tclConsole();
    if (console) {
      // invoke from another thread.
      QByteArray normalizedSignature =
          QMetaObject::normalizedSignature("put(QString)");
      int methodIndex =
          console->metaObject()->indexOfMethod(normalizedSignature);
      QMetaMethod method = console->metaObject()->method(methodIndex);
      method.invoke(console, Qt::QueuedConnection, Q_ARG(QString, buf));
    }
  }
  done = !done;
  return toWrite;
}

int DriverBlockModeProc(ClientData instanceData, int mode) {
  Q_UNUSED(instanceData)
  Q_UNUSED(mode)
  return 0;
}

TclWorker::TclWorker(TclInterp *interpreter, std::ostream &out, QObject *parent)
    : QThread(parent), m_interpreter(interpreter), m_out(out) {}

void TclWorker::runCommand(const QString &command) { m_cmd = command; }

void TclWorker::abort() {
  auto resultObjPtr = Tcl_NewObj();
  //  Tcl_Interp *in = m_interpreter;
  int ret = Tcl_CancelEval(m_interpreter, resultObjPtr, nullptr, 0);
  qDebug() << ret;
  if (ret != TCL_OK) {
    m_output = Tcl_GetString(resultObjPtr);
  } else {
    runCommand("error \"aborted by user\"");
  }
}

QString TclWorker::output() const { return m_output; }

void TclWorker::run() {
  init();

  m_putsOutput.clear();
  m_returnCode = Tcl_Eval(m_interpreter, qPrintable(m_cmd));

  QString output = Tcl_GetStringResult(m_interpreter);
  if (m_putsOutput.isEmpty()) {
    setOutput(output);
  }
  emit tclFinished();
}

int TclWorker::returnCode() const { return m_returnCode; }

TclInterp *TclWorker::getInterpreter() { return m_interpreter; }

void TclWorker::setOutput(const QString &out) {
  m_output = out;
  m_out << m_output.toLatin1().data();
  if (!m_output.isEmpty()) m_out << std::endl;
}

void TclWorker::init() {
  static Tcl_Channel m_channel;
  static Tcl_Channel errConsoleChannel;
  Tcl_ChannelType *channelOut = new Tcl_ChannelType{
      "outconsole",
      static_cast<Tcl_ChannelTypeVersion>(TCL_CHANNEL_VERSION_5),
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

  if (!m_channel) {
    m_channel = Tcl_CreateChannel(channelOut, "stdout",
                                  reinterpret_cast<ClientData>(TCL_STDOUT),
                                  TCL_WRITABLE);
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
        channelOut, "stderr", reinterpret_cast<ClientData>(TCL_STDERR),
        TCL_WRITABLE);
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
