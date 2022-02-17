#pragma once

#include <QThread>

#include "ConsoleDefines.h"

namespace FOEDAG {

class TclWorker : public QObject {
  Q_OBJECT
 public:
  TclWorker(TclInterp *interpreter, std::ostream &out,
            QObject *parent = nullptr);

  void run();
  int returnCode() const;
  TclInterp *getInterpreter();
  std::ostream &out() { return m_out; }

 public slots:
  void runCommand(const QString &command);
  void abort();

 signals:
  void tclFinished();

 private:
  void init();
  void setOutput(const QString &out);

 private:
  TclInterp *m_interpreter{nullptr};
  std::ostream &m_out;
  int m_returnCode{0};
  QString m_cmd;
  Tcl_ChannelType *channelOut{nullptr};
};

}  // namespace FOEDAG
