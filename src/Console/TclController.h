#pragma once

#include <QObject>
namespace FOEDAG {
class TclInterpreter;
}

class TclController : public QObject {
  Q_OBJECT
 public:
  explicit TclController(FOEDAG::TclInterpreter *interpreter);
  ~TclController();

 public slots:
  void runCommand(const QString &command);

 signals:
  void sendOutput(QString);

 private:
  FOEDAG::TclInterpreter *m_interpreter{nullptr};
  // Tcl_Channel m_channel;
};