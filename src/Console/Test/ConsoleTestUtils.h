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

#include <QApplication>
#include <QKeyEvent>

#include "StreamBuffer.h"
#include "Tcl/TclInterpreter.h"
#include "TclConsole.h"
#include "TclConsoleBuilder.h"

namespace FOEDAG {

void sendCommand(const QString& command, QObject* receiver) {
  for (auto ch : command) {
    if (ch == '\n')
      QApplication::postEvent(
          receiver,
          new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
    else
      QApplication::postEvent(
          receiver,
          new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, ch));
  }
  QApplication::postEvent(
      receiver,
      new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
}

TclConsoleWidget* Init(Tcl_Interp* interp) {
  TclConsoleWidget* console{nullptr};
  StreamBuffer* buffer = new StreamBuffer;
  QWidget* w = createConsole(
      interp, std::make_unique<TclConsole>(interp, buffer->getStream()), buffer,
      nullptr, &console);
  w->show();
  return console;
}

class StateCheck : public QObject {
  QString m_text;
  TclConsoleWidget* m_console;

 public:
  StateCheck(const QString& textToCheck, FOEDAG::TclConsoleWidget* console)
      : m_text(textToCheck), m_console(console) {
    connect(console, &FOEDAG::TclConsoleWidget::stateChanged, this,
            &StateCheck::stateChanged);
  }
 public slots:
  void stateChanged(FOEDAG::State st) {
    if (st == FOEDAG::State::IDLE) {
      QString consoleText = m_console->toPlainText();
      if (consoleText != m_text) {
        qDebug() << "FAILED";
        qDebug() << "Expected: " << m_text;
        qDebug() << "Actual:   " << consoleText;
        exit(1);
      } else {
        qDebug() << "SUCCESS";
      }
    }
  }
};

#define CHECK_EXPECTED(cmd, expectedOut)                                    \
  FOEDAG::StateCheck* check = new FOEDAG::StateCheck{expectedOut, console}; \
  Q_UNUSED(check)                                                           \
  sendCommand(cmd, console);

}  // namespace FOEDAG
