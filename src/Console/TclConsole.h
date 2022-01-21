#pragma once

#include <QThread>

#include "ConsoleInterface.h"
#include "TclWorker.h"

namespace FOEDAG {
class TclInterpreter;
}

class TclConsole : public ConsoleInterface {
  Q_OBJECT
 public:
  TclConsole(FOEDAG::TclInterpreter *interpreter, std::ostream &out,
             QObject *parent = nullptr);
  void registerInterpreter(FOEDAG::TclInterpreter *interpreter);
  ~TclConsole();
  void run(const QString &command) override;
  int returnCode() const override;
  QStringList suggestCommand(const QString &cmd, QString &prefix) override;
  bool isCommandComplete(const QString &command) override;

 public slots:
  void abort();

 private slots:
  void tclFinished();

 signals:
  void sendCommand(QString);
  void abort_();

 private:
  QStringList getFilesCompletion(FOEDAG::TclInterpreter *interpreter,
                                 const QString &cmd, QString &prefix) const;

 private:
  TclWorker *m_tclWorker;
  std::vector<TclWorker *> m_tclWorkers;
  std::ostream &m_out;
};
