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
#include "ProjNavigator/sources_form.h"
#include "Tcl/TclInterpreter.h"

FOEDAG::Session* GlobalSession;

QWidget* proNavigatorBuilder(FOEDAG::CommandLine* cmd,
                             FOEDAG::TclInterpreter* interp) {
  Q_UNUSED(interp);
  if (cmd->Argc() > 2) {
    return new FOEDAG::SourcesForm(cmd->Argv()[2]);
  } else {
    return new FOEDAG::SourcesForm("/testproject");
  }
}

void registerProjNavigatorCommands(QWidget* widget, FOEDAG::Session* session) {
  auto projnavigator = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->show();
    return 0;
  };
  session->TclInterp()->registerCmd("projnavigator_show", projnavigator, widget,
                                    0);

  auto projnavigatorhide = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->hide();
    return 0;
  };
  session->TclInterp()->registerCmd("projnavigator_close", projnavigatorhide,
                                    widget, 0);

  auto openproject = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TestOpenProject(argc, argv);
    srcForm->show();
    return 0;
  };
  session->TclInterp()->registerCmd("open_project", openproject, widget, 0);

  auto createfileset = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TclCreateDesign(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("create_design", createfileset, widget, 0);

  auto addfiles = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TclAddOrCreateFiles(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("add_files", addfiles, widget, 0);

  auto setactive = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TclSetActiveDesign(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("set_active_design", setactive, widget, 0);

  auto settopmodule = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TclSetTopModule(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("set_top_module", settopmodule, widget, 0);

  auto settarget = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Q_UNUSED(interp);
    FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
    srcForm->TclSetAsTarget(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("set_as_target", settarget, widget, 0);
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  if (!cmd->WithQt()) {
    // Batch mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, nullptr, registerProjNavigatorCommands);
    return foedag->initBatch();
  } else {
    // Gui mode
    FOEDAG::Foedag* foedag = new FOEDAG::Foedag(cmd, proNavigatorBuilder,
                                                registerProjNavigatorCommands);
    return foedag->initGui();
  }
}
