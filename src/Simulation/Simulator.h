/*
Copyright 2021-2022 The Foedag team

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
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Compiler/CompilerDefines.h"

namespace FOEDAG {
class TclInterpreter;
class TclInterpreterHandler;
class Session;
class Compiler;

class Simulator {
 public:
  enum class SimulatorType { Verilator, Icarus, GHDL, VCS, Questa, Xcelium };
  enum class SimulationType {
    RTL,
    Gate,
    PNR,
    BitstreamFrontDoor,
    BitstreamBackDoor
  };
  enum class WaveformType { VCD, FST, GHW };
  enum class SimulationOpt { None, Clean };

  static SimulationType ToSimulationType(const std::string& str, bool& ok);
  static SimulatorType ToSimulatorType(
      const std::string& str, bool& ok,
      SimulatorType defaultValue = SimulatorType::Icarus);
  static std::string ToString(SimulatorType type);
  // Most common use case, create the compiler in your main
  Simulator() = default;
  Simulator(TclInterpreter* interp, Compiler* compiler, std::ostream* out,
            TclInterpreterHandler* tclInterpreterHandler = nullptr);
  void SetInterpreter(TclInterpreter* interp) { m_interp = interp; }
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetErrStream(std::ostream* err) { m_err = err; };
  std::ostream* GetOutStream() { return m_out; }
  void SetTclInterpreterHandler(TclInterpreterHandler* tclInterpreterHandler);
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() const { return m_session; }
  virtual ~Simulator() {}
  bool Simulate(SimulationType action, SimulatorType type,
                const std::string& wave_file);
  TclInterpreter* TclInterp() { return m_interp; }
  bool RegisterCommands(TclInterpreter* interp);
  bool Clean(SimulationType action);

  std::string& getResult() { return m_result; }

  virtual void Message(const std::string& message);
  virtual void ErrorMessage(const std::string& message);
  void SetSimulatorType(SimulatorType type) { m_simulatorTool = type; }
  SimulatorType GetSimulatorType() { return m_simulatorTool; }

  void SetWaveformType(WaveformType type) { m_waveType = type; }
  WaveformType GetWaveformType() { return m_waveType; }

  void SetSimulatorPath(SimulatorType type, const std::string path);
  void ResetGateSimulationModel();
  void AddGateSimulationModel(const std::filesystem::path& path);

  void SetSimulatorCompileOption(const std::string& simulation,
                                 SimulatorType type,
                                 const std::string& options);
  void SetSimulatorElaborationOption(const std::string& simulation,
                                     SimulatorType type,
                                     const std::string& options);
  void SetSimulatorExtraOption(const std::string& simulation,
                               SimulatorType type, const std::string& options);
  void SetSimulatorSimulationOption(const std::string& simulation,
                                    SimulatorType type,
                                    const std::string& options);

  std::string GetSimulatorCompileOption(SimulationType simulation,
                                        SimulatorType type);
  std::string GetSimulatorElaborationOption(SimulationType simulation,
                                            SimulatorType type);
  std::string GetSimulatorExtraOption(SimulationType simulation,
                                      SimulatorType type);
  std::string GetSimulatorSimulationOption(SimulationType simulation,
                                           SimulatorType type);

  void SimulationOption(SimulationOpt option);
  SimulationOpt SimulationOption() const;

  void WaveFile(SimulationType type, const std::string& file);
  std::string WaveFile(SimulationType type) const;

  void UserSimulationType(SimulationType simulation, SimulatorType simulator);
  SimulatorType UserSimulationType(SimulationType simulation, bool& ok) const;

 protected:
  virtual bool SimulateRTL(SimulatorType type);
  virtual bool SimulateGate(SimulatorType type);
  virtual bool SimulatePNR(SimulatorType type);
  virtual bool SimulateBitstream(SimulationType sim_type,
                                 SimulatorType simulator_type);

  virtual std::string SimulatorName(SimulatorType type);
  virtual std::filesystem::path SimulatorExecPath(SimulatorType type);
  virtual std::string IncludeDirective(SimulatorType type);
  virtual std::string LibraryPathDirective(SimulatorType type);
  virtual std::string LibraryFileDirective(SimulatorType type);
  virtual std::string LibraryExtDirective(SimulatorType type);
  virtual std::string MacroDirective(SimulatorType type);
  virtual std::string TopModuleCmd(SimulatorType type);
  virtual std::string LanguageDirective(SimulatorType type,
                                        Design::Language lang);
  virtual std::string SimulationFileList(
      SimulationType action, SimulatorType type,
      const std::string& designFiles = std::string{});
  virtual int SimulationJob(SimulationType simulation, SimulatorType type,
                            const std::string& file_list);
  virtual std::string SimulatorRunCommand(SimulationType simulation,
                                          SimulatorType type);
  virtual std::string SimulatorCompilationOptions(SimulatorType type);
  class ProjectManager* ProjManager() const;
  std::string FileList(SimulationType action);
  static std::string LogFile(SimulationType type);
  /* Propected members */
  TclInterpreter* m_interp = nullptr;
  Compiler* m_compiler = nullptr;
  Session* m_session = nullptr;
  bool m_stop = false;
  std::ostream* m_out = &std::cout;
  std::ostream* m_err = &std::cerr;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler{nullptr};
  SimulatorType m_simulatorTool = SimulatorType::Icarus;
  std::string m_output;
  std::map<SimulatorType, std::filesystem::path> m_simulatorPathMap;
  using SimulationOptionMap = std::map<SimulatorType, std::string>;
  std::map<SimulationType, SimulationOptionMap> m_simulatorCompileOptionMap;
  std::map<SimulationType, SimulationOptionMap> m_simulatorElaborationOptionMap;
  std::map<SimulationType, SimulationOptionMap> m_simulatorExtraOptionMap;
  std::map<SimulationType, SimulationOptionMap> m_simulatorSimulationOptionMap;
  std::vector<std::filesystem::path> m_gateSimulationModels;
  std::string m_waveFile;
  WaveformType m_waveType = WaveformType::FST;
  SimulationOpt m_simulationOpt{SimulationOpt::None};
  std::map<SimulationType, std::string> m_waveFiles;
  std::map<SimulationType, SimulatorType> m_simulatorTypes;
  SimulationType m_simType = SimulationType::RTL;
};

}  // namespace FOEDAG

// declare metatype for QVariant usage
Q_DECLARE_METATYPE(FOEDAG::Simulator::SimulationType)

#endif
