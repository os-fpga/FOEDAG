#pragma once

#include <QThread>

namespace FOEDAG {
class TclInterpreter;
}

class TclWorker : public QThread {
  Q_OBJECT
 public:
  TclWorker(FOEDAG::TclInterpreter *interpreter, std::ostream &out,
            QObject *parent = nullptr);
  QString output() const;

  void run() override;

 public slots:
  void runCommand(const QString &command);
  void abort();

 signals:
  void tclFinished();

 private:
  void init();
  void setOutput(const QString &out);

 private:
  FOEDAG::TclInterpreter *m_interpreter{nullptr};
  std::ostream &m_out;
  QString m_output;
  QString m_putsOutput;
  QString m_cmd;
};
