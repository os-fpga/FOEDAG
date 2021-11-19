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

#include <QMainWindow>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Compiler/WorkerThread.h"
#include "Main/CommandLine.h"
#include "Tcl/TclInterpreter.h"

#ifndef SESSION_H
#define SESSION_H

namespace FOEDAG {

class Session {
 private:
 public:
  Session(QWidget *mainWindow, TclInterpreter *interp, CommandStack *stack,
          CommandLine *cmdLine)
      : m_mainWindow(mainWindow),
        m_interp(interp),
        m_stack(stack),
        m_cmdLine(cmdLine) {}

  ~Session();

  QWidget *MainWindow() { return m_mainWindow; }
  TclInterpreter *TclInterp() { return m_interp; }
  CommandStack *CmdStack() { return m_stack; }
  CommandLine *CmdLine() { return m_cmdLine; }

 private:
  QWidget *m_mainWindow;
  TclInterpreter *m_interp;
  CommandStack *m_stack;
  CommandLine *m_cmdLine;
};

}  // namespace FOEDAG

#endif
