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

#include "Console/StreamBuffer.h"
#include "DesignRuns/runs_form.h"
#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"
#include "tclutils/TclUtils.h"

QWidget* DesignRunsBuilder(FOEDAG::CommandLine* cmd,
                           FOEDAG::TclInterpreter* interp) {
  Q_UNUSED(interp);
  FOEDAG::StreamBuffer* buffer = new FOEDAG::StreamBuffer;
  FOEDAG::RunsForm* runForm = new FOEDAG::RunsForm(buffer->getStream());
  if (cmd->Argc() > 2) {
    runForm->InitRunsForm(cmd->Argv()[2]);
  }
  return runForm;
}

void registerDesignRunsCommands(QWidget* widget, FOEDAG::Session* session) {
  FOEDAG::utils::Command::registerAllcommands(
      GlobalSession->TclInterp()->getInterp(), GlobalSession->MainWindow());
}

int main(int argc, char** argv) {
  FOEDAG::utils::initCommandRegister();
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  if (!cmd->WithQt()) {
    // Batch mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, nullptr, registerDesignRunsCommands);
    return foedag->initBatch();
  } else {
    // Gui mode
    FOEDAG::Foedag* foedag =
        new FOEDAG::Foedag(cmd, DesignRunsBuilder, registerDesignRunsCommands);
    return foedag->initGui();
  }
}
