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
#include "CustomLayout.h"

#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QToolTip>
#include <limits>

#include "ui_CustomLayout.h"

namespace FOEDAG {

CustomLayout::CustomLayout(const QStringList &baseDevices,
                           const QStringList &allDevices, QWidget *parent)
    : QDialog(parent), ui(new Ui::CustomLayout) {
  ui->setupUi(this);
  setWindowTitle("Create new device...");
  ui->comboBox->insertItems(0, baseDevices);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &CustomLayout::close);
  connect(
      ui->buttonBox, &QDialogButtonBox::accepted, this, [this, allDevices]() {
        if (ui->lineEditName->text().isEmpty()) {
          QToolTip::showText(ui->lineEditName->mapToGlobal(QPoint(0, 0)),
                             "Please specify name", ui->lineEditName);
          return;
        }
        if (allDevices.contains(ui->lineEditName->text())) {
          QToolTip::showText(ui->lineEditName->mapToGlobal(QPoint(0, 0)),
                             "This name is already used, please choose another",
                             ui->lineEditName);
          return;
        }

        CustomLayoutData data{
            ui->comboBox->currentText(), ui->lineEditName->text(),
            EFpga{ui->doubleSpinBoxAR->value(), ui->spinBoxBram->value(),
                  ui->spinBoxDsp->value(), ui->spinBoxLE->value(),
                  ui->spinBoxFLE->value(), ui->spinBoxClb->value()}};
        emit sendCustomLayoutData(data);
        accept();
      });
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &CustomLayout::reject);
  setObjectName("CustomLayout");
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okButton) okButton->setObjectName("CustomLayoutOk");

  ui->doubleSpinBoxAR->setMinimum(0.7);
  ui->doubleSpinBoxAR->setMaximum(1.4);
  ui->doubleSpinBoxAR->setSingleStep(0.1);

  connect(ui->doubleSpinBoxAR, &QDoubleSpinBox::valueChanged, this,
          &CustomLayout::updateRunTimeResources);
  auto spinBoxes = findChildren<QSpinBox *>();
  for (auto spinBox : spinBoxes) {
    connect(spinBox, &QSpinBox::valueChanged, this,
            &CustomLayout::updateRunTimeResources);
    spinBox->setMaximum(std::numeric_limits<int>::max() /
                        1000);  // TODO, update this to avoid type
                                // overflow issue
  }
  ui->tableWidget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);
  ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
  updateRunTimeResources();
}

CustomLayout::~CustomLayout() { delete ui; }

void CustomLayout::setCustomLayoutData(const CustomLayoutData &newData) {
  ui->lineEditName->setText(newData.name);
  ui->doubleSpinBoxAR->setValue(newData.eFpga.aspectRatio);
  ui->spinBoxBram->setValue(newData.eFpga.bram);
  ui->spinBoxClb->setValue(newData.eFpga.clb);
  ui->spinBoxDsp->setValue(newData.eFpga.dsp);
  ui->spinBoxFLE->setValue(newData.eFpga.fle);
  ui->spinBoxLE->setValue(newData.eFpga.le);
  setBaseDevice(newData.baseName);
}

void CustomLayout::setBaseDevice(const QString &baseDevice) {
  int index = ui->comboBox->findData(baseDevice, Qt::DisplayRole);
  if (index != -1) ui->comboBox->setCurrentIndex(index);
}

void CustomLayout::updateRunTimeResources() {
  EFpga data{ui->doubleSpinBoxAR->value(), ui->spinBoxBram->value(),
             ui->spinBoxDsp->value(),      ui->spinBoxLE->value(),
             ui->spinBoxFLE->value(),      ui->spinBoxClb->value()};
  EFpgaMath efpga{data};
  // ui->tableWidget->item(0,
  // 0)->setText(QString::number(deviceRes.lutsCount())); // TODO
  // ui->tableWidget->item(0,
  // 1)->setText(QString::number(deviceRes.ffsCount())); // TODO
  ui->tableWidget->item(0, 2)->setText(QString::number(efpga.bramCount()));
  ui->tableWidget->item(0, 3)->setText(QString::number(efpga.dspCount()));
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okButton) okButton->setEnabled(efpga.isBlockCountValid());
  ui->labelWarning->setVisible(!efpga.isBlockCountValid());
  ui->labelWarning->setText("Error: increase CLBs or decrease non-CLBs");
}

}  // namespace FOEDAG
