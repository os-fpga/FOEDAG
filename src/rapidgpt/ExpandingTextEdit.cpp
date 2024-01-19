// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH
// Qt-GPL-exception-1.0
#include "ExpandingTextEdit.h"

#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QScrollArea>

ExpandingTextEdit::ExpandingTextEdit(QWidget *parent) : QTextEdit(parent) {
  setSizePolicy(
      QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QAbstractTextDocumentLayout *docLayout = document()->documentLayout();
  connect(docLayout, &QAbstractTextDocumentLayout::documentSizeChanged, this,
          &ExpandingTextEdit::updateHeight);
  connect(this, &QTextEdit::cursorPositionChanged, this,
          &ExpandingTextEdit::reallyEnsureCursorVisible);
  m_minimumHeight =
      qRound(docLayout->documentSize().height()) + frameWidth() * 2;
}
void ExpandingTextEdit::updateHeight(const QSizeF &documentSize) {
  m_minimumHeight = qRound(documentSize.height()) + frameWidth() * 2;
  updateGeometry();
}
QSize ExpandingTextEdit::sizeHint() const {
  return QSize(100, m_minimumHeight);
}
QSize ExpandingTextEdit::minimumSizeHint() const {
  return QSize(100, m_minimumHeight);
}
void ExpandingTextEdit::reallyEnsureCursorVisible() {
  QObject *ancestor = parent();
  while (ancestor) {
    QScrollArea *scrollArea = qobject_cast<QScrollArea *>(ancestor);
    if (scrollArea &&
        (scrollArea->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff &&
         scrollArea->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)) {
      const QRect &r = cursorRect();
      const QPoint &c = mapTo(scrollArea->widget(), r.center());
      scrollArea->ensureVisible(c.x(), c.y());
      break;
    }
    ancestor = ancestor->parent();
  }
}
