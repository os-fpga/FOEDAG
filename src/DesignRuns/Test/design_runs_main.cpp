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

#include "DesignRuns/runs_form.h"
#include "Main/Foedag.h"
#include "Main/ProjectFile/ProjectFileLoader.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"
#include "tclutils/TclUtils.h"

QWidget* DesignRunsBuilder(FOEDAG::Session* session) {
  FOEDAG::RunsForm* runForm = new FOEDAG::RunsForm();
  if (session->CmdLine()->Argc() > 2) {
    FOEDAG::ProjectFileLoader loader(FOEDAG::Project::Instance());
    loader.registerComponent(
        new FOEDAG::ProjectManagerComponent(runForm->projectManager()),
        FOEDAG::ComponentId::TaskManager);
    loader.registerComponent(
        new FOEDAG::TaskManagerComponent(
            new FOEDAG::TaskManager(GlobalSession->GetCompiler())),
        FOEDAG::ComponentId::TaskManager);
    loader.registerComponent(
        new FOEDAG::CompilerComponent(GlobalSession->GetCompiler()),
        FOEDAG::ComponentId::Compiler);
    loader.Load(session->CmdLine()->Argv()[2]);
    runForm->InitRunsForm();
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

  FOEDAG::Compiler* compiler = new FOEDAG::Compiler();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, DesignRunsBuilder, registerDesignRunsCommands, compiler);

  return foedag->init(guiType);
}
