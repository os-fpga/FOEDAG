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
#ifndef SESSION_H
#define SESSION_H

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

namespace FOEDAG {

class ProjectFileLoader;
class Settings;
enum class GUI_TYPE { GT_NONE, GT_WIDGET, GT_QML };

class Session {
 public:
  Session(QWidget *mainWindow, TclInterpreter *interp, CommandStack *stack,
          CommandLine *cmdLine, ToolContext *context, Compiler *compiler,
          Settings *settings)
      : m_mainWindow(mainWindow),
        m_interp(interp),
        m_stack(stack),
        m_cmdLine(cmdLine),
        m_context(context),
        m_compiler(compiler),
        m_settings(settings) {
    m_compiler->SetSession(this);
    m_compiler->SetInterpreter(interp);
  }

  ~Session();
  void MainWindow(QWidget *window) { m_mainWindow = window; }
  QWidget *MainWindow() { return m_mainWindow; }
  TclInterpreter *TclInterp() { return m_interp; }
  CommandStack *CmdStack() { return m_stack; }
  CommandLine *CmdLine() { return m_cmdLine; }
  ToolContext *Context() { return m_context; }
  Compiler *GetCompiler() { return m_compiler; }
  Settings *GetSettings() { return m_settings; }
  void windowShow();
  void windowHide();

  void setGuiType(FOEDAG::GUI_TYPE newGuiType);

  void setWindowModel(MainWindowModel *newWindowModel);

  int ReturnStatus() { return m_returnStatus; }
  void ReturnStatus(int status) { m_returnStatus = status; }

  void ProjectFileLoader(
      std::shared_ptr<FOEDAG::ProjectFileLoader> projectFileLoader);
  std::shared_ptr<FOEDAG::ProjectFileLoader> ProjectFileLoader() const;

 private:
  QWidget *m_mainWindow = nullptr;
  MainWindowModel *m_windowModel = nullptr;
  TclInterpreter *m_interp = nullptr;
  CommandStack *m_stack = nullptr;
  CommandLine *m_cmdLine = nullptr;
  FOEDAG::GUI_TYPE m_guiType = GUI_TYPE::GT_NONE;
  ToolContext *m_context = nullptr;
  Compiler *m_compiler = nullptr;
  Settings *m_settings = nullptr;
  int m_returnStatus =
      0;  // When running in batch mode, executable returned status
  std::shared_ptr<FOEDAG::ProjectFileLoader> m_projectFileLoader;
};

}  // namespace FOEDAG

#endif
