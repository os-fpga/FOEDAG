// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH
// Qt-GPL-exception-1.0
#pragma once

#include <QTextEdit>

/*
  Automatically adapt height to document contents
 */
class ExpandingTextEdit : public QTextEdit {
  Q_OBJECT
 public:
  explicit ExpandingTextEdit(QWidget *parent = 0);
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
 private slots:
  void updateHeight(const QSizeF &documentSize);
  void reallyEnsureCursorVisible();

 private:
  int m_minimumHeight;
};
