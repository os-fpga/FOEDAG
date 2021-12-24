#pragma once

#include <QObject>

namespace FOEDAG {
class TclInterpreter;
}

class TclWorker : public QObject {
  Q_OBJECT
 public:
  TclWorker(FOEDAG::TclInterpreter *interpreter, QObject *parent = nullptr);
  QString output() const;
  FOEDAG::TclInterpreter *getInterp();

 public slots:
  void runCommand(const QString &command);
  void abort();

 signals:
  void ready();

 private:
  void init();
  void setOutput(const QString &out);

 private:
  FOEDAG::TclInterpreter *m_interpreter{nullptr};
  QString m_output;
  QString m_putsOutput;
};
