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
#include "Main/ToolContext.h"
#include "MainWindow/mainwindowmodel.h"
#include "Tcl/TclInterpreter.h"

#ifndef SESSION_H
#define SESSION_H

namespace FOEDAG {

enum class GUI_TYPE { GT_NONE, GT_WIDGET, GT_QML };

class Session {
 public:
  Session(QWidget *mainWindow, TclInterpreter *interp, CommandStack *stack,
          CommandLine *cmdLine, ToolContext *context)
      : m_mainWindow(mainWindow),
        m_interp(interp),
        m_stack(stack),
        m_cmdLine(cmdLine),
        m_context(context) {}

  ~Session();

  QWidget *MainWindow() { return m_mainWindow; }
  TclInterpreter *TclInterp() { return m_interp; }
  CommandStack *CmdStack() { return m_stack; }
  CommandLine *CmdLine() { return m_cmdLine; }
  const ToolContext *Context() { return m_context; }
  void windowShow();
  void windowHide();

  void setGuiType(FOEDAG::GUI_TYPE newGuiType);

  void setWindowModel(MainWindowModel *newWindowModel);

 private:
  QWidget *m_mainWindow = nullptr;
  MainWindowModel *m_windowModel = nullptr;
  TclInterpreter *m_interp = nullptr;
  CommandStack *m_stack = nullptr;
  CommandLine *m_cmdLine = nullptr;
  FOEDAG::GUI_TYPE m_guiType = GUI_TYPE::GT_NONE;
  ToolContext *m_context = nullptr;
};

}  // namespace FOEDAG

#endif
