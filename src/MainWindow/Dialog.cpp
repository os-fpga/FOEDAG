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
#include "Dialog.h"

#include <QDialogButtonBox>
#include <QLayout>

namespace FOEDAG {

Dialog::Dialog(QWidget *parent) : QDialog(parent) {}

QDialogButtonBox *Dialog::initDialogBox(QLayout *layout, Buttons buttons) {
  QDialogButtonBox::StandardButtons btns;
  if (buttons & Ok) btns |= QDialogButtonBox::Ok;
  if (buttons & Yes) btns |= QDialogButtonBox::Yes;
  if (buttons & No) btns |= QDialogButtonBox::No;
  if (buttons & Cancel) btns |= QDialogButtonBox::Cancel;
  auto buttonBox = new QDialogButtonBox{btns};
  connect(buttonBox, &QDialogButtonBox::accepted, this, &Dialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &Dialog::reject);
  layout->addWidget(buttonBox);
  return buttonBox;
}

}  // namespace FOEDAG
