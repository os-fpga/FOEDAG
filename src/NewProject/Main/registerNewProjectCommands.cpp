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

#include "registerNewProjectCommands.h"

#include <QApplication>

#include "../CustomLayout.h"
#include "../new_project_dialog.h"
#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"

void registerNewProjectCommands(QWidget* widget, FOEDAG::Session* session) {
  auto newproject = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->show();
    return 0;
  };
  session->TclInterp()->registerCmd("newproject_gui_open", newproject, widget,
                                    0);

  auto newprojecthide = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->hide();
    return 0;
  };
  session->TclInterp()->registerCmd("newproject_gui_close", newprojecthide,
                                    widget, 0);

  auto btnnext = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->Next_TclCommand_Test();
    return 0;
  };
  session->TclInterp()->registerCmd("next", btnnext, widget, 0);

  auto createproject = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->CreateProject_Tcl_Test(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("create_project", createproject, widget, 0);

  auto createdevice = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->CreateDevice_TclCommand_Test();
    auto customLayout = dialog->findChild<FOEDAG::CustomLayout*>();
    if (customLayout) {
      return TCL_OK;
    }
    Tcl_AppendResult(interp, "No CustomLayout widget", nullptr);
    return TCL_ERROR;
  };
  session->TclInterp()->registerCmd("createdevice", createdevice, widget, 0);

  auto editdevice = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::newProjectDialog* dialog = (FOEDAG::newProjectDialog*)(clientData);
    dialog->EditDevice_TclCommand_Test();
    auto customLayout = dialog->findChild<FOEDAG::CustomLayout*>();
    if (customLayout) {
      return TCL_OK;
    }
    Tcl_AppendResult(interp, "No CustomLayout widget", nullptr);
    return TCL_ERROR;
  };
  session->TclInterp()->registerCmd("editdevice", editdevice, widget, 0);
}
