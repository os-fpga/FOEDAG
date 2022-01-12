#pragma once

#include <QObject>
#include <QThread>

#include "TclWorker.h"
namespace FOEDAG {
class TclInterpreter;
}

class TclController : public QObject {
  Q_OBJECT
 public:
  explicit TclController(FOEDAG::TclInterpreter *interpreter,
                         QObject *parent = nullptr);
  ~TclController();

 public slots:
  void runCommand(const QString &command);
  void abort();

 private slots:
  void tclCommandDone();

 signals:
  void sendOutput(QString);
  void sendCommand(QString);
  void abort_();

 private:
  QThread m_tclThread;
  TclWorker m_tclWorker;
};
