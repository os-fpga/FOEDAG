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

static const QString heightErrorMessage =
    "Please correct the Height parameter and try again.<br>"
    "<b>Note</b>"
    "<ul><li>If there are DSPs or BRAMs:</li><p>Height should be a multiple of "
    "k: "
    "H = (k * 3), where k is 1, 2..n. So Height can only be 3, 6, 9...</p>"
    "<li>If there are no DSPs or BRAMs:</li>"
    "<p>Height can only be 3, 4, 5...</p></ul>";

CustomLayout::CustomLayout(const QStringList &baseDevices,
                           const QStringList &allDevices, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::CustomLayout),
      m_validator(new QRegularExpressionValidator{
          QRegularExpression{"([0-9]+,)+"}, this}) {
  ui->setupUi(this);
  setWindowTitle("Create new device...");
  ui->comboBox->insertItems(0, baseDevices);
  ui->lineEditDsp->setValidator(m_validator);
  ui->lineEditBram->setValidator(m_validator);
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
        if (ui->spinBoxWidth->value() == 0) {
          QToolTip::showText(ui->spinBoxWidth->mapToGlobal(QPoint(0, 0)),
                             "Please specify Width", this);
          return;
        }
        if (ui->spinBoxHeight->value() == 0) {
          QToolTip::showText(ui->spinBoxHeight->mapToGlobal(QPoint(0, 0)),
                             "Please specify Height", this);
          return;
        }
        CustomLayoutData data{
            ui->comboBox->currentText(), ui->lineEditName->text(),
            ui->spinBoxWidth->value(),   ui->spinBoxHeight->value(),
            ui->lineEditBram->text(),    ui->lineEditDsp->text()};
        CustomDeviceResources deviceRes{data};
        if (!deviceRes.isHeightValid()) {
          QMessageBox::critical(this, "Invalid Height parameters",
                                heightErrorMessage);
          return;
        }
        if (!deviceRes.isValid()) {
          QMessageBox::critical(this, "Invalid Parameters",
                                "Please correct the parameters and try again.");
          return;
        }
        emit sendCustomLayoutData(data);
        accept();
      });
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &CustomLayout::reject);
  setObjectName("CustomLayout");
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okButton) okButton->setObjectName("CustomLayoutOk");
  ui->spinBoxWidth->setMaximum(std::numeric_limits<int>::max());
  ui->spinBoxHeight->setMaximum(std::numeric_limits<int>::max());

  ui->lineEditName->setValidator(new QRegularExpressionValidator{
      QRegularExpression{"^(?![-_])[0-9a-zA-Z-_]+"}, this});
  ui->spinBoxHeight->setMinimum(3);
  ui->spinBoxWidth->setMinimum(3);

  connect(ui->spinBoxWidth, &QSpinBox::valueChanged, this,
          &CustomLayout::updateRunTimeResources);
  connect(ui->spinBoxHeight, &QSpinBox::valueChanged, this,
          &CustomLayout::updateRunTimeResources);
  connect(ui->lineEditDsp, &QLineEdit::textChanged, this,
          &CustomLayout::updateRunTimeResources);
  connect(ui->lineEditBram, &QLineEdit::textChanged, this,
          &CustomLayout::updateRunTimeResources);
  ui->tableWidget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);
  ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
  updateRunTimeResources();
}

CustomLayout::~CustomLayout() { delete ui; }

void CustomLayout::setCustomLayoutData(const CustomLayoutData &newData) {
  ui->lineEditName->setText(newData.name);
  ui->lineEditBram->setText(newData.bram);
  ui->lineEditDsp->setText(newData.dsp);
  ui->spinBoxHeight->setValue(newData.height);
  ui->spinBoxWidth->setValue(newData.width);
  setBaseDevice(newData.baseName);
}

void CustomLayout::setBaseDevice(const QString &baseDevice) {
  int index = ui->comboBox->findData(baseDevice, Qt::DisplayRole);
  if (index != -1) ui->comboBox->setCurrentIndex(index);
}

void CustomLayout::updateRunTimeResources() {
  CustomLayoutData data{ui->comboBox->currentText(), ui->lineEditName->text(),
                        ui->spinBoxWidth->value(),   ui->spinBoxHeight->value(),
                        ui->lineEditBram->text(),    ui->lineEditDsp->text()};
  CustomDeviceResources deviceRes{data};
  ui->tableWidget->item(0, 0)->setText(QString::number(deviceRes.lutsCount()));
  ui->tableWidget->item(0, 1)->setText(QString::number(deviceRes.ffsCount()));
  ui->tableWidget->item(0, 2)->setText(QString::number(deviceRes.bramCount()));
  ui->tableWidget->item(0, 3)->setText(QString::number(deviceRes.dspCount()));
  ui->tableWidget->item(0, 4)->setText(
      QString::number(deviceRes.carryLengthCount()));
  static const QColor error{242, 136, 168};
  if (deviceRes.lutsCount() > 0) {
    ui->tableWidget->item(0, 0)->setBackground(QBrush{});
    ui->tableWidget->item(0, 0)->setToolTip({});
  } else {
    ui->tableWidget->item(0, 0)->setBackground(QBrush{error});
    ui->tableWidget->item(0, 0)->setToolTip(
        "LUT count must be bigger then 0. Please correct the parameters and "
        "try again.");
  }
  if (deviceRes.ffsCount() > 0) {
    ui->tableWidget->item(0, 1)->setBackground(QBrush{});
    ui->tableWidget->item(0, 1)->setToolTip({});
  } else {
    ui->tableWidget->item(0, 1)->setBackground(QBrush{error});
    ui->tableWidget->item(0, 1)->setToolTip(
        "FF count must be bigger then 0. Please correct the parameters and "
        "try again.");
  }
  if (!deviceRes.isHeightValid() && !ui->lineEditBram->text().isEmpty()) {
    ui->tableWidget->item(0, 2)->setBackground(QBrush{error});
    ui->tableWidget->item(0, 2)->setToolTip(heightErrorMessage);
  } else {
    ui->tableWidget->item(0, 2)->setBackground(QBrush{});
    ui->tableWidget->item(0, 2)->setToolTip({});
  }
  if (!deviceRes.isHeightValid() && !ui->lineEditDsp->text().isEmpty()) {
    ui->tableWidget->item(0, 3)->setBackground(QBrush{error});
    ui->tableWidget->item(0, 3)->setToolTip(heightErrorMessage);
  } else {
    ui->tableWidget->item(0, 3)->setBackground(QBrush{});
    ui->tableWidget->item(0, 3)->setToolTip({});
  }
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okButton) okButton->setEnabled(deviceRes.isValid());
}

}  // namespace FOEDAG
