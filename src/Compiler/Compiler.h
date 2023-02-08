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
#ifndef COMPILER_H
#define COMPILER_H

#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
//#include "DesignQuery/DesignQuery.h"
#include "IPGenerate/IPGenerator.h"
#include "Main/CommandLine.h"
#include "Simulation/Simulator.h"
#include "Tcl/TclInterpreter.h"

class QProcess;

namespace FOEDAG {

class TaskManager;
class TclInterpreterHandler;
class Session;
class DesignManager;
class TclCommandIntegration;
class Constraints;

struct DeviceData {
  std::string family;
  std::string series;
  std::string package;
};

class Compiler {
  friend Simulator;

 public:
  enum class NetlistType { Blif, EBlif, Edif, Verilog, VHDL };
  enum class Action {
    NoAction,
    IPGen,
    Analyze,
    Synthesis,
    Pack,
    Global,
    Detailed,
    Routing,
    STA,
    Power,
    Bitstream,
    Batch,
    SimulateRTL,
    SimulateGate,
    SimulatePNR,
    SimulateBitstream
  };
  enum class State {
    None,
    IPGenerated,
    Analyzed,
    Synthesized,
    Packed,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    PowerAnalyzed,
    BistreamGenerated
  };
  enum MsgSeverity { Ignore, Info, Warning, Error };
  enum class IPGenerateOpt { None, Clean, List };
  enum class DesignAnalysisOpt { None, Clean };
  enum class SynthesisOpt { None, Area, Delay, Mixed, Clean };
  enum class PackingOpt { None, Clean };
  enum class GlobalPlacementOpt { None, Clean };
  enum class PlacementOpt { None, Clean };
  enum class PinAssignOpt { Random, In_Define_Order, Free };
  enum class RoutingOpt { None, Clean };
  enum class PowerOpt { None, Clean };
  enum class STAOpt { None, Clean, View };
  enum class BitstreamOpt { DefaultBitsOpt, Force, EnableSimulation, Clean };
  enum class STAEngineOpt { Tatum, Opensta };

  // Most common use case, create the compiler in your main
  Compiler() = default;
  Compiler(TclInterpreter* interp, std::ostream* out,
           TclInterpreterHandler* tclInterpreterHandler = nullptr);
  void SetInterpreter(TclInterpreter* interp) { m_interp = interp; }
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetErrStream(std::ostream* err) { m_err = err; };
  std::ostream* GetOutStream() { return m_out; }
  void SetTclInterpreterHandler(TclInterpreterHandler* tclInterpreterHandler);
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() const { return m_session; }
  virtual ~Compiler();

  void BatchScript(const std::string& script) { m_batchScript = script; }
  State CompilerState() const { return m_state; }
  void CompilerState(State st) { m_state = st; }
  bool Compile(Action action);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  void start();
  void finish();
  class ProjectManager* ProjManager() const {
    return m_projManager;
  }
  std::string& getResult() { return m_result; }

  void setTaskManager(TaskManager* newTaskManager);
  TaskManager* GetTaskManager() const;
  Constraints* getConstraints() { return m_constraints; }
  void setGuiTclSync(TclCommandIntegration* tclCommands);
  virtual void Help(std::ostream* out);
  virtual void Version(std::ostream* out);
  virtual void Message(const std::string& message) const;
  virtual void ErrorMessage(const std::string& message,
                            bool append = true) const;
  virtual std::vector<std::string> GetCleanFiles(
      Action action, const std::string& projectName,
      const std::string& topModule) const;
  void CleanFiles(Action action);
  std::string GetMessagePrefix() const;
  void SetUseVerific(bool on) { m_useVerific = on; }

  void SetIPGenerator(IPGenerator* generator) { m_IPGenerator = generator; }
  IPGenerator* GetIPGenerator() { return m_IPGenerator; }
  void SetSimulator(Simulator* simulator) { m_simulator = simulator; }
  Simulator* GetSimulator();

  bool BuildLiteXIPCatalog(std::filesystem::path litexPath);
  bool HasIPInstances();
  bool HasIPDefinitions();

  // VPR, Yosys generic opt
  void ChannelWidth(uint32_t width) { m_channel_width = width; }
  void LutSize(uint32_t size) { m_lut_size = size; }

