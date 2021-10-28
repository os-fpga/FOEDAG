/*
 Copyright 2021 The Foedag Team

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
    Tcl_Exit(0);
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
  // Never ends
}

}  // namespace
}  // namespace FOEDAG
