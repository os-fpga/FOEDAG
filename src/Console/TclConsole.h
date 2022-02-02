#pragma once

#include <QThread>

#include "ConsoleInterface.h"
#include "TclWorker.h"

namespace FOEDAG {

class TclConsole : public ConsoleInterface {
  Q_OBJECT
 public:
  TclConsole(TclInterp *interpreter, std::ostream &out,
             QObject *parent = nullptr);
  void registerInterpreter(TclInterp *interpreter);
  ~TclConsole() override;
  void run(const QString &command) override;
  int returnCode() const override;
  QStringList suggestCommand(const QString &cmd, QString &prefix) override;
  bool isCommandComplete(const QString &command) override;
  void abort() override;

 private slots:
  void tclFinished();

 private:
  QStringList getFilesCompletion(TclInterp *interpreter, const QString &cmd,
                                 QString &prefix) const;

 private:
  TclWorker *m_tclWorker;
  std::vector<TclWorker *> m_tclWorkers;
  std::ostream &m_out;
};

}  // namespace FOEDAG
