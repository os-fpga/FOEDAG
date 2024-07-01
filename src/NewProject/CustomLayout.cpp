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

QString CustomLayout::toolName() { return "Rapid eFPGA configurator"; }

CustomLayout::CustomLayout(const QStringList &baseDevices,
                           const QStringList &allDevices,
                           const std::function<void(const QString &)> &logger,
                           QWidget *parent)
    : QDialog(parent), ui(new Ui::CustomLayout) {
  ui->setupUi(this);
  setWindowTitle(QString{"%1: Create new device..."}.arg(toolName()));
  QStringList filteredDevices{};
  std::copy_if(baseDevices.begin(), baseDevices.end(),
               std::back_inserter(filteredDevices),
               [](const QString &device) { return device.startsWith('e'); });
  ui->comboBox->insertItems(0, filteredDevices);
  m_isNameValid = [this, allDevices, filteredDevices]() -> QString {
    if (filteredDevices.isEmpty()) return "No base device";
    auto name = ui->lineEditName->text();
    if (name.isEmpty()) return "Please specify name";
    if (allDevices.contains(name))
      return "This name is already used, please choose another";
    return QString{};
  };
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &CustomLayout::close);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this, logger]() {
    if (logger) {
      logger(
          QString{"\"%1\" created from \"%2\" with the following parameters:"}
              .arg(ui->lineEditName->text(), ui->comboBox->currentText()));
      logger(QString{
          "Aspect Ratio: %1, DSP: %2, BRAM: %3, LE: %4, FLE: %5, CLB: %6"}
                 .arg(ui->doubleSpinBoxAR->cleanText(),
                      ui->spinBoxDsp->cleanText(), ui->spinBoxBram->cleanText(),
                      ui->spinBoxLE->cleanText(), ui->spinBoxFLE->cleanText(),
                      ui->spinBoxClb->cleanText()));
      QStringList resources{};
      for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        resources += QString{"%1: %2"}.arg(
            ui->tableWidget->verticalHeaderItem(row)->text(),
            ui->tableWidget->item(row, 0)->text());
      }
      logger(QString{"Resources: %1"}.arg(resources.join(", ")));
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
  connect(ui->lineEditName, &QLineEdit::textChanged, this,
          &CustomLayout::updateRunTimeResources);
  auto spinBoxes = findChildren<QSpinBox *>();
  for (auto spinBox : spinBoxes) {
    connect(spinBox, &QSpinBox::valueChanged, this,
            &CustomLayout::updateRunTimeResources);
    spinBox->setMaximum(std::numeric_limits<int>::max() /
                        1000);  // avoid type overflow issue
  }
  ui->lineEditName->setValidator(new QRegularExpressionValidator{
      QRegularExpression{"^(?![-_])[0-9a-zA-Z-_]+"}, this});
  ui->tableWidget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);
  ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
  for (int row = 0; row < ui->tableWidgetStatus->rowCount(); row++) {
    for (int col = 0; col < ui->tableWidgetStatus->columnCount(); col++)
      ui->tableWidgetStatus->setItem(row, col, new QTableWidgetItem{});
  }
  for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
    for (int col = 0; col < ui->tableWidget->columnCount(); col++)
      ui->tableWidget->setItem(row, col, new QTableWidgetItem{});
  }
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
  QStringList tableData = {QString::number(efpga.columns()),
                           QString::number(efpga.height()),
                           QString::number(efpga.need() * 100, 'f', 1),
                           QString::number(efpga.actual() * 100, 'f', 1),
                           QString::number(efpga.lutCount()),
                           QString::number(efpga.ffCount()),
                           QString::number(efpga.dspCount()),
                           QString::number(efpga.bramCount())};
  for (int i = 0; i < tableData.size(); i++)
    ui->tableWidget->item(i, 0)->setText(tableData.at(i));

  ui->tableWidgetStatus->item(0, 0)->setText(
      efpga.isBlockCountValid() && efpga.isLutCountValid() ? "Pass" : "Fail");
  QString errorText = {};
  if (!efpga.isBlockCountValid())
    errorText = "Increase CLBs or decrease non-CLBs";
  else if (!efpga.isLutCountValid())
    errorText = "LUT count must be greater than 0";
  ui->tableWidgetStatus->item(0, 1)->setText(errorText);

  ui->tableWidgetStatus->item(1, 0)->setText(
      efpga.isDeviceSizeValid() ? "Pass" : "Fail");
  ui->tableWidgetStatus->item(1, 1)->setText(
      efpga.isDeviceSizeValid()
          ? ""
          : "Device size must be between 1x1 and 160x160");

  QString nameStatus = m_isNameValid();

  ui->tableWidgetStatus->item(2, 0)->setText(nameStatus.isEmpty() ? "Pass"
                                                                  : "Fail");
  ui->tableWidgetStatus->item(2, 1)->setText(nameStatus.isEmpty() ? ""
                                                                  : nameStatus);

  bool passStatus = true;
  for (int i = 0; i < ui->tableWidgetStatus->rowCount(); i++) {
    if (ui->tableWidgetStatus->item(i, 0)->text() != "Pass") {
      passStatus = false;
      break;
    }
  }
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okButton) okButton->setEnabled(passStatus);
}

}  // namespace FOEDAG
