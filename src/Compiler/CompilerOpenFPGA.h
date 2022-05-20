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

  void YosysExecPath(const std::filesystem::path& path) {
    m_yosysExecutablePath = path;
  }
  void OpenFpgaExecPath(const std::filesystem::path& path) {
    m_openFpgaExecutablePath = path;
  }
  void VprExecPath(const std::filesystem::path& path) {
    m_vprExecutablePath = path;
  }
  void ArchitectureFile(const std::filesystem::path& path) {
    m_architectureFile = path;
  }
  void YosysScript(const std::string& script) { m_yosysScript = script; }
  void OpenFPGAScript(const std::string& script) { m_openFPGAScript = script; }
  void OpenFpgaArchitectureFile(const std::filesystem::path& path) {
    m_OpenFpgaArchitectureFile = path;
  }
  void OpenFpgaSimSettingFile(const std::filesystem::path& path) {
    m_OpenFpgaSimSettingFile = path;
  }
  void OpenFpgaBitstreamSettingFile(const std::filesystem::path& path) {
    m_OpenFpgaBitstreamSettingFile = path;
  }
  void OpenFpgaRepackConstraintsFile(const std::filesystem::path& path) {
    m_OpenFpgaRepackConstraintsFile = path;
  }
  void OpenFpgaPinmapXMLFile(const std::filesystem::path& path) {
    m_OpenFpgaPinMapXml = path;
  }
  void OpenFpgaPinmapCSVFile(const std::filesystem::path& path) {
    m_OpenFpgaPinMapCSV = path;
  }
  void DeviceSize(const std::string& XxY) { m_deviceSize = XxY; }
  void Help(std::ostream* out);
  void Version(std::ostream* out);
  void KeepAllSignals(bool on) { m_keepAllSignals = on; }

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
  virtual bool LoadDeviceData(const std::string& deviceName);

  virtual bool DesignChanged(const std::string& synth_script,
                             const std::filesystem::path& synth_scrypt_path);
  virtual std::string InitSynthesisScript();
  virtual std::string FinishSynthesisScript(const std::string& script);
  virtual std::string InitOpenFPGAScript();
  virtual std::string FinishOpenFPGAScript(const std::string& script);
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  std::filesystem::path m_yosysExecutablePath = "yosys";
  std::filesystem::path m_openFpgaExecutablePath = "openfpga";
  std::filesystem::path m_vprExecutablePath = "vpr";
  std::filesystem::path m_architectureFile =
      "tests/Arch/k6_frac_N10_tileable_40nm.xml";
  std::filesystem::path m_OpenFpgaArchitectureFile =
      "tests/Arch/k6_N10_40nm_openfpga.xml";
  std::filesystem::path m_OpenFpgaSimSettingFile =
      "tests/Arch/fixed_sim_openfpga.xml";
  std::filesystem::path m_OpenFpgaBitstreamSettingFile =
      "tests/Arch/bitstream_annotation.xml";
  std::filesystem::path m_OpenFpgaRepackConstraintsFile =
      "tests/Arch/repack_design_constraint.xml";
  std::filesystem::path m_OpenFpgaPinMapXml = "";
  std::filesystem::path m_OpenFpgaPinMapCSV = "";
  std::string m_deviceSize;
  std::string m_yosysScript;
  std::string m_openFPGAScript;
  virtual std::string BaseVprCommand();
  bool m_keepAllSignals = false;
};

}  // namespace FOEDAG

#endif
