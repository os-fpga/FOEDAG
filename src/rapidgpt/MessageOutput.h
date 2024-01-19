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

#include <QWidget>

#include "RapidGptContext.h"

namespace Ui {
class MessageOutput;
}
class Label;
namespace FOEDAG {

class MessageOutput : public QWidget {
  Q_OBJECT

 public:
  enum ButtonFlag { None = 0, Edit = 1, Regenerate = 2, Delete = 4 };
  Q_DECLARE_FLAGS(ButtonFlags, ButtonFlag)
  Q_FLAG(ButtonFlags)
  explicit MessageOutput(const Message &message, QWidget *parent = nullptr);
  ~MessageOutput() override;
  QString text() const;

  void setButtonFlags(ButtonFlags flags);

 signals:
  void buttonPressed(ButtonFlag button);

 protected:
  void paintEvent(QPaintEvent *) override;

 private:
  Ui::MessageOutput *ui;
  ButtonFlags m_buttonFlags{None};
};

}  // namespace FOEDAG

Q_DECLARE_OPERATORS_FOR_FLAGS(FOEDAG::MessageOutput::ButtonFlags)
