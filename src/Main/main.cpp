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

#if defined(_MSC_VER)
#include <direct.h>
#include <process.h>
#else
#include <sys/param.h>
#include <unistd.h>
#endif

#include <string.h>
#include <sys/stat.h>
extern "C" {
#include <tcl.h>
}

#include <QApplication>
#include <QLabel>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "CommandLine.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "foedag.h"
#include "qttclnotifier.hpp"

FOEDAG::Session* GlobalSession;

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);

  // Batch mode
  if (!cmd->WithQt()) {
    FOEDAG::TclInterpreter interpreter(argv[0]);
    std::string result = interpreter.evalCmd("puts \"Tcl only mode\"");
    // --script <script>
    if (!cmd->Script().empty()) result = interpreter.evalFile(cmd->Script());
    if (result != "") {
      std::cout << result << '\n';
    }
    return 0;
  }

  // Gui mode
  QApplication app(argc, (char**)argv);
  FOEDAG::MainWindow* main_win = new FOEDAG::MainWindow;
  FOEDAG::TclInterpreter* interpreter = new FOEDAG::TclInterpreter(argv[0]);
  FOEDAG::CommandStack* commands = new FOEDAG::CommandStack(interpreter);
  GlobalSession = new FOEDAG::Session(main_win, interpreter, commands, cmd);
  registerTclCommands(GlobalSession);

  QtTclNotify::QtTclNotifier::setup();  // Registers notifier with Tcl

  // Tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // --gui_test <script> Gui replay, register test
  if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
    interpreter->evalGuiTestFile(GlobalSession->CmdLine()->GuiTestScript());
  }

  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    // --script <script>
    if (!GlobalSession->CmdLine()->Script().empty()) {
      Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
    }
    // --gui_test <script> Gui replay, invoke test
    if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
      std::string proc = "call_test";
      Tcl_EvalEx(interp, proc.c_str(), -1, 0);
    }
    return 0;
  };

  // Start Loop
  Tcl_MainEx(argc, (char**)argv, tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}
