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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Compiler/Compiler.h"

#ifndef COMPILER_OPENFPGA_H
#define COMPILER_OPENFPGA_H

namespace FOEDAG {
enum class SynthesisType { Yosys, QL, RS };

class CompilerOpenFPGA : public Compiler {
 public:
  CompilerOpenFPGA() = default;
  ~CompilerOpenFPGA() = default;
  void AnalyzeExecPath(const std::filesystem::path& path) {
    m_analyzeExecutablePath = path;
  }
  void YosysExecPath(const std::filesystem::path& path) {
    m_yosysExecutablePath = path;
  }
  void OpenFpgaExecPath(const std::filesystem::path& path) {
    m_openFpgaExecutablePath = path;
  }
  void VprExecPath(const std::filesystem::path& path) {
    m_vprExecutablePath = path;
  }
  void StaExecPath(const std::filesystem::path& path) {
    m_staExecutablePath = path;
  }
  void PinConvExecPath(const std::filesystem::path& path) {
    m_pinConvExecutablePath = path;
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
  void OpenFpgaFabricKeyFile(const std::filesystem::path& path) {
    m_OpenFpgaFabricKeyFile = path;
  }
  void OpenFpgaPinmapXMLFile(const std::filesystem::path& path) {
    m_OpenFpgaPinMapXml = path;
  }
  void PbPinFixup(const std::string& name) { m_pb_pin_fixup = name; }
  void DeviceSize(const std::string& XxY) { m_deviceSize = XxY; }
  void Help(std::ostream* out);
  void Version(std::ostream* out);
  void KeepAllSignals(bool on) { m_keepAllSignals = on; }
  const std::string& YosysPluginLibName() { return m_yosysPluginLib; }
  const std::string& YosysPluginName() { return m_yosysPlugin; }
  const std::string& YosysMapTechnology() { return m_mapToTechnology; }

  void YosysPluginLibName(const std::string& libname) {
    m_yosysPluginLib = libname;
  }
  void YosysPluginName(const std::string& name) { m_yosysPlugin = name; }
  void YosysMapTechnology(const std::string& tech) { m_mapToTechnology = tech; }

  const std::string& PerDeviceSynthOptions() { return m_perDeviceSynthOptions; }
  void PerDeviceSynthOptions(const std::string& options) {
    m_perDeviceSynthOptions = options;
  }

  void SynthType(SynthesisType type) { m_synthType = type; }

  const std::string& PerDevicePnROptions() { return m_perDevicePnROptions; }
  void PerDevicePnROptions(const std::string& options) {
    m_perDevicePnROptions = options;
  }

 protected:
  virtual bool IPGenerate();
  virtual bool Analyze();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool ConvertSdcPinConstrainToPcf(std::vector<std::string>&);
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();
  virtual bool LoadDeviceData(const std::string& deviceName);
  virtual bool LicenseDevice(const std::string& deviceName);
  virtual bool DesignChanged(const std::string& synth_script,
                             const std::filesystem::path& synth_scrypt_path,
                             const std::filesystem::path& outputFile);
  virtual std::vector<std::string> GetCleanFiles(
      Action action, const std::string& projectName,
      const std::string& topModule) const;
  virtual std::string InitSynthesisScript();
  virtual std::string FinishSynthesisScript(const std::string& script);
  virtual std::string InitAnalyzeScript();
  virtual std::string FinishAnalyzeScript(const std::string& script);
  virtual std::string InitOpenFPGAScript();
  virtual std::string FinishOpenFPGAScript(const std::string& script);
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  virtual std::pair<bool, std::string> IsDeviceSizeCorrect(
      const std::string& size) const;
  bool VerifyTargetDevice() const;
  static std::filesystem::path copyLog(FOEDAG::ProjectManager* projManager,
                                       const std::string& srcFileName,
                                       const std::string& destFileName);
  bool DesignChangedForAnalysis(std::string& synth_script,
                                std::filesystem::path& synth_scrypt_path,
                                std::filesystem::path& outputFile);
  std::filesystem::path m_yosysExecutablePath = "yosys";
  std::filesystem::path m_analyzeExecutablePath = "analyze";
  SynthesisType m_synthType = SynthesisType::Yosys;
  std::string m_yosysPluginLib;
  std::string m_yosysPlugin;
  std::string m_mapToTechnology;
  std::string m_perDeviceSynthOptions;
  std::string m_perDevicePnROptions;
  std::string m_synthesisType;  // QL, Yosys, ...
  std::filesystem::path m_openFpgaExecutablePath = "openfpga";
  std::filesystem::path m_vprExecutablePath = "vpr";
  std::filesystem::path m_staExecutablePath = "sta";
  std::filesystem::path m_pinConvExecutablePath = "pin_c";
  /*!
   * \brief m_architectureFile
   * We required from user explicitly specify architecture file.
   */
  std::filesystem::path m_architectureFile = "";

  /*!
   * \brief m_OpenFpgaArchitectureFile
   * We required from user explicitly specify openfpga architecture file.
   */
  std::filesystem::path m_OpenFpgaArchitectureFile = "";
  std::filesystem::path m_OpenFpgaSimSettingFile = "";
  std::filesystem::path m_OpenFpgaBitstreamSettingFile = "";
  std::filesystem::path m_OpenFpgaRepackConstraintsFile = "";
  std::filesystem::path m_OpenFpgaFabricKeyFile = "";
  std::filesystem::path m_OpenFpgaPinMapXml = "";
  std::string m_deviceSize;
  std::string m_yosysScript;
  std::string m_openFPGAScript;
  std::string m_pb_pin_fixup;

  virtual std::string BaseVprCommand();
  virtual std::string BaseStaCommand();
  virtual std::string BaseStaScript(std::string libFileName,
                                    std::string netlistFileName,
                                    std::string sdfFileName,
                                    std::string sdcFileName);
  bool m_keepAllSignals = false;
};

}  // namespace FOEDAG

#endif
