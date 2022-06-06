#pragma once

#include <QObject>
#include <iostream>

#include "ConsoleDefines.h"

namespace FOEDAG {

class TclWorker : public QObject {
  Q_OBJECT
 public:
  TclWorker(TclInterp *interpreter, std::ostream &out,
            std::ostream *err = &std::cerr, bool batchMode = false,
            QObject *parent = nullptr);

  void run();
  TclInterp *getInterpreter();
  std::ostream &out() { return m_out; }
  std::ostream *err() { return m_err; }
  void setErrStream(std::ostream *err);
  void setOutput(const QString &out);
  void setError(const QString &err);

 public slots:
  void runCommand(const QString &command);
  void abort();

 signals:
  void tclFinished();

 private:
  void init(bool batchMode);

 private:
  TclInterp *m_interpreter{nullptr};
  std::ostream &m_out;
  std::ostream *m_err;
  Tcl_ChannelType *channelOut{nullptr};
  Tcl_ChannelType *channelIn{nullptr};
  Tcl_ChannelType *channelErr{nullptr};
};

}  // namespace FOEDAG
