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
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "foedag.h"
#include "qttclnotifier.hpp"
#include "CommandLine.h"

void registerTclCommands(FOEDAG::Session* session) {
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

  auto tcl_exit = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due to
                  // the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);

 auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    GlobalSession->CmdLine()->printHelp();
    return 0;
  };
  session->TclInterp()->registerCmd("help", help, 0, 0);

}
