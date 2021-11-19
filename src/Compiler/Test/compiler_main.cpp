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

#include "Main/CommandLine.h"
#include "Main/Foedag.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Compiler/Design.h"
#include "Compiler/Compiler.h"
#include "Compiler/WorkerThread.h"

FOEDAG::Session* GlobalSession;

QWidget* mainWindowBuilder(FOEDAG::CommandLine* cmd) {
  return new FOEDAG::MainWindow;
}

void registerExampleCommands(FOEDAG::Session* session) {
  auto design_init = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
    std::string designName = "test_design";
    FOEDAG::Design* design = new FOEDAG::Design(designName);
    FOEDAG::Compiler* compiler = new FOEDAG::Compiler(GlobalSession->TclInterp(), design, std::cout);
    compiler->registerCommands();
    return 0;
  };
  session->TclInterp()->registerCmd("design_init", design_init, 0, 0);
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  if (!cmd->WithQt()) {
    // Batch mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, nullptr, registerExampleCommands);
    return foedag->initBatch();
  } else {
    // Gui mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, mainWindowBuilder, registerExampleCommands);
    return foedag->initGui();
  }
}
