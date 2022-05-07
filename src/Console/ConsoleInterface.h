#pragma once

#include <QObject>

namespace FOEDAG {

class ConsoleInterface : public QObject {
  Q_OBJECT
 public:
  explicit ConsoleInterface(QObject *parent = nullptr);
  virtual ~ConsoleInterface() = default;

  virtual void run(const QString &command) = 0;
  virtual void abort() = 0;
  virtual QStringList suggestCommand(const QString &cmd, QString &prefix) = 0;
  virtual bool isCommandComplete(const QString &command) = 0;
  virtual void setErrorStream(std::ostream *err) = 0;

 signals:
  void inProgressState();
  void done();
  void sendOutput(const QString &);
  void aborted();
};

}  // namespace FOEDAG