  IPGenerateOpt IPGenOpt() const { return m_ipGenerateOpt; }
  void IPGenOpt(IPGenerateOpt opt) { m_ipGenerateOpt = opt; }
  DesignAnalysisOpt AnalyzeOpt() const { return m_analysisOpt; }
  void AnalyzeOpt(DesignAnalysisOpt opt) { m_analysisOpt = opt; }
  PackingOpt PackOpt() const { return m_packingOpt; }
  void PackOpt(PackingOpt opt) { m_packingOpt = opt; }
  SynthesisOpt SynthOpt() const { return m_synthOpt; }
  void SynthOpt(SynthesisOpt opt) { m_synthOpt = opt; }
  GlobalPlacementOpt GlobPlacementOpt() const { return m_globalPlacementOpt; }
  void GlobPlacementOpt(GlobalPlacementOpt opt) { m_globalPlacementOpt = opt; }
  PlacementOpt PlaceOpt() const { return m_placementOpt; }
  void PlaceOpt(PlacementOpt opt) { m_placementOpt = opt; }
  PinAssignOpt PinAssignOpts() { return m_pinAssignOpt; }
  void PinAssignOpts(PinAssignOpt opt) { m_pinAssignOpt = opt; }
  RoutingOpt RouteOpt() const { return m_routingOpt; }
  void RouteOpt(RoutingOpt opt) { m_routingOpt = opt; }
  STAOpt TimingAnalysisOpt() const { return m_staOpt; }
  void TimingAnalysisOpt(STAOpt opt) { m_staOpt = opt; }
  PowerOpt PowerAnalysisOpt() const { return m_powerOpt; }
  void PowerAnalysisOpt(PowerOpt opt) { m_powerOpt = opt; }
  STAEngineOpt TimingAnalysisEngineOpt() const { return m_staEngineOpt; }
  void TimingAnalysisEngineOpt(STAEngineOpt opt) { m_staEngineOpt = opt; }

  BitstreamOpt BitsOpt() const { return m_bitstreamOpt; }
  void BitsOpt(BitstreamOpt opt) { m_bitstreamOpt = opt; }
  // Compiler specific opt
  const std::string& SynthMoreOpt() { return m_synthMoreOpt; }
  void SynthMoreOpt(const std::string& opt) { m_synthMoreOpt = opt; }

  const std::string& PlaceMoreOpt() { return m_placeMoreOpt; }
  void PlaceMoreOpt(const std::string& opt) { m_placeMoreOpt = opt; }

  const std::string& IPGenMoreOpt() { return m_ipGenMoreOpt; }
  void IPGenMoreOpt(const std::string& opt) { m_ipGenMoreOpt = opt; }

  void PnROpt(const std::string& opt) { m_pnrOpt = opt; }
  const std::string& PnROpt() { return m_pnrOpt; }

  std::string& GetOutput() { return m_output; }

  bool BitstreamEnabled() { return m_bitstreamEnabled; }
  void BitstreamEnabled(bool enabled) { m_bitstreamEnabled = enabled; }

  bool PinConstraintEnabled() { return m_pin_constraintEnabled; }
  void PinConstraintEnabled(bool enabled) { m_pin_constraintEnabled = enabled; }

  virtual const std::string GetNetlistPath();

  void AddMsgSeverity(std::string msg, MsgSeverity severity) {
    m_severityMap.insert(std::make_pair(msg, severity));
  }
  const std::map<std::string, MsgSeverity>& MsgSeverityMap() {
    return m_severityMap;
  }

  void SetConstraints(Constraints* c);

  void SetNetlistType(NetlistType type) { m_netlistType = type; }
  NetlistType GetNetlistType() { return m_netlistType; }

  void virtual CustomSimulatorSetup(Simulator::SimulationType action);
  void SetWaveformFile(const std::string& wave) { m_waveformFile = wave; }
  const std::string& GetWavefromFile() { return m_waveformFile; }

  QProcess* GetGTKWaveProcess();
  void GTKWaveSendCmd(const std::string& gtkWaveCmd,
                      bool raiseGtkWindow = true);

  void PinmapCSVFile(const std::filesystem::path& path);
  const std::filesystem::path& PinmapCSVFile() const;

