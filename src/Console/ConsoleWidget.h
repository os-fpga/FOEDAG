#pragma once

#include <QPlainTextEdit>
#include <memory>
#include <ostream>

#include "ConsoleInterface.h"

class ConsoleWidget : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit ConsoleWidget(std::unique_ptr<ConsoleInterface> iConsole,
                         QWidget *parent = nullptr);

  std::ostream &getStream();

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

 private slots:
  void put(const QString &str);

 private:
  void updateScroll();

 private:
  std::unique_ptr<ConsoleInterface> m_console;
  class StreamBuffer *m_buffer;
  std::ostream m_stream;
};
