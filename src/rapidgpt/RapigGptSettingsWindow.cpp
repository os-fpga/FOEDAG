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
#include "RapigGptSettingsWindow.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

static constexpr auto rapidGptKey{"rapidGptKey"};

namespace FOEDAG {

RapigGptSettingsWindow::RapigGptSettingsWindow(QSettings &settings,
                                               QWidget *parent)
    : QDialog{parent}, m_apiKeyLineEdit(new QLineEdit), m_settings(settings) {
  auto lineLayout = new QHBoxLayout;
  lineLayout->addWidget(new QLabel{"API Key:"});
  m_apiKeyLineEdit->setText(
      m_settings.value(rapidGptKey, QVariant{}).toString());
  lineLayout->addWidget(m_apiKeyLineEdit);
  auto buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  auto ok = buttons->button(QDialogButtonBox::Ok);
  auto cancel = buttons->button(QDialogButtonBox::Cancel);
  connect(ok, &QPushButton::clicked, this, &RapigGptSettingsWindow::accept);
  connect(cancel, &QPushButton::clicked, this, &RapigGptSettingsWindow::reject);
  auto layout = new QVBoxLayout;
  layout->addLayout(lineLayout);
  layout->addWidget(buttons);
  setLayout(layout);
}

void RapigGptSettingsWindow::SetApiKey(const QString &key) {
  m_apiKeyLineEdit->setText(key);
}

QString RapigGptSettingsWindow::ApiKey() const {
  return m_apiKeyLineEdit->text();
}

void RapigGptSettingsWindow::accept() {
  m_settings.setValue(rapidGptKey, m_apiKeyLineEdit->text());
  QDialog::accept();
}

}  // namespace FOEDAG
