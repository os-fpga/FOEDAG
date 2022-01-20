#pragma once

#include <QObject>

class ConsoleInterface : public QObject {
  Q_OBJECT
 public:
  explicit ConsoleInterface(QObject *parent = nullptr);
  virtual ~ConsoleInterface() = default;

  virtual void run(const QString &command) = 0;
  virtual int returnCode() const = 0;
  virtual QStringList suggestCommand(const QString &cmd, QString &prefix) = 0;
  virtual bool isCommandComplete(const QString &command) = 0;

 signals:
  void done();
  void sendOutput(const QString &);
};
