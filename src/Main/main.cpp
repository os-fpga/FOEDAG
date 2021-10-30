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

  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    // --script <script>
    if (!GlobalSession->CmdLine()->Script().empty()) {
      int code =
          Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
      if (code >= TCL_ERROR) {
        std::cout << std::string("Tcl Error: " +
                                 std::string(Tcl_GetStringResult(interp)));
      }
    }
    return 0;
  };

  // Start Loop
  Tcl_MainEx(argc, (char**)argv, tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}
