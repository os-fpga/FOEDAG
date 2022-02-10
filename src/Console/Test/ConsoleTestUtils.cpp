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
}

TclConsoleWidget *InitConsole(void *clientData) {
  FOEDAG::TclConsoleWidget *console =
      static_cast<FOEDAG::TclConsoleWidget *>(clientData);
  console->clearText();
  return console;
}

StateCheck::~StateCheck() {
  if (!m_pass) {
    testFail(
        "FAILED\nSomething goes wrong with events. We haven't received console "
        "IDLE state.");
  }
}

void StateCheck::testFail(const QString &message) {
  if (!message.isEmpty()) qDebug().noquote() << message;
  ::exit(1);
}

}  // namespace FOEDAG
