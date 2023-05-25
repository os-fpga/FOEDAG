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
#pragma once

#include <QDialog>

class QDialogButtonBox;

namespace FOEDAG {

class Dialog : public QDialog {
  Q_OBJECT

 public:
  explicit Dialog(QWidget *parent = nullptr);

  enum Button { Ok = 1, Yes = 2, No = 4, Cancel = 8 };
  Q_DECLARE_FLAGS(Buttons, Button)

 protected:
  [[maybe_unused]] QDialogButtonBox *initDialogBox(QLayout *layout,
                                                   Buttons buttons);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Dialog::Buttons)
}  // namespace FOEDAG
