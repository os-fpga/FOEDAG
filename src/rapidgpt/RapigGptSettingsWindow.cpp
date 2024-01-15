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

#include "ui_RapigGptSettingsWindow.h"

static constexpr auto rapidGptKey{"rapidGpt/ApiKey"};
static constexpr auto rapidGptPrecision{"rapidGpt/Precision"};
static constexpr auto rapidGptInteractivity{"rapidGpt/Interactivity"};
static constexpr auto rapidGptRemote{"rapidGpt/RemoteUrl"};

namespace FOEDAG {

RapigGptSettingsWindow::RapigGptSettingsWindow(QSettings &settings,
                                               QWidget *parent)
    : QDialog{parent},
      ui(new Ui::RapigGptSettingsWindow),
      m_settings(settings) {
  ui->setupUi(this);
  ui->comboBoxPrecision->addItem("More Precise", "0.2");
  ui->comboBoxPrecision->addItem("Balanced (default)", "0.5");
  ui->comboBoxPrecision->addItem("More Creative", "0.7");
  ui->comboBoxInteractivity->addItem("Less Interactive", "-1");
  ui->comboBoxInteractivity->addItem("More Interactive (default)", "1");
  ui->lineEditApiKey->setText(
      m_settings.value(rapidGptKey, QVariant{}).toString());
  ui->lineEditRemoteUrl->setText(
      m_settings.value(rapidGptRemote, QVariant{}).toString());
  auto index = ui->comboBoxPrecision->findData(
      m_settings.value(rapidGptPrecision, QVariant{"0.5"}));
  ui->comboBoxPrecision->setCurrentIndex((index != -1) ? index : 1);
  index = ui->comboBoxInteractivity->findData(
      m_settings.value(rapidGptInteractivity, QVariant{"1"}));
  ui->comboBoxInteractivity->setCurrentIndex((index != -1) ? index : 1);
  auto ok = ui->buttonBox->button(QDialogButtonBox::Ok);
  auto cancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
  connect(ok, &QPushButton::clicked, this, &RapigGptSettingsWindow::accept);
  connect(cancel, &QPushButton::clicked, this, &RapigGptSettingsWindow::reject);
}

RapidGptSettings RapigGptSettingsWindow::fromSettings(
    const QSettings &settings) {
  return {settings.value(rapidGptKey).toString(),
          settings.value(rapidGptPrecision, "0.5").toString(),
          settings.value(rapidGptInteractivity, "1").toString(),
          settings.value(rapidGptRemote).toString()};
}

void RapigGptSettingsWindow::accept() {
  m_settings.setValue(rapidGptKey, ui->lineEditApiKey->text());
  m_settings.setValue(rapidGptPrecision, ui->comboBoxPrecision->currentData());
  m_settings.setValue(rapidGptInteractivity,
                      ui->comboBoxInteractivity->currentData());
  m_settings.setValue(rapidGptRemote, ui->lineEditRemoteUrl->text());
  QDialog::accept();
}

}  // namespace FOEDAG
