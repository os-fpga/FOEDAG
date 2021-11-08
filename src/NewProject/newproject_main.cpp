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

#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"
#include "new_project_dialog.h"

FOEDAG::Session* GlobalSession;
FOEDAG::newProjectDialog* m_dialog;

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  FOEDAG::TclInterpreter* interpreter = new FOEDAG::TclInterpreter();

  m_dialog = new FOEDAG::newProjectDialog();

  auto newproject = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    m_dialog->show();
    return 0;
  };
  interpreter->registerCmd("newproject", newproject, 0, 0);

  QtTclNotify::QtTclNotifier::setup();  // Registers notifier with Tcl

  // Tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    FOEDAG::TclInterpreter interpreter;
    std::string result = interpreter.evalCmd(
        "puts \"Hello put newproject to create new project.\"");
    return 0;
  };

  // Start Loop
  Tcl_MainEx(argc, argv, tcl_init, interpreter->getInterp());
  return 0;
}
