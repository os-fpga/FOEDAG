/*
 Copyright 2021 The Foedag team

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <QMainWindow>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Tcl/TclInterpreter.h"

#ifndef SESSION_H
#define SESSION_H

namespace FOEDAG {

class Session {
 private:
 public:
  Session(QMainWindow *mainWindow, TclInterpreter *interp, CommandStack *stack)
      : m_mainWindow(mainWindow), m_interp(interp), m_stack(stack) {}

  ~Session();

  QMainWindow *MainWindow() { return m_mainWindow; }
  TclInterpreter *TclInterp() { return m_interp; }
  CommandStack *CmdStack() { return m_stack; }

 private:
  QMainWindow *m_mainWindow;
  TclInterpreter *m_interp;
  CommandStack *m_stack;
};

}  // namespace FOEDAG

#endif
