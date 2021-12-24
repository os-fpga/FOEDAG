#pragma once

#include <QObject>

class ConsoleInterface : public QObject {
  Q_OBJECT
 public:
  explicit ConsoleInterface(QObject *parent = nullptr);
  virtual ~ConsoleInterface() = default;

  virtual void run(const QString &command) = 0;

 signals:
};
