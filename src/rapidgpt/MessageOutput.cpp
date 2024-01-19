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
#include "MessageOutput.h"

#include <QDateTime>
#include <QPainter>
#include <QStyleOption>

#include "ui_MessageOutput.h"

namespace FOEDAG {

MessageOutput::MessageOutput(const Message &message, QWidget *parent)
    : QWidget(parent), ui(new Ui::MessageOutput) {
  ui->setupUi(this);
  ui->labelText->setText(message.content);

  setStyleSheet(R"(
  QWidget {
    border-width: 1px;
    border-radius: 6px;
    background-color: rgb(230, 230, 230)
  }
  QToolButton:hover#toolButtonDelete {
    background-color:white;
  }
  QToolButton:hover#toolButtonRegenerate {
    background-color:white;
  }
  QToolButton:hover#toolButtonEdit {
    background-color:white;
  }
)");

  ui->labelTime->setText(message.date);
  if (message.delay != 0)
    ui->labelDelay->setText(
        QString{"%1 Sec"}.arg(QString::number(message.delay, 'g', 3)));
  ui->labelUser->setText(message.role);
  connect(ui->toolButtonDelete, &QToolButton::clicked, this,
          [this]() { emit buttonPressed(ButtonFlag::Delete); });

  connect(ui->toolButtonEdit, &QToolButton::clicked, this,
          [this]() { emit buttonPressed(ButtonFlag::Edit); });

  connect(ui->toolButtonRegenerate, &QToolButton::clicked, this,
          [this]() { emit buttonPressed(ButtonFlag::Regenerate); });
}

MessageOutput::~MessageOutput() { delete ui; }

QString MessageOutput::text() const { return ui->labelText->toPlainText(); }

void MessageOutput::setButtonFlags(ButtonFlags flags) {
  m_buttonFlags = flags;
  ui->toolButtonEdit->setVisible((flags & ButtonFlag::Edit) != 0);
  ui->toolButtonRegenerate->setVisible((flags & ButtonFlag::Regenerate) != 0);
  ui->toolButtonDelete->setVisible((flags & ButtonFlag::Delete) != 0);
}

void MessageOutput::paintEvent(QPaintEvent *) {
  QStyleOption opt;
  opt.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

}  // namespace FOEDAG
