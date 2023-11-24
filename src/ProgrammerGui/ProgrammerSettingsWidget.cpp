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
#include "ProgrammerSettingsWidget.h"

#include <QSettings>

#include "ui_ProgrammerSettingsWidget.h"

namespace FOEDAG {

ProgrammerSettingsWidget::ProgrammerSettingsWidget(
    const ProgrammerSettings &pSettings, QSettings &settings, QWidget *parent)
    : Dialog(parent),
      ui(new Ui::ProgrammerSettingsWidget),
      m_settings(settings) {
  ui->setupUi(this);
  initDialogBox(ui->gridLayout, Dialog::Ok | Dialog::Cancel);
  connect(this, &ProgrammerSettingsWidget::accepted, this,
          &ProgrammerSettingsWidget::apply);

  // disable for now
  ui->tabWidget->removeTab(3);
  ui->tabWidget->removeTab(2);
  ui->pushButtonAddDevice->hide();
  ui->pushButtonRemoveDevice->hide();
  ui->pushButtonEditDevice->hide();

  for (auto &[cable, freq] : pSettings.frequency.values()) {
    ui->comboBoxHw->addItem(QString::fromStdString(cable.name));
    ui->spinBoxFreq->setRange(freq, freq);
  }

  for (auto deviceInfo : pSettings.devices) {
    int rowIndex = ui->tableWidgetDevices->rowCount();
    ui->tableWidgetDevices->insertRow(rowIndex);
    ui->tableWidgetDevices->setItem(
        rowIndex, 0,
        new QTableWidgetItem{QString::fromStdString(deviceInfo->dev.name)});
    ui->tableWidgetDevices->setItem(
        rowIndex, 1,
        new QTableWidgetItem{ToHexString(deviceInfo->dev.tapInfo.idCode)});
    ui->tableWidgetDevices->setItem(
        rowIndex, 2,
        new QTableWidgetItem{ToHexString(deviceInfo->dev.tapInfo.irMask)});
    ui->tableWidgetDevices->setItem(
        rowIndex, 3,
        new QTableWidgetItem{QString::number(deviceInfo->dev.tapInfo.irLen)});
  }
}

ProgrammerSettingsWidget::~ProgrammerSettingsWidget() { delete ui; }

void ProgrammerSettingsWidget::openTab(int index) {
  if (index >= 0 && index < ui->tabWidget->count())
    ui->tabWidget->setCurrentIndex(index);
}

void ProgrammerSettingsWidget::apply() {
  m_settings.setValue(HardwareFrequencyKey().arg(ui->comboBoxHw->currentText()),
                      ui->spinBoxFreq->text());
}

}  // namespace FOEDAG
