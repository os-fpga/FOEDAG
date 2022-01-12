#include "ConsoleWidget.h"

#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextBlock>

#include "StreamBuffer.h"

constexpr auto begin = "% ";

ConsoleWidget::ConsoleWidget(std::unique_ptr<ConsoleInterface> iConsole,
                             QWidget *parent)
    : QPlainTextEdit(parent),
      m_console(std::move(iConsole)),
      m_buffer{new StreamBuffer{this}},
      m_stream(m_buffer) {
  connect(m_buffer, &StreamBuffer::ready, this, &ConsoleWidget::put);
}

std::ostream &ConsoleWidget::getStream() { return m_stream; }

void ConsoleWidget::append(const QString &text) {
  if (!text.isEmpty()) {
    const QString tmp = text.simplified() + "\n";
    insertPlainText(tmp);
    updateScroll();
  }
}

void ConsoleWidget::put(const QString &str) {
  textCursor().insertText(str);
  updateScroll();
}

void ConsoleWidget::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      if (m_console) m_console->run(document()->lastBlock().text().toUtf8());
      emit sendCommand(document()->lastBlock().text().toUtf8());
      QPlainTextEdit::keyPressEvent(e);
      break;
    case Qt::Key_Backspace:
      if (document()->lastBlock().begin() != document()->lastBlock().end())
        QPlainTextEdit::keyPressEvent(e);
      break;
    case Qt::Key_Up:
    case Qt::Key_Down:
      // TBD for the history
      break;

    case Qt::Key_Tab:
      // TBD for autocomplete
      break;

    case Qt::Key_C:
      if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier) {
        emit abort();
      } else
        QPlainTextEdit::keyPressEvent(e);
      break;

    default:
      QPlainTextEdit::keyPressEvent(e);
      break;
  }
  updateScroll();
}

void ConsoleWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  event->ignore();
}

void ConsoleWidget::updateScroll() {
  QScrollBar *bar = verticalScrollBar();
  bar->setValue(bar->maximum());
}
