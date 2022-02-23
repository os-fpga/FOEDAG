#pragma once

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

  void setTclCommandInProggress(bool inProgress);

 private slots:
  void tclWorkerFinished();

 private:
  QStringList getFilesCompletion(TclInterp *interpreter, const QString &cmd,
                                 QString &prefix) const;

 private:
  TclWorker *m_tclWorker;
  std::vector<TclWorker *> m_tclWorkers;
  std::ostream &m_out;
  bool m_commandInProggress{false};
};

}  // namespace FOEDAG
