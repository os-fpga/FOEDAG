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

// clang-format off

#include "Compiler/CompilerOpenFPGA_ql.h"
#include "Compiler/CompilerOpenFPGA.h"
#include "Main/CommandLine.h"
#include "Main/Foedag.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"

QWidget* mainWindowBuilder(FOEDAG::Session* session) {
  return new FOEDAG::MainWindow{session};
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Compiler* compiler = nullptr;
  FOEDAG::CompilerOpenFPGA* opcompiler = nullptr;
  FOEDAG::CompilerOpenFPGA_ql* opcompiler_ql = nullptr;
  FOEDAG::Settings* settings = new FOEDAG::Settings();
  if (cmd->CompilerName() == "openfpga") {
    opcompiler = new FOEDAG::CompilerOpenFPGA();
    compiler = opcompiler;
    compiler->SetUseVerific(cmd->UseVerific());
  } else if (cmd->CompilerName() == "test") {
      compiler = new FOEDAG::Compiler();
  } else {
    // default flow should be QL flow.
    opcompiler_ql = new FOEDAG::CompilerOpenFPGA_ql();
    compiler = opcompiler_ql;
    compiler->SetUseVerific(cmd->UseVerific());
  }

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, mainWindowBuilder, registerAllFoedagCommands, compiler, settings);
  if (opcompiler) {
    std::filesystem::path binpath = foedag->Context()->BinaryPath();
    std::filesystem::path datapath = foedag->Context()->DataPath();
    std::filesystem::path yosysPath = binpath / "yosys";
    std::filesystem::path vprPath = binpath / "vpr";
    std::filesystem::path openFpgaPath = binpath / "openfpga";
    std::filesystem::path pinConvPath = binpath / "pin_c";
    std::filesystem::path litexPath = binpath / "litex";
    std::filesystem::path archPath =
        datapath / "Arch" / "k6_frac_N10_tileable_40nm.xml";
    std::filesystem::path openFpgaArchPath =
        datapath / "Arch" / "k6_N10_40nm_openfpga.xml";
    std::filesystem::path bitstreamSettingPath =
        datapath / "Arch" / "bitstream_annotation.xml";
    std::filesystem::path simSettingPath =
        datapath / "Arch" / "fixed_sim_openfpga.xml";
    std::filesystem::path repackConstraintPath =
        datapath / "Arch" / "repack_design_constraint.xml";
    opcompiler->YosysExecPath(yosysPath);
    opcompiler->VprExecPath(vprPath);
    opcompiler->OpenFpgaExecPath(openFpgaPath);
    opcompiler->ArchitectureFile(archPath);
    opcompiler->OpenFpgaArchitectureFile(openFpgaArchPath);
    opcompiler->OpenFpgaBitstreamSettingFile(bitstreamSettingPath);
    opcompiler->OpenFpgaSimSettingFile(simSettingPath);
    opcompiler->OpenFpgaRepackConstraintsFile(repackConstraintPath);
    opcompiler->PinConvExecPath(pinConvPath);
    opcompiler->BuildLiteXIPCatalog(litexPath);
  }
  return foedag->init(guiType);
}

// clang-format on
