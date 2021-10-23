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
#include <tcl.h>

#include <QApplication>
#include <QLabel>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "foedag.h"
#include "qttclnotifier.hpp"

using namespace FOEDAG;

FOEDAG::Session* GlobalSession;

void registerTclCommands(Session* session) {
  auto gui_start = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    GlobalSession->MainWindow()->show();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_start", gui_start, 0, 0);

  auto gui_stop = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    GlobalSession->MainWindow()->hide();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_stop", gui_stop, 0, 0);
}

int main(int argc, char** argv) {
  // Do not run Qt when option "-noqt" is specified
  if (argc >= 2) {
    if (std::string(argv[1]) == "-noqt") {
      TclInterpreter interpreter(argv[0]);
      std::string result =
          interpreter.evalCmd("puts \"Hello Foedag, you have Tcl!\"");
      if (result != "") {
        std::cout << result << '\n';
      }
      return 0;
    }
  }
  QApplication app(argc, (char**)argv);
  MainWindow* main_win = new MainWindow;
  TclInterpreter* interpreter = new TclInterpreter(argv[0]);
  CommandStack* commands = new CommandStack(interpreter);
  GlobalSession = new Session(main_win, interpreter, commands);
  registerTclCommands(GlobalSession);

  QtTclNotify::QtTclNotifier::setup();  // registers my notifier with Tcl

  // Tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // Dummy Tcl_AppInit
  auto tcl_init = [](Tcl_Interp*) -> int { return 0; };

  // Start Loop
  Tcl_MainEx(argc, (char**)argv, tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}
