#pragma once

#include <QPlainTextEdit>
#include <memory>

#include "ConsoleInterface.h"

class ConsoleWidget : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit ConsoleWidget(std::unique_ptr<ConsoleInterface> iConsole,
                         QWidget *parent = nullptr);

 public slots:
  void append(const QString &);

 signals:
  void sendCommand(QString);
  void abort();

 protected:
  void keyPressEvent(QKeyEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private:
  void updateScroll();

 private:
  std::unique_ptr<ConsoleInterface> m_console;
};
