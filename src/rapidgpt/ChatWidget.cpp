/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#include "ChatWidget.h"

#include <QDesktopServices>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QSpacerItem>

#include "MessageOutput.h"
#include "ui_ChatWidget.h"

namespace FOEDAG {

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ChatWidget),
      m_spacer(new QSpacerItem{0, 0, QSizePolicy::Expanding}) {
  ui->setupUi(this);
  connect(ui->toolButtonSend, &QToolButton::clicked, this,
          &ChatWidget::buttonClicked);
  connect(ui->toolButtonDeleteAll, &QToolButton::clicked, this,
          &ChatWidget::cleanHistory);
  connect(ui->label, &QLabel::linkActivated, this, &ChatWidget::openLink);

  auto scrollBar = ui->scrollArea->verticalScrollBar();
  connect(scrollBar, &QScrollBar::rangeChanged, this,
          [scrollBar](int min, int max) { scrollBar->setValue(max); });
  // prevent horizontal scroll
  setMinimumWidth(230);
  ui->textEdit->setAcceptRichText(false);
}

ChatWidget::~ChatWidget() { delete ui; }

void ChatWidget::keyPressEvent(QKeyEvent *event) {
  if ((event->key() == Qt::Key_Return) && event->modifiers() == Qt::CTRL) {
    buttonClicked();
    return;
  }
  QWidget::keyPressEvent(event);
}

void ChatWidget::buttonClicked() {
  auto text = ui->textEdit->toPlainText();
  if (!text.isEmpty()) {
    ui->textEdit->clear();
    setEnableToSend(false);
    emit userText(text);
  }
}

void ChatWidget::openLink(const QString &link) {
  QDesktopServices::openUrl(QUrl{link});
}

void ChatWidget::updateMessageButtons() {
  for (int i = 0; i < m_widgets.count(); i++) {
    if ((i % 2) == 0) {
      m_widgets.at(i)->setButtonFlags(MessageOutput::Delete);
    }
    if ((i % 2) == 1) {
      m_widgets.at(i)->setButtonFlags(MessageOutput::None);
    }
  }
  if (m_widgets.count() != 0) {
    int index = m_widgets.count() - ((m_widgets.count() % 2 == 0) ? 2 : 1);
    m_widgets.at(index)->setButtonFlags(MessageOutput::Delete |
                                        MessageOutput::Edit |
                                        MessageOutput::Regenerate);
  }
}

void ChatWidget::editMessage(int index) {
  auto text = m_widgets.at(index)->text();
  ui->textEdit->setText(text);
  emit removeMessageAt(index);
}

void ChatWidget::addMessage(const Message &message) {
  QHBoxLayout *l = new QHBoxLayout;
  auto messageOut = new MessageOutput{message};
  if (m_widgets.count() % 2 == 0) l->addSpacing(20);

  l->addWidget(messageOut);
  if (m_widgets.count() % 2 == 1) l->addSpacing(20);

  ui->tableLayout->insertLayout(ui->tableLayout->count(), l);
  m_widgets.append(messageOut);
  updateMessageButtons();
  connect(messageOut, &MessageOutput::buttonPressed, this,
          [this, messageOut](MessageOutput::ButtonFlag btn) {
            if (!m_enable) return;
            if (btn == MessageOutput::Regenerate) emit regenerateLast();
            if (btn == MessageOutput::Delete)
              emit removeMessageAt(m_widgets.indexOf(messageOut));
            if (btn == MessageOutput::Edit)
              editMessage(m_widgets.indexOf(messageOut));
          });
}

void ChatWidget::clear() {
  qDeleteAll(m_widgets);
  m_widgets.clear();
  while (ui->tableLayout->count() != 0) {
    delete ui->tableLayout->itemAt(0);
  }
  ui->textEdit->clear();
}

int ChatWidget::count() const { return m_widgets.size(); }

void ChatWidget::removeAt(int index) {
  if (index >= 0 && index < m_widgets.count()) {
    auto w = m_widgets.at(index);
    delete w;
    m_widgets.remove(index);
    if (index < ui->tableLayout->count()) delete ui->tableLayout->itemAt(index);
    updateMessageButtons();
  }
}

void ChatWidget::setEnableToSend(bool enable) {
  m_enable = enable;
  ui->textEdit->setEnabled(enable);
  ui->toolButtonSend->setEnabled(enable);
  ui->toolButtonDeleteAll->setEnabled(enable);
}

void ChatWidget::setEnableIncognitoMode(bool enable) {
  ui->label->setVisible(enable);
  if (enable) {
    ui->horizontalLayoutIncognitoMode->removeItem(m_spacer);
  } else {
    ui->horizontalLayoutIncognitoMode->insertSpacerItem(0, m_spacer);
  }
}

}  // namespace FOEDAG
