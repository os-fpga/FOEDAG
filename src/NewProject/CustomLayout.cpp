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

#include <QToolTip>

#include "ui_CustomLayout.h"

namespace FOEDAG {

CustomLayout::CustomLayout(const QStringList &baseDevices,
                           const QStringList &allDevices, QWidget *parent)
    : QDialog(parent), ui(new Ui::CustomLayout) {
  ui->setupUi(this);
  setWindowTitle("Create new device...");
  ui->comboBox->insertItems(0, baseDevices);
  QRegularExpression numberCommaSeparated{"([0-8]+,)+"};
  ui->lineEditDsp->setValidator(
      new QRegularExpressionValidator{numberCommaSeparated});
  ui->lineEditBram->setValidator(
      new QRegularExpressionValidator{numberCommaSeparated});
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
        emit sendCustomLayoutData(
            {ui->comboBox->currentText(), ui->lineEditName->text(),
             ui->spinBoxWidth->value(), ui->spinBoxHeight->value(),
             ui->lineEditBram->text(), ui->lineEditDsp->text()});
        close();
      });
}

CustomLayout::~CustomLayout() { delete ui; }

}  // namespace FOEDAG
