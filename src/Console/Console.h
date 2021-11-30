#pragma once

#include <QPlainTextEdit>

class Console : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit Console(QWidget *parent = nullptr);

 public slots:
  void append(const QString &);

 signals:
  void sendCommand(QString);

 protected:
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private:
};