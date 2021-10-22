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
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "qttclnotifier.hpp"

/* TODO: Remove this ugly global variable */
MainWindow* main_win = nullptr;

static int GuiCloseCmd(ClientData clientData, Tcl_Interp* interp, int argc,
                       const char** argv) {
  main_win->hide();
  return 0;
}

// static int tcl_eval(std::string cmd) {
//  Tcl_Eval(interp, cmd.c_str());
//}

static int GuiStartCmd(ClientData clientData, Tcl_Interp* interp, int argc,
                       const char** argv) {
  /* Create the main window */
  main_win->show();
  return 0;
}

static int Tcl_AppInit(Tcl_Interp* interp) {
  int Mymodule_Init(Tcl_Interp*);

  //  interpreter.registerCmd("gui_start", GuiStartCmd, 0, nullptr);
  //  CommandStack commands(&interpreter);

  // TODO: register gui close command
  Tcl_CreateCommand(interp, "gui_start", GuiStartCmd, 0, 0);
  Tcl_CreateCommand(interp, "gui_close", GuiCloseCmd, 0, 0);

  if (Tcl_Init(interp) == TCL_ERROR) return TCL_ERROR;

  /* Now initialize our functions */
  // if (Mymodule_Init(interp) == TCL_ERROR)
  //  return TCL_ERROR;
  return TCL_OK;
}

int main(int argc, char** argv) {
  TclInterpreter interpreter(argv[0]);

  // Do not run Qt when option "-noqt" is specified
  if (argc >= 2) {
    if (std::string(argv[1]) == "-noqt") {
      return 0;
    }
  }
  QApplication app(argc, (char**)argv);
  main_win = new MainWindow;

  QtTclNotify::QtTclNotifier::setup();  // registers my notifier with Tcl

  // tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // create a Tcl interpreter and connect it to the terminal
  Tcl_Main(argc, (char**)argv, Tcl_AppInit);

  //  std::string result =
  //      interpreter.evalCmd("puts \"Hello Foedag, you have Tcl!\"");
  //  std::cout << result << '\n';
  //  if (argc >= 2) {
  //    if (std::string(argv[1]) == "-noqt") {
  //      return 0;
  //    }
  //  }
  //  Command* start = new Command("gui_start", "bye_gui");
  //  commands.push_and_exec(start);
}
