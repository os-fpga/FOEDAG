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
#ifndef COMPILER_OPENFPGA_H
#define COMPILER_OPENFPGA_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Compiler/Compiler.h"

namespace FOEDAG {
enum class SynthesisType { Yosys, QL, RS };

class CompilerOpenFPGA : public Compiler {
 public:
  CompilerOpenFPGA() { m_name = "openfpga"; };
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
  void ReConstructVExecPath(const std::filesystem::path& path) {
    m_ReConstructVExecPath = path;
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
  void RoutingGraphFile(const std::filesystem::path& path) {
    m_routingGraphFile = path;
  }
  void DeviceTagVersion(const std::string& version) {
    m_deviceTagVersion = version;
  }
  void BaseDeviceName(const std::string& BaseDevice) {
    m_DeviceNameforLicense = BaseDevice;
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
  void OpenFpgaPinConstraintFile(const std::filesystem::path& path) {
    m_OpenFpgaPinConstraintXml = path;
  }
  void OpenFpgaRICModelDir(const std::filesystem::path& path) {
    m_OpenFpgaRICModelDir = path;
  }
  void PbPinFixup(const std::string& name) { m_pb_pin_fixup = name; }
  void DeviceSize(const std::string& XxY) { m_deviceSize = XxY; }

  void MaxDeviceDSPCount(uint32_t max_dsp) { m_maxDeviceDSPCount = max_dsp; }
  void MaxDeviceBRAMCount(uint32_t max_bram) {
    m_maxDeviceBRAMCount = max_bram;
  }
  void MaxDeviceCarryLength(uint32_t carry_length) {
    m_maxDeviceCarryLength = carry_length;
  }
  void MaxDeviceLUTCount(uint32_t max_lut) { m_maxDeviceLUTCount = max_lut; }
  void MaxDeviceFFCount(uint32_t max_ff) { m_maxDeviceFFCount = max_ff; }
  void MaxDeviceIOCount(uint32_t max_io) { m_maxDeviceIOCount = max_io; }
  uint32_t MaxDeviceDSPCount() { return m_maxDeviceDSPCount; }
  uint32_t MaxDeviceBRAMCount() { return m_maxDeviceBRAMCount; }
  uint32_t MaxDeviceCarryLength() { return m_maxDeviceCarryLength; }
  uint32_t MaxDeviceLUTCount() { return m_maxDeviceLUTCount; }
  uint32_t MaxDeviceFFCount() { return m_maxDeviceFFCount; }
  uint32_t MaxDeviceIOCount() { return m_maxDeviceIOCount; }
  void MaxUserDSPCount(uint32_t max_dsp) { m_maxUserDSPCount = max_dsp; }
  void MaxUserBRAMCount(uint32_t max_bram) { m_maxUserBRAMCount = max_bram; }
  void MaxUserCarryLength(uint32_t max_carry_length) {
    m_maxUserCarryLength = max_carry_length;
  }
  int32_t MaxUserDSPCount() { return m_maxUserDSPCount; }
  int32_t MaxUserBRAMCount() { return m_maxUserBRAMCount; }
  int32_t MaxUserCarryLength() { return m_maxUserCarryLength; }
  void FlatRouting(bool value) { m_flatRouting = value; }
  std::vector<std::string> helpTags() const;
  void Version(std::ostream* out);
  void KeepAllSignals(bool on) { m_keepAllSignals = on; }
  const std::string& DeviceTagVersion() { return m_deviceTagVersion; }
  const std::string& BaseDeviceName() { return m_DeviceNameforLicense; }

  const std::string& PerDeviceSynthOptions() { return m_perDeviceSynthOptions; }
  void PerDeviceSynthOptions(const std::string& options) {
    m_perDeviceSynthOptions = options;
  }

  void SynthType(SynthesisType type) { m_synthType = type; }

  const std::string& PerDevicePnROptions() { return m_perDevicePnROptions; }
  void PerDevicePnROptions(const std::string& options) {
    m_perDevicePnROptions = options;
  }

  // In Analyze mode, returns ports (input_only: w/o outputs and ios)
  // In Synthesize mode, returns inferred clocks
  virtual std::pair<bool, std::string> isRtlClock(const std::string& str,
                                                  bool regex, bool input_only);

  std::string BaseVprCommand();

 protected:
  virtual bool IPGenerate();
  virtual bool Analyze();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool ConvertSdcPinConstrainToPcf(std::vector<std::string>&);
  virtual bool WriteTimingConstraints();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();
  virtual bool LoadDeviceData(const std::string& deviceName);
  virtual bool LoadDeviceData(const std::string& deviceName,
                              const std::filesystem::path& deviceListFile,
                              const std::filesystem::path& devicesBase,
                              bool& deviceFound);
  virtual bool LicenseDevice(const std::string& deviceName);
  virtual bool DesignChanged(const std::string& synth_script,
                             const std::filesystem::path& synth_scrypt_path,
                             const std::filesystem::path& outputFile);
  virtual void reloadSettings();
  virtual std::string InitSynthesisScript();
  virtual std::string FinishSynthesisScript(const std::string& script);
  virtual std::string InitAnalyzeScript();
  virtual std::string FinishAnalyzeScript(const std::string& script);
  virtual std::string InitOpenFPGAScript();
  virtual std::string FinishOpenFPGAScript(const std::string& script);
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  virtual std::pair<bool, std::string> IsDeviceSizeCorrect(
      const std::string& size) const;
  virtual void SetDeviceResources();
  bool VerifyTargetDevice() const;
  std::string YosysDesignParsingCommmands();
  std::string SurelogDesignParsingCommmands();
  std::string GhdlDesignParsingCommmands();
  static std::filesystem::path copyLog(FOEDAG::ProjectManager* projManager,
                                       const std::string& srcFileName,
                                       const std::string& destFileName);
  bool DesignChangedForAnalysis(std::string& synth_script,
                                std::filesystem::path& synth_scrypt_path,
                                std::filesystem::path& outputFile);
  void processCustomLayout();
  void RenamePostSynthesisFiles(Action action);
  std::filesystem::path m_yosysExecutablePath = "yosys";
  std::filesystem::path m_analyzeExecutablePath = "analyze";
  SynthesisType m_synthType = SynthesisType::Yosys;
  std::string m_perDeviceSynthOptions;
  std::string m_perDevicePnROptions;
  std::string m_synthesisType;  // QL, Yosys, ...
  std::filesystem::path m_openFpgaExecutablePath = "openfpga";
  std::filesystem::path m_vprExecutablePath = "vpr";
  std::filesystem::path m_ReConstructVExecPath = "finalize";
  std::filesystem::path m_staExecutablePath = "sta";
  std::filesystem::path m_pinConvExecutablePath = "planning";
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
  std::filesystem::path m_routingGraphFile = "";
  std::filesystem::path m_OpenFpgaSimSettingFile = "";
  std::filesystem::path m_OpenFpgaBitstreamSettingFile = "";
  std::filesystem::path m_runtime_OpenFpgaBitstreamSettingFile = "";
  std::filesystem::path m_OpenFpgaRepackConstraintsFile = "";
  std::filesystem::path m_OpenFpgaFabricKeyFile = "";
  std::filesystem::path m_OpenFpgaPinMapXml = "";
  std::filesystem::path m_OpenFpgaPinConstraintXml = "";
  std::filesystem::path m_OpenFpgaRICModelDir = "";
  std::string m_deviceTagVersion;
  std::string m_deviceSize;
  std::string m_yosysScript;
  std::string m_openFPGAScript;
  std::string m_pb_pin_fixup;
  uint32_t m_maxDeviceDSPCount = 0;
  uint32_t m_maxDeviceBRAMCount = 0;
  uint32_t m_maxDeviceLUTCount = 0;
  uint32_t m_maxDeviceFFCount = 0;
  uint32_t m_maxDeviceIOCount = 0;
  uint32_t m_maxDeviceCarryLength = 0;
  int32_t m_maxUserDSPCount = -1;
  int32_t m_maxUserBRAMCount = -1;
  int32_t m_maxUserCarryLength = -1;
  bool m_flatRouting = false;
  struct BaseVprDefaults {
    bool gen_post_synthesis_netlist{true};
  };
  virtual std::string BaseVprCommand(BaseVprDefaults defaults);
  virtual std::string BaseStaCommand();
  virtual std::string BaseStaScript(std::string libFileName,
                                    std::string netlistFileName,
                                    std::string sdfFileName,
                                    std::string sdcFileName);
  bool m_keepAllSignals = false;
  std::string m_DeviceNameforLicense;
};

}  // namespace FOEDAG

#endif
