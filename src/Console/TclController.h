#pragma once

#include <QObject>
extern "C" {
#include <tcl.h>
}

class TclController : public QObject {
  Q_OBJECT
 public:
  explicit TclController(Tcl_Interp *interpreter);
  ~TclController();
  void runCommand(const QString &command);

 signals:
  void sendOutput(QString);

 private:
  Tcl_Interp *m_interpreter{nullptr};
  Tcl_Channel m_channel;
};