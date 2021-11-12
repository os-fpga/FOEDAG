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

#include "../new_project_dialog.h"
#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"

FOEDAG::Session* GlobalSession;

QWidget* newProjectBuilder(FOEDAG::CommandLine* cmd) {
  return new FOEDAG::newProjectDialog();
}

void registerNewProjectCommands(FOEDAG::Session* session) {
  auto newproject = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->show();
    return 0;
  };
  session->TclInterp()->registerCmd("newproject_gui_open", newproject,
                                    GlobalSession->MainWindow(), 0);

  auto newprojecthide = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->hide();
    return 0;
  };
  session->TclInterp()->registerCmd("newproject_gui_close", newprojecthide,
                                    GlobalSession->MainWindow(), 0);

  auto btnnext = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->tcl_command_test();
    return 0;
  };
  session->TclInterp()->registerCmd("next", btnnext,
                                    GlobalSession->MainWindow(), 0);

  session->TclInterp()->evalCmd(
      "puts \"Hello put newproject_gui_open to show new project GUI.\"");
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  if (!cmd->WithQt()) {
    // Batch mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, nullptr, registerNewProjectCommands);
    return foedag->initBatch();
  } else {
    // Gui mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, newProjectBuilder, registerNewProjectCommands);
    return foedag->initGui();
  }
}
