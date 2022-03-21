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

#include "ConsoleTestUtils.h"
#include "Main/Foedag.h"
#include "TestingUtils.h"

QWidget* mainWindowBuilder(FOEDAG::CommandLine* cmd,
                           FOEDAG::TclInterpreter* interp) {
  Q_UNUSED(cmd)
  FOEDAG::TclConsoleWidget* console{nullptr};
  auto buffer = new FOEDAG::StreamBuffer;
  auto w = FOEDAG::createConsole(interp->getInterp(),
                                 std::make_unique<FOEDAG::TclConsole>(
                                     interp->getInterp(), buffer->getStream()),
                                 buffer, nullptr, &console);
  return w;
}

void registerExampleCommands(QWidget* widget, FOEDAG::Session* session) {
  auto debug = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
    QWidget* w = static_cast<QWidget*>(clientData);
    FOEDAG::TclConsoleWidget* console = w->findChild<FOEDAG::TclConsoleWidget*>(
        FOEDAG::TclConsoleWidget::consoleObjectName());
    console->getBuffer()->getStream() << argv[1] << std::endl;
    return TCL_OK;
  };
  Tcl_CreateCommand(GlobalSession->TclInterp()->getInterp(), "debug", debug,
                    GlobalSession->MainWindow(), nullptr);
  FOEDAG::testing::test::runAllTests(GlobalSession->TclInterp()->getInterp(),
                                     GlobalSession->MainWindow());
}

int main(int argc, char** argv) {
  FOEDAG::testing::initTesting();
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
