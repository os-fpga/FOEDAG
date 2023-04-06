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

#include "Compiler/CompilerOpenFPGA.h"
#include "Main/CommandLine.h"
#include "Main/Foedag.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"

QWidget* mainWindowBuilder(FOEDAG::Session* session) {
  auto m = new FOEDAG::MainWindow{session};
  auto info = m->Info();
  info.licenseFile = "LICENSE";
  m->Info(info);
  return m;
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Compiler* compiler = nullptr;
  FOEDAG::CompilerOpenFPGA* opcompiler = nullptr;
  FOEDAG::Settings* settings = new FOEDAG::Settings();
  if (cmd->CompilerName() == "openfpga") {
    opcompiler = new FOEDAG::CompilerOpenFPGA();
    compiler = opcompiler;
    compiler->SetUseVerific(cmd->UseVerific());
  } else {
    compiler = new FOEDAG::Compiler();
  }

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, mainWindowBuilder, registerAllFoedagCommands, compiler, settings);
  std::filesystem::path binpath = foedag->Context()->BinaryPath();
  std::filesystem::path datapath = foedag->Context()->DataPath();
  if (opcompiler) {
    std::filesystem::path analyzePath = binpath / "analyze";
    std::filesystem::path yosysPath = binpath / "yosys";
    std::filesystem::path vprPath = binpath / "vpr";
    std::filesystem::path openFpgaPath = binpath / "openfpga";
    std::filesystem::path pinConvPath = binpath / "pin_c";
    std::filesystem::path bitstreamSettingPath =
        datapath / "Arch" / "bitstream_annotation.xml";
    std::filesystem::path simSettingPath =
        datapath / "Arch" / "fixed_sim_openfpga.xml";
    std::filesystem::path repackConstraintPath =
        datapath / "Arch" / "repack_design_constraint.xml";
    std::filesystem::path openOcdPath = binpath / "openocd";
    opcompiler->AnalyzeExecPath(analyzePath);
    opcompiler->YosysExecPath(yosysPath);
    opcompiler->VprExecPath(vprPath);
    opcompiler->OpenFpgaExecPath(openFpgaPath);
    opcompiler->OpenFpgaBitstreamSettingFile(bitstreamSettingPath);
    opcompiler->OpenFpgaSimSettingFile(simSettingPath);
    opcompiler->OpenFpgaRepackConstraintsFile(repackConstraintPath);
    opcompiler->PinConvExecPath(pinConvPath);
    opcompiler->ProgrammerToolExecPath(openOcdPath);

    std::filesystem::path configFileSearchDir = datapath / "configuration";
    opcompiler->SetConfigFileSearchDirectory(configFileSearchDir);
  }
  return foedag->init(guiType);
}
