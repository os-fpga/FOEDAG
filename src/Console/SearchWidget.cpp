/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SearchWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>

namespace FOEDAG {

SearchWidget::SearchWidget(QTextEdit *searchEdit, QWidget *parent,
                           Qt::WindowFlags f)
    : QWidget(parent, f), m_searchEdit(searchEdit) {
  QGridLayout *layout = new QGridLayout;
  layout->setContentsMargins(0, 6, 6, 6);
  QLineEdit *edit = new QLineEdit{this};
  connect(edit, &QLineEdit::textChanged, this, [this](const QString &text) {
    m_textToSearch = text;
    findNext();
  });
  edit->installEventFilter(this);
  layout->addWidget(edit);
  m_edit = edit;
  QIcon closeIcon = style()->standardIcon(QStyle::SP_DockWidgetCloseButton);
  QPushButton *closeBtn = new QPushButton{this};
  connect(closeBtn, &QPushButton::clicked, this, [this]() {
    m_enableSearch = false;
    hide();
  });
  closeBtn->setIcon(closeIcon);
  layout->addWidget(closeBtn, 0, 2);

  QCheckBox *findWholeWords = new QCheckBox{this};
  findWholeWords->setText(tr("Whole word"));
  connect(findWholeWords, &QCheckBox::stateChanged, this, [this](int state) {
    m_searchFlags.setFlag(QTextDocument::FindFlag::FindWholeWords,
                          state == Qt::Checked);
    findNext();
  });
  QGridLayout *checksLayout = new QGridLayout;
  checksLayout->addWidget(findWholeWords, 0, 0);

  QCheckBox *findCaseSensitively = new QCheckBox{this};
  findCaseSensitively->setText(tr("Case sensitive"));
  connect(findCaseSensitively, &QCheckBox::stateChanged, this,
          [this](int state) {
            m_searchFlags.setFlag(QTextDocument::FindFlag::FindCaseSensitively,
                                  state == Qt::Checked);
            findNext();
          });
  checksLayout->addWidget(findCaseSensitively, 0, 1);

  QCheckBox *findBackward = new QCheckBox{this};
  findBackward->setText(tr("Backward"));
  connect(findBackward, &QCheckBox::stateChanged, this, [this](int state) {
    m_searchFlags.setFlag(QTextDocument::FindFlag::FindBackward,
                          state == Qt::Checked);
    findNext();
  });
  checksLayout->addWidget(findBackward, 1, 0);
  checksLayout->setColumnStretch(1, 1);
  layout->addLayout(checksLayout, 1, 0);

  QPushButton *nextBtn = new QPushButton{this};
  nextBtn->setText(tr("Next"));
  nextBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  connect(nextBtn, &QPushButton::clicked, this, &SearchWidget::findNext);
  layout->addWidget(nextBtn, 0, 1);

  setLayout(layout);
  hide();
}

void SearchWidget::search() {
  m_enableSearch = true;
  show();
  m_edit->selectAll();
  m_edit->setFocus();
}

bool SearchWidget::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    if (auto keyEvent = dynamic_cast<QKeyEvent *>(event)) {
      switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
          findNext();
          break;
        case Qt::Key_Escape:
          hide();
          break;
        default:
          break;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void SearchWidget::findNext() {
  if (!m_searchEdit) return;

  if (m_enableSearch && !m_textToSearch.isEmpty()) {
    m_edit->setStyleSheet(QString());
    if (m_searchEdit->find(m_textToSearch, m_searchFlags) == false) {
      if (m_searchFlags & QTextDocument::FindFlag::FindBackward)
        m_searchEdit->moveCursor(QTextCursor::End);
      else
        m_searchEdit->moveCursor(QTextCursor::Start);
    }
    bool found = m_searchEdit->find(m_textToSearch, m_searchFlags);
    if (!found && !m_searchEdit->textCursor().hasSelection()) {
      m_edit->setStyleSheet("QLineEdit:focus{background-color: #F0B8C4;}");
    }
  }
}

}  // namespace FOEDAG