  DeviceData deviceData() const;
  void setDeviceData(const DeviceData& newDeviceData);

 protected:
  /* Methods that can be customized for each new compiler flow */
  virtual bool IPGenerate();
  virtual bool Analyze();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();

  /*!
   * \brief CheckTargetDevice
   * \return true if target device is set otherwise return false
   */
  virtual bool VerifyTargetDevice() const;
  bool HasTargetDevice();

  bool CreateDesign(const std::string& name,
                    const std::string& type = std::string{});

  /* Compiler class utilities */
  bool RunBatch();
  bool RunCompileTask(Action action);

  void SetEnvironmentVariable(const std::string variable,
                              const std::string value);
  virtual int ExecuteAndMonitorSystemCommand(
      const std::string& command, const std::string logFile = std::string{},
      bool appendLog = false);
  std::string ReplaceAll(std::string_view str, std::string_view from,
                         std::string_view to);
  virtual std::pair<bool, std::string> IsDeviceSizeCorrect(
      const std::string& size) const;

  enum AddFilesType { Design, Simulation };
  int add_files(Compiler* compiler, Tcl_Interp* interp, int argc,
                const char* argv[], AddFilesType filesType);

  void installGTKWaveHelpers();
  void writeHelp(
      std::ostream* out,
      const std::vector<std::pair<std::string, std::string>>& cmdDescPairs,
      int frontSpacePadCount, int descColumn);
  void writeWaveHelp(std::ostream* out, int frontSpacePadCount, int descColumn);
  void AddHeadersToLogs();
  void AddErrorLink(const class Task* const current);
  bool HasInternalError() const;
  /* Propected members */
  TclInterpreter* m_interp = nullptr;
  Session* m_session = nullptr;
  class ProjectManager* m_projManager = nullptr;
  bool m_stop = false;
  State m_state = State::None;
  std::ostream* m_out = &std::cout;
  std::ostream* m_err = &std::cerr;
  std::string m_batchScript;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler{nullptr};
  TaskManager* m_taskManager{nullptr};
  TclCommandIntegration* m_tclCmdIntegration{nullptr};
  Constraints* m_constraints = nullptr;
  std::string m_output;
  bool m_useVerific = false;

  // Tasks generic options
  IPGenerateOpt m_ipGenerateOpt = IPGenerateOpt::None;
  DesignAnalysisOpt m_analysisOpt = DesignAnalysisOpt::None;
  SynthesisOpt m_synthOpt = SynthesisOpt::None;
  PackingOpt m_packingOpt = PackingOpt::None;
  GlobalPlacementOpt m_globalPlacementOpt = GlobalPlacementOpt::None;
  PlacementOpt m_placementOpt = PlacementOpt::None;
  PinAssignOpt m_pinAssignOpt = PinAssignOpt::In_Define_Order;
  RoutingOpt m_routingOpt = RoutingOpt::None;
  PowerOpt m_powerOpt = PowerOpt::None;
  STAOpt m_staOpt = STAOpt::None;
  STAEngineOpt m_staEngineOpt = STAEngineOpt::Tatum;
  BitstreamOpt m_bitstreamOpt = BitstreamOpt::DefaultBitsOpt;
  std::filesystem::path m_PinMapCSV{};
  DeviceData m_deviceData;

  // Compiler specific options
  std::string m_pnrOpt;
  std::string m_synthMoreOpt;
  std::string m_placeMoreOpt;
  std::string m_ipGenMoreOpt;

  // VPR, Yosys options
  uint32_t m_channel_width = 100;
  uint32_t m_lut_size = 6;
  bool m_bitstreamEnabled = true;
  bool m_pin_constraintEnabled = true;
  class QProcess* m_process = nullptr;

  // Sub engines
  IPGenerator* m_IPGenerator = nullptr;
  Simulator* m_simulator = nullptr;
  // DesignQuery* m_DesignQuery = nullptr;

  // Error message severity
  std::map<std::string, MsgSeverity> m_severityMap;

  std::map<std::string, std::string> m_environmentVariableMap;

  NetlistType m_netlistType = NetlistType::Blif;

  std::string m_waveformFile;

  // GTKWave
  QProcess* m_gtkwave_process = nullptr;
};

}  // namespace FOEDAG

#endif
