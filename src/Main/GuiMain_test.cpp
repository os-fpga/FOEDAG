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

#include <QApplication>
#include <QLabel>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "Command/CommandStack.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "foedag.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "qttclnotifier.hpp"

FOEDAG::Session* GlobalSession;

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

  auto gui_refresh = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    return 0;
  };
  session->TclInterp()->registerCmd("gui_refresh", gui_refresh, 0, 0);

  auto tcl_exit = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due to
                  // the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);
}

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {
TEST(GuiMain, GuiOpenClose) {
  char** argv = new char*[1];
  int argc = 1;
  argv[0] = strdup("FakePAth");

  QApplication app(argc, (char**)argv);
  FOEDAG::MainWindow* main_win = new FOEDAG::MainWindow;
  FOEDAG::TclInterpreter* interpreter = new FOEDAG::TclInterpreter(argv[0]);
  FOEDAG::CommandStack* commands = new FOEDAG::CommandStack(interpreter);
  GlobalSession = new FOEDAG::Session(main_win, interpreter, commands);
  registerTclCommands(GlobalSession);

  QtTclNotify::QtTclNotifier::setup();  // registers my notifier with Tcl

  // Tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // Tcl_AppInit runs the test
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    Tcl_Eval(interp, R"(
      gui_start
      after 1000 gui_stop
      after 2000 tcl_exit

      set CONT 1 
      while {$CONT} {
        set a 0
        after 100 set a 1
        vwait a
      }
      
    )");
    return 0;
  };

  // Start Loop
  Tcl_MainEx(argc, (char**)argv, tcl_init, interpreter->getInterp());
  // Never ends, executable exits with the Tcl_exit call.
  // Google test cannot tear down the test, so no reporting happens
}

}  // namespace
}  // namespace FOEDAG
