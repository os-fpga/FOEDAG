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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "IPGenerate/IPGenerator.h"
#include "Main/CommandLine.h"
#include "Tcl/TclInterpreter.h"

#ifndef COMPILER_H
#define COMPILER_H

namespace FOEDAG {

class TaskManager;
class TclInterpreterHandler;
class Session;
class DesignManager;
class TclCommandIntegration;
class Constraints;

class Compiler {
 public:
  enum class Action {
    NoAction,
    IPGen,
    Synthesis,
    Pack,
    Global,
    Detailed,
    Routing,
    STA,
    Power,
    Bitstream,
    Batch
  };
  enum class State {
    None,
    IPGenerated,
    Synthesized,
    Packed,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    PowerAnalyzed,
    BistreamGenerated
  };
  enum class IPGenerateOpt { None, Clean };
  enum class SynthesisOpt { None, Area, Delay, Mixed, Clean };
  enum class PackingOpt { None, Clean };
  enum class GlobalPlacementOpt { None, Clean };
  enum class PlacementOpt { None, Clean };
  enum class RoutingOpt { None, Clean };
  enum class PowerOpt { None, Clean };
  enum class STAOpt { None, Clean };
  enum class BitstreamOpt { NoBitsOpt, Force, Clean };

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
  Session* GetSession() { return m_session; }
  virtual ~Compiler();

  void BatchScript(const std::string& script) { m_batchScript = script; }
  State CompilerState() { return m_state; }
  bool Compile(Action action);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  bool Clear();
  void start();
  void finish();
  class ProjectManager* ProjManager() {
    return m_projManager;
  }
  std::string& getResult() { return m_result; }

  void setTaskManager(TaskManager* newTaskManager);
  Constraints* getConstraints() { return m_constraints; }
  void setGuiTclSync(TclCommandIntegration* tclCommands);
  virtual void Help(std::ostream* out);
  virtual void Version(std::ostream* out);
  virtual void Message(const std::string& message);
  virtual void ErrorMessage(const std::string& message);
  void SetUseVerific(bool on) { m_useVerific = on; }
  void SetHardError(bool on) { m_hardError = on; }

  void SetIPGenerator(IPGenerator* generator) { m_IPGenerator = generator; }
  IPGenerator* GetIPGenerator() { return m_IPGenerator; }
  bool BuildLiteXIPCatalog(std::filesystem::path litexPath);

  // VPR, Yosys generic opt
  void ChannelWidth(uint32_t width) { m_channel_width = width; }
  void LutSize(uint32_t size) { m_lut_size = size; }

  IPGenerateOpt IPGenOpt() const { return m_ipGenerateOpt; }
  void IPGenOpt(IPGenerateOpt opt) { m_ipGenerateOpt = opt; }
  PackingOpt PackOpt() const { return m_packingOpt; }
  void PackOpt(PackingOpt opt) { m_packingOpt = opt; }
  SynthesisOpt SynthOpt() const { return m_synthOpt; }
  void SynthOpt(SynthesisOpt opt) { m_synthOpt = opt; }
  GlobalPlacementOpt GlobPlacementOpt() const { return m_globalPlacementOpt; }
  void GlobPlacementOpt(GlobalPlacementOpt opt) { m_globalPlacementOpt = opt; }
  PlacementOpt PlaceOpt() const { return m_placementOpt; }
  void PlaceOpt(PlacementOpt opt) { m_placementOpt = opt; }
  RoutingOpt RouteOpt() const { return m_routingOpt; }
  void RouteOpt(RoutingOpt opt) { m_routingOpt = opt; }
  STAOpt TimingAnalysisOpt() const { return m_staOpt; }
  void TimingAnalysisOpt(STAOpt opt) { m_staOpt = opt; }
  PowerOpt PowerAnalysisOpt() const { return m_powerOpt; }
  void PowerAnalysisOpt(PowerOpt opt) { m_powerOpt = opt; }

  BitstreamOpt BitsOpt() { return m_bitstreamOpt; }
  void BitsOpt(BitstreamOpt opt) { m_bitstreamOpt = opt; }
  // Compiler specific opt
  const std::string& SynthMoreOpt() { return m_synthMoreOpt; }
  void SynthMoreOpt(const std::string& opt) { m_synthMoreOpt = opt; }

  void PnROpt(const std::string& opt) { m_pnrOpt = opt; }
  const std::string& PnROpt() { return m_pnrOpt; }

 protected:
  /* Methods that can be customized for each new compiler flow */
  virtual bool IPGenerate();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();

  bool CreateDesign(const std::string& name);

  /* Compiler class utilities */
  bool RunBatch();
  bool RunCompileTask(Action action);
  virtual bool ExecuteSystemCommand(const std::string& command);
  virtual int ExecuteAndMonitorSystemCommand(const std::string& command);
  std::string ReplaceAll(std::string_view str, std::string_view from,
                         std::string_view to);
  bool FileExists(const std::filesystem::path& name);
  void Tokenize(std::string_view str, std::string_view separator,
                std::vector<std::string>& result);
  std::string& Trim(std::string& str) { return Ltrim(Rtrim(str)); }
  std::string& Ltrim(std::string& str);
  std::string& Rtrim(std::string& str);

  time_t Mtime(const std::filesystem::path& path);
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
  bool m_hardError = false;

  // Tasks generic options
  IPGenerateOpt m_ipGenerateOpt = IPGenerateOpt::None;
  SynthesisOpt m_synthOpt = SynthesisOpt::None;
  PackingOpt m_packingOpt = PackingOpt::None;
  GlobalPlacementOpt m_globalPlacementOpt = GlobalPlacementOpt::None;
  PlacementOpt m_placementOpt = PlacementOpt::None;
  RoutingOpt m_routingOpt = RoutingOpt::None;
  PowerOpt m_powerOpt = PowerOpt::None;
  STAOpt m_staOpt = STAOpt::None;
  BitstreamOpt m_bitstreamOpt = BitstreamOpt::NoBitsOpt;

  // Compiler specific options
  std::string m_pnrOpt;
  std::string m_synthMoreOpt;

  // VPR, Yosys options
  uint32_t m_channel_width = 100;
  uint32_t m_lut_size = 6;

  class QProcess* m_process = nullptr;
  IPGenerator* m_IPGenerator = nullptr;
};

}  // namespace FOEDAG

#endif
