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
#include "Tcl/TclInterpreter.h"
#include "main_window.h"

static int GuiStartCmd(ClientData clientData, Tcl_Interp* interp, int argc,
                       const char** argv) {
  QApplication app(argc, (char**)argv);
  MainWindow main_win;
  main_win.show();
  return app.exec();
}

int main(int argc, char** argv) {
  TclInterpreter interpreter(argv[0]);

  interpreter.registerCmd("gui_start", GuiStartCmd, 0, nullptr);
  CommandStack commands(&interpreter);

  std::string result =
      interpreter.evalCmd("puts \"Hello Foedag, you have Tcl!\"");
  std::cout << result << '\n';
  if (argc >= 2) {
    if (std::string(argv[1]) == "-noqt") {
      return 0;
    }
  }
  Command* start = new Command("gui_start", "bye_gui");
  commands.push_and_exec(start);
}
