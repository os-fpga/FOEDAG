#include "Console.h"

#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextBlock>

constexpr auto begin = "% ";

Console::Console(QWidget *parent) : QPlainTextEdit(parent) {
  setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
}

void Console::append(const QString &text) {
  if (!text.isEmpty()) appendPlainText(text);
}

void Console::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
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

    default:
      QPlainTextEdit::keyPressEvent(e);
      break;
  }
  QScrollBar *bar = verticalScrollBar();
  bar->setValue(bar->maximum());
}

void Console::mousePressEvent(QMouseEvent *event) { event->ignore(); }

void Console::mouseDoubleClickEvent(QMouseEvent *event) { event->ignore(); }