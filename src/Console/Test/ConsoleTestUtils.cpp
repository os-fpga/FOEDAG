/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "ConsoleTestUtils.h"

namespace FOEDAG {

void sendCommand(const QString &command, QObject *receiver) {
  for (auto ch : command) {
    if (ch == '\n')
      QApplication::postEvent(
          receiver,
          new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
    else if (ch == controlC)
      QApplication::postEvent(
          receiver,
          new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, ch));
    else
      QApplication::postEvent(
          receiver,
          new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, ch));
  }
  QApplication::postEvent(
      receiver,
      new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
}

TclConsoleWidget *InitConsole(Tcl_Interp *interp) {
  TclConsoleWidget *console{nullptr};
  StreamBuffer *buffer = new StreamBuffer;
  QWidget *w = createConsole(
      interp, std::make_unique<TclConsole>(interp, buffer->getStream()), buffer,
      nullptr, &console);
  w->show();
  return console;
}

}  // namespace FOEDAG
