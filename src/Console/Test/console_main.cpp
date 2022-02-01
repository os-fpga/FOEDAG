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
#include <thread>

#include "Main/Foedag.h"
#include "StreamBuffer.h"
#include "Tcl/TclInterpreter.h"
#include "TclConsole.h"
#include "TclConsoleWidget.h"

FOEDAG::Session* GlobalSession;

QWidget* mainWindowBuilder(FOEDAG::CommandLine* cmd,
                           FOEDAG::TclInterpreter* interp) {
  Q_UNUSED(cmd)
  return new FOEDAG::TclConsoleWidget{
      interp->getInterp(),
      std::make_unique<FOEDAG::TclConsole>(interp->getInterp(), std::cout),
      new FOEDAG::StreamBuffer};
}

void registerExampleCommands(FOEDAG::Session* session) {
  auto console_open = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Q_UNUSED(clientData)
    Q_UNUSED(argv)
    Q_UNUSED(argc)
    auto console = new FOEDAG::TclConsoleWidget{
        interp, std::make_unique<FOEDAG::TclConsole>(interp, std::cout),
        new FOEDAG::StreamBuffer};
    console->show();
    return 0;
  };
  session->TclInterp()->registerCmd("console_open", console_open,
                                    GlobalSession->TclInterp(), nullptr);
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
