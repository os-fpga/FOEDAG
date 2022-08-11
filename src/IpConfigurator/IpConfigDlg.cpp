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
#include "IpConfigurator/IpConfigDlg.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace FOEDAG;

IpConfigDlg::IpConfigDlg(QWidget* parent /*nullptr*/) {
  this->setWindowTitle("Configure IP");
  this->setObjectName("IpConfigDlg");
  QVBoxLayout* topLayout = new QVBoxLayout();
  this->setLayout(topLayout);

  topLayout->addStretch();

  // Dialog Buttons
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Cancel);
  btns->setObjectName("IpConfigDlg_QDialogButtonBox");
  topLayout->addWidget(btns);
  QPushButton* instantiateBtn = new QPushButton("Instantiate IP", this);
  btns->addButton(instantiateBtn, QDialogButtonBox::ButtonRole::ActionRole);
}
