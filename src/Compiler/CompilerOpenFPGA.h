/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2021-2022 The Open-Source FPGA Foundation

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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Compiler/Compiler.h"

#ifndef COMPILER_OPENFPGA_H
#define COMPILER_OPENFPGA_H

namespace FOEDAG {
class CompilerOpenFPGA : public Compiler {
 public:
  CompilerOpenFPGA() = default;
  ~CompilerOpenFPGA() = default;

  void setYosysExecPath(const std::filesystem::path& path) {
    m_yosysExecutablePath = path;
  }
  void setOpenFpgaExecPath(const std::filesystem::path& path) {
    m_openFpgaExecutablePath = path;
  }
  void setVprExecPath(const std::filesystem::path& path) {
    m_vprExecutablePath = path;
  }
  void setArchitectureFile(const std::filesystem::path& path) {
    m_architectureFile = path;
    Message("Architecture file: " + path.string());
  }
  void setYosysScript(const std::string& script) { m_yosysScript = script; }
  
  void help(std::ostream* out);
 protected:
  virtual bool IPGenerate();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  std::filesystem::path m_yosysExecutablePath = "yosys";
  std::filesystem::path m_openFpgaExecutablePath = "openfpga.sh";
  std::filesystem::path m_vprExecutablePath = "vpr";
  std::filesystem::path m_architectureFile =
      "tests/Arch/k6_frac_N10_tileable_40nm.xml";
  std::string m_yosysScript;
  std::string getBaseVprCommand();
};

}  // namespace FOEDAG

#endif
