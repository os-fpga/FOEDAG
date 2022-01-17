#include "ConsoleWidget.h"

#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextBlock>

#include "StreamBuffer.h"

ConsoleWidget::ConsoleWidget(std::unique_ptr<ConsoleInterface> iConsole,
                             StreamBuffer *buffer, QWidget *parent)
    : QPlainTextEdit(parent),
      m_console(std::move(iConsole)),
      m_buffer{buffer},
      m_startWith(m_console ? m_console->startWith() : QString()) {
  connect(m_buffer, &StreamBuffer::ready, this, &ConsoleWidget::put);
  if (m_console) {
    connect(m_console.get(), &ConsoleInterface::done, this,
            &ConsoleWidget::commandDone);
  }
  commandDone();
  setMouseTracking(true);
}

void ConsoleWidget::append(const QString &text) {
  if (!text.isEmpty()) {
    auto prev = textCursor().position();
    const QString tmp = text.simplified() /* + "\n"*/;
    appendHtml(tmp);
    m_beginPosition = textCursor().position() - prev;
    m_lastPos = textCursor().position();
    updateScroll();
  }
}

void ConsoleWidget::put(const QString &str) {
  if (!str.isEmpty()) {
    auto prev = textCursor().position();
    insertPlainText(str);
    m_beginPosition = textCursor().position() - prev;
    m_lastPos = textCursor().position();

    updateScroll();
  }
}

void ConsoleWidget::commandDone() { put(m_startWith); }

void ConsoleWidget::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
      QString cmd = getCommand();
      QPlainTextEdit::keyPressEvent(e);
      if (m_console) m_console->run(cmd.toUtf8());
    } break;
    case Qt::Key_Backspace:
      if (m_lastPos < textCursor().position()) QPlainTextEdit::keyPressEvent(e);
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

void ConsoleWidget::mouseMoveEvent(QMouseEvent *e) {
  // Cursor was dragged to make a selection, deactivate links
  if (m_mouseButtonPressed != Qt::NoButton && textCursor().hasSelection())
    m_linkActivated = false;

  if (!m_linkActivated || anchorAt(e->pos()).isEmpty())
    viewport()->setCursor(Qt::IBeamCursor);
  else
    viewport()->setCursor(Qt::PointingHandCursor);
  QPlainTextEdit::mouseMoveEvent(e);
}

void ConsoleWidget::mousePressEvent(QMouseEvent *e) {
  m_mouseButtonPressed = e->button();
  QPlainTextEdit::mousePressEvent(e);
}

void ConsoleWidget::mouseReleaseEvent(QMouseEvent *e) {
  if (m_linkActivated && m_mouseButtonPressed == Qt::LeftButton)
    handleLink(e->pos());

  // Mouse was released, activate links again
  m_linkActivated = true;
  m_mouseButtonPressed = Qt::NoButton;

  QPlainTextEdit::mouseReleaseEvent(e);
}

void ConsoleWidget::updateScroll() {
  QScrollBar *bar = verticalScrollBar();
  bar->setValue(bar->maximum());
}

QString ConsoleWidget::getCommand() const {
  QString cmd = document()->lastBlock().text();
  return cmd.mid(m_beginPosition - 1);
}

void ConsoleWidget::handleLink(const QPoint &p) { qDebug() << anchorAt(p); }
