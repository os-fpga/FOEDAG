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
#include "NewFile/new_file.h"
#include "Tcl/TclInterpreter.h"
#include "tcltest/TestingUtils.h"

FOEDAG::Session* GlobalSession;

QWidget* newFileBuilder(FOEDAG::CommandLine* cmd,
                        FOEDAG::TclInterpreter* interp) {
  Q_UNUSED(cmd);
  Q_UNUSED(interp);
  return new FOEDAG::NewFile();
}

void registerNewFileCommands(QWidget* widget, FOEDAG::Session* session) {
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
        new FOEDAG::Foedag(cmd, nullptr, registerNewFileCommands);
    return foedag->initBatch();
  } else {
    // Gui mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, newFileBuilder, registerNewFileCommands);
    return foedag->initGui();
  }
}
