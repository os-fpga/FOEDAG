#pragma once

#include <QPlainTextEdit>
#include <QTextBlock>
#include <memory>
#include <ostream>

#include "ConsoleInterface.h"

class StreamBuffer;
class ConsoleWidget : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit ConsoleWidget(std::unique_ptr<ConsoleInterface> iConsole,
                         StreamBuffer *buffer, QWidget *parent = nullptr);

 public slots:
  /*!
   * \brief append - append \a text into the end and insert new line.
   */
  void append(const QString &text);

 signals:
  void sendCommand(QString);
  void abort();

 protected:
  void keyPressEvent(QKeyEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

 private slots:
  void put(const QString &str);
  void commandDone();

 private:
  void updateScroll();
  QString getCommand() const;
  void handleLink(const QPoint &p);

 private:
  std::unique_ptr<ConsoleInterface> m_console;
  StreamBuffer *m_buffer;
  const QString m_startWith;

  Qt::MouseButton m_mouseButtonPressed = Qt::NoButton;
  bool m_linkActivated{false};
  int m_beginPosition{0};
  int m_lastPos;
};
