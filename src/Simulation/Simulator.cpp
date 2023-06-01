/*
Copyright 2021-2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <charconv>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/Log.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Simulator.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;

Simulator::SimulationType Simulator::ToSimulationType(const std::string& str,
                                                      bool& ok) {
  ok = true;
  if (str == "rtl") return SimulationType::RTL;
  if (str == "pnr") return SimulationType::PNR;
  if (str == "gate") return SimulationType::Gate;
  if (str == "bitstream_bd") return SimulationType::BitstreamBackDoor;
  if (str == "bitstream_fd") return SimulationType::BitstreamFrontDoor;
  ok = false;
  return SimulationType::RTL;
}

Simulator::SimulatorType Simulator::ToSimulatorType(
    const std::string& str, bool& ok, SimulatorType defaultValue) {
  Simulator::SimulatorType sim_tool = defaultValue;
  ok = true;
  if (str == "verilator") {
    sim_tool = Simulator::SimulatorType::Verilator;
  } else if (str == "icarus") {
    sim_tool = Simulator::SimulatorType::Icarus;
  } else if (str == "ghdl") {
    sim_tool = Simulator::SimulatorType::GHDL;
  } else if (str == "vcs") {
    sim_tool = Simulator::SimulatorType::VCS;
  } else if (str == "questa") {
    sim_tool = Simulator::SimulatorType::Questa;
  } else if (str == "xcelium") {
    sim_tool = Simulator::SimulatorType::Xcelium;
  } else {
    ok = false;
  }
  return sim_tool;
}

std::string Simulator::ToString(SimulatorType type) {
  switch (type) {
    case Simulator::SimulatorType::GHDL:
      return "ghdl";
    case Simulator::SimulatorType::Icarus:
      return "icarus";
    case Simulator::SimulatorType::Questa:
      return "questa";
    case Simulator::SimulatorType::VCS:
      return "vcs";
    case Simulator::SimulatorType::Verilator:
      return "verilator";
    case Simulator::SimulatorType::Xcelium:
      return "xcelium";
  }
  return std::string{};
}

Simulator::Simulator(TclInterpreter* interp, Compiler* compiler,
                     std::ostream* out,
                     TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_compiler(compiler),
      m_out(out),
      m_tclInterpreterHandler(tclInterpreterHandler) {}

void Simulator::AddGateSimulationModel(const std::filesystem::path& path) {
  m_gateSimulationModels.push_back(path);
}

void Simulator::SetSimulatorCompileOption(const std::string& simulation,
                                          SimulatorType type,
                                          const std::string& options) {
  bool ok{false};
  auto level = ToSimulationType(simulation, ok);
  if (ok) {
    m_simulatorCompileOptionMap[level][type] = options;
  } else {
    for (auto level : {SimulationType::RTL, SimulationType::PNR,
                       SimulationType::Gate, SimulationType::BitstreamBackDoor,
                       SimulationType::BitstreamFrontDoor})
      m_simulatorCompileOptionMap[level][type] = options;
  }
}

void Simulator::SetSimulatorElaborationOption(const std::string& simulation,
                                              SimulatorType type,
                                              const std::string& options) {
  bool ok{false};
  auto level = ToSimulationType(simulation, ok);
  if (ok) {
    m_simulatorElaborationOptionMap[level][type] = options;
  } else {
    for (auto level : {SimulationType::RTL, SimulationType::PNR,
                       SimulationType::Gate, SimulationType::BitstreamBackDoor,
                       SimulationType::BitstreamFrontDoor})
      m_simulatorElaborationOptionMap[level][type] = options;
  }
}

void Simulator::SetSimulatorExtraOption(const std::string& simulation,
                                        SimulatorType type,
                                        const std::string& options) {
  bool ok{false};
  auto level = ToSimulationType(simulation, ok);
  if (ok) {
    m_simulatorExtraOptionMap[level][type] = options;
  } else {
    for (auto level : {SimulationType::RTL, SimulationType::PNR,
                       SimulationType::Gate, SimulationType::BitstreamBackDoor,
                       SimulationType::BitstreamFrontDoor})
      m_simulatorExtraOptionMap[level][type] = options;
  }
}

void Simulator::SetSimulatorSimulationOption(const std::string& simulation,
                                             SimulatorType type,
                                             const std::string& options) {
  bool ok{false};
  auto level = ToSimulationType(simulation, ok);
  if (ok) {
    m_simulatorSimulationOptionMap[level][type] = options;
  } else {
    for (auto level : {SimulationType::RTL, SimulationType::PNR,
                       SimulationType::Gate, SimulationType::BitstreamBackDoor,
                       SimulationType::BitstreamFrontDoor})
      m_simulatorSimulationOptionMap[level][type] = options;
  }
}

void Simulator::ResetGateSimulationModel() { m_gateSimulationModels.clear(); }

bool Simulator::RegisterCommands(TclInterpreter* interp) {
  bool ok = true;
  auto set_top_testbench = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    Simulator* simulator = (Simulator*)clientData;
    if (argc == 2) {
      simulator->ProjManager()->setTopModuleSim(QString::fromLatin1(argv[1]));
      return TCL_OK;
    }
    return TCL_ERROR;
  };
  interp->registerCmd("set_top_testbench", set_top_testbench, this, 0);

  auto simulation_options = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    Simulator* simulator = (Simulator*)clientData;
    if (argc > 3) {
      std::string options;
      std::string phase;
      std::string level;
      Simulator::SimulatorType sim_tool = Simulator::SimulatorType::Icarus;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool ok{false};
        auto tmp = Simulator::ToSimulatorType(arg, ok);
        if (ok) sim_tool = tmp;
        if (arg == "compilation") {
          phase = "compilation";
        } else if (arg == "comp") {
          phase = "compilation";
        } else if (arg == "elaboration") {
          phase = "elaboration";
        } else if (arg == "elab") {
          phase = "elaboration";
        } else if (arg == "simul") {
          phase = "simulation";
        } else if (arg == "simulation") {
          phase = "simulation";
        } else if (arg == "extra_options") {
          phase = "extra_options";
        } else if (arg == "rtl" || arg == "pnr" || arg == "gate" ||
                   arg == "bitstream_bd" || arg == "bitstream_fd") {
          level = arg;
        } else {
          // skip level if present
          if (arg == "rtl" || arg == "pnr" || arg == "gate" ||
              arg == "bitstream_bd" || arg == "bitstream_fd") {
            continue;
          }
          // skip simulation name if present
          if (arg == "verilator" || arg == "icarus" || arg == "ghdl" ||
              arg == "vcs" || arg == "questa" || arg == "xcelium") {
            continue;
          }
          options += arg + " ";
        }
      }
      options = StringUtils::rtrim(options);
      if (phase == "compilation") {
        simulator->SetSimulatorCompileOption(level, sim_tool, options);
      } else if (phase == "elaboration") {
        simulator->SetSimulatorElaborationOption(level, sim_tool, options);
      } else if (phase == "simulation") {
        simulator->SetSimulatorSimulationOption(level, sim_tool, options);
      } else if (phase == "extra_options") {
        simulator->SetSimulatorExtraOption(level, sim_tool, options);
      }
      if (!phase.empty()) {
        if (simulator->m_compiler->GuiTclSync())
          simulator->m_compiler->GuiTclSync()->saveSettings();
        return TCL_OK;
      }
    }
    Tcl_AppendResult(interp,
                     "Invalid arguments. Usage: simulation_options <simulator> "
                     "<phase> ?<level>? <options>",
                     nullptr);
    return TCL_ERROR;
  };
  interp->registerCmd("simulation_options", simulation_options, this, 0);

  return ok;
}

bool Simulator::Clean(SimulationType action) {
  Message("Cleaning simulation results for " + ProjManager()->projectName());
  auto waveFile = m_waveFiles.find(action);
  if (waveFile != m_waveFiles.end()) {
    auto filePath =
        std::filesystem::path(ProjManager()->projectPath()) / waveFile->second;
    if (FileUtils::FileExists(filePath)) std::filesystem::remove(filePath);
    auto logPath =
        std::filesystem::path(ProjManager()->projectPath()) / LogFile(action);
    if (FileUtils::FileExists(logPath)) std::filesystem::remove(logPath);
  }
  auto obj_dir =
      std::filesystem::path(ProjManager()->projectPath()) / "obj_dir";
  if (FileUtils::FileExists(obj_dir)) std::filesystem::remove_all(obj_dir);
  for (auto& de :
       std::filesystem::directory_iterator(ProjManager()->projectPath())) {
    if ((de.path().extension() == ".cf") || de.path().extension() == ".out")
      std::filesystem::remove(de.path());
  }
  SimulationOption(SimulationOpt::None);
  return true;
}

void Simulator::Message(const std::string& message) {
  m_compiler->Message(message);
}
void Simulator::ErrorMessage(const std::string& message) {
  m_compiler->ErrorMessage(message);
}

std::string Simulator::GetSimulatorCompileOption(SimulationType simulation,
                                                 SimulatorType type) {
  if (m_simulatorCompileOptionMap.count(simulation) != 0) {
    auto itr = m_simulatorCompileOptionMap[simulation].find(type);
    if (itr != m_simulatorCompileOptionMap[simulation].end())
      return (*itr).second;
  }
  return std::string{};
}

std::string Simulator::GetSimulatorElaborationOption(SimulationType simulation,
                                                     SimulatorType type) {
  if (m_simulatorElaborationOptionMap.count(simulation) != 0) {
    auto itr = m_simulatorElaborationOptionMap[simulation].find(type);
    if (itr != m_simulatorElaborationOptionMap[simulation].end())
      return (*itr).second;
  }
  return std::string{};
}

std::string Simulator::GetSimulatorExtraOption(SimulationType simulation,
                                               SimulatorType type) {
  if (m_simulatorExtraOptionMap.count(simulation) != 0) {
    auto itr = m_simulatorExtraOptionMap[simulation].find(type);
    if (itr != m_simulatorExtraOptionMap[simulation].end())
      return (*itr).second;
  }
  return std::string{};
}

std::string Simulator::GetSimulatorSimulationOption(SimulationType simulation,
                                                    SimulatorType type) {
  if (m_simulatorSimulationOptionMap.count(simulation) != 0) {
    auto itr = m_simulatorSimulationOptionMap[simulation].find(type);
    if (itr != m_simulatorSimulationOptionMap[simulation].end())
      return (*itr).second;
  }
  return std::string{};
}

void Simulator::SimulationOption(SimulationOpt option) {
  m_simulationOpt = option;
}

Simulator::SimulationOpt Simulator::SimulationOption() const {
  return m_simulationOpt;
}

void Simulator::WaveFile(SimulationType type, const std::string& file) {
  if (!file.empty()) m_waveFiles[type] = file;
}

std::string Simulator::WaveFile(SimulationType type) const {
  if (m_waveFiles.count(type) != 0) return m_waveFiles.at(type);
  return std::string{};
}

void Simulator::UserSimulationType(SimulationType simulation,
                                   SimulatorType simulator) {
  // create new item if not exists
  m_simulatorTypes[simulation] = simulator;
}

Simulator::SimulatorType Simulator::UserSimulationType(
    SimulationType simulation, bool& ok) const {
  if (m_simulatorTypes.count(simulation) != 0) {
    ok = true;
    return m_simulatorTypes.at(simulation);
  }
  ok = false;
  return SimulatorType::Verilator;
}

bool Simulator::Simulate(SimulationType action, SimulatorType type,
                         const std::string& wave_file) {
  m_simType = action;
  if (SimulationOption() == SimulationOpt::Clean) return Clean(action);
  WaveFile(action, wave_file);
  UserSimulationType(action, type);
  m_waveFile = wave_file;
  if (wave_file.empty()) {
    m_waveFile = WaveFile(action);
  }
  if (m_waveFile.find(".vcd") != std::string::npos) {
    m_waveType = WaveformType::VCD;
  } else if (m_waveFile.find(".fst") != std::string::npos) {
    m_waveType = WaveformType::FST;
  } else if (m_waveFile.find(".ghw") != std::string::npos) {
    m_waveType = WaveformType::GHW;
  }
  switch (action) {
    case SimulationType::RTL: {
      return SimulateRTL(type);
      break;
    }
    case SimulationType::Gate: {
      return SimulateGate(type);
      break;
    }
    case SimulationType::PNR: {
      return SimulatePNR(type);
      break;
    }
    case SimulationType::BitstreamFrontDoor: {
      return SimulateBitstream(action, type);
      break;
    }
    case SimulationType::BitstreamBackDoor: {
      return SimulateBitstream(action, type);
      break;
    }
  }
  return false;
}

class ProjectManager* Simulator::ProjManager() const {
  return m_compiler->ProjManager();
}

std::string Simulator::FileList(SimulationType action) {
  std::string list;

  return list;
}

std::string Simulator::LogFile(SimulationType type) {
  switch (type) {
    case Simulator::SimulationType::RTL:
      return std::string{"simulation_rtl.rpt"};
    case Simulator::SimulationType::Gate:
      return std::string{"simulation_gate.rpt"};
    case Simulator::SimulationType::PNR:
      return std::string{"simulation_pnr.rpt"};
    case Simulator::SimulationType::BitstreamFrontDoor:
      return std::string{"simulation_bitstream_front.rpt"};
    case Simulator::SimulationType::BitstreamBackDoor:
      return std::string{"simulation_bitstream_back.rpt"};
  }
  return std::string{};
}

std::string Simulator::SimulatorName(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "verilator";
    case SimulatorType::Icarus:
      return "iverilog";
    case SimulatorType::GHDL:
      return "ghdl";
    case SimulatorType::Questa:
      return "questa";
    case SimulatorType::VCS:
      return "vcs";
    case SimulatorType::Xcelium:
      return "xcelium";
  }
  return "Invalid";
}

std::string Simulator::IncludeDirective(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "-I";
    case SimulatorType::Icarus:
      return "-I";
    case SimulatorType::GHDL:
      return "Invalid";
    case SimulatorType::Questa:
      return "-I";
    case SimulatorType::VCS:
      return "+incdir+";
    case SimulatorType::Xcelium:
      return "-I";
  }
  return "Invalid";
}

std::string Simulator::LibraryPathDirective(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "-y ";
    case SimulatorType::Icarus:
      return "-y ";
    case SimulatorType::GHDL:
      return "-P";
    case SimulatorType::Questa:
      return "-y ";
    case SimulatorType::VCS:
      return "-y ";
    case SimulatorType::Xcelium:
      return "-y ";
  }
  return "Invalid";
}

std::string Simulator::LibraryFileDirective(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "-v ";
    case SimulatorType::Icarus:
      return "-l ";
    case SimulatorType::GHDL:
      return "";
    case SimulatorType::Questa:
      return "-v ";
    case SimulatorType::VCS:
      return "-v ";
    case SimulatorType::Xcelium:
      return "-v ";
  }
  return "Invalid";
}

std::string Simulator::LibraryExtDirective(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "+libext+";
    case SimulatorType::Icarus:
      return "-Y ";
    case SimulatorType::GHDL:
      return "Invalid";
    case SimulatorType::Questa:
      return "+libext+";
    case SimulatorType::VCS:
      return "+libext+";
    case SimulatorType::Xcelium:
      return "+libext+";
  }
  return "Invalid";
}

void Simulator::SetSimulatorPath(SimulatorType type, const std::string path) {
  m_simulatorPathMap.emplace(type, path);
}

std::filesystem::path Simulator::SimulatorExecPath(SimulatorType type) {
  std::map<SimulatorType, std::filesystem::path>::iterator itr =
      m_simulatorPathMap.find(type);
  if (itr != m_simulatorPathMap.end()) {
    return (*itr).second;
  }
  return "";
}

std::string Simulator::SimulatorCompilationOptions(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator: {
      std::string options =
          "-cc --assert -Wall -Wno-DECLFILENAME "
          "-Wno-UNUSEDSIGNAL "
          "-Wno-TIMESCALEMOD "
          "-Wno-WIDTH -Wno-fatal -Wno-BLKANDNBLK ";
      switch (m_waveType) {
        case WaveformType::VCD:
          options += "--trace ";
          break;
        case WaveformType::FST:
          options += "--trace-fst ";
          break;
        case WaveformType::GHW:
          break;
      }
      return options;
      break;
    }
    case SimulatorType::Icarus:
      return "-gno-specify -DIVERILOG=1 -v";
    case SimulatorType::GHDL:
      return "-a -fsynopsys";
    case SimulatorType::Questa:
      return "";
    case SimulatorType::VCS:
      return "";
    case SimulatorType::Xcelium:
      return "";
  }
  return "Invalid";
}

std::string Simulator::MacroDirective(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "-D";
    case SimulatorType::Icarus:
      return "-D";
    case SimulatorType::GHDL:
      return "Invalid";
    case SimulatorType::Questa:
      return "-D";
    case SimulatorType::VCS:
      return "-D";
    case SimulatorType::Xcelium:
      return "-D";
  }
  return "Invalid";
}

std::string Simulator::TopModuleCmd(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "--top-module ";
    case SimulatorType::Icarus:
      return "-s ";
    case SimulatorType::GHDL:
      return " ";
    case SimulatorType::Questa:
      return "Todo";
    case SimulatorType::VCS:
      return "Todo";
    case SimulatorType::Xcelium:
      return "Todo";
  }
  return "Invalid";
}

std::string Simulator::LanguageDirective(SimulatorType type,
                                         Design::Language lang) {
  switch (type) {
    case SimulatorType::Verilator:
      switch (lang) {
        case Design::Language::VERILOG_1995:
          return "+1364-1995ext+.v";
        case Design::Language::VERILOG_2001:
          return "+1364-2001ext+.v";
        case Design::Language::SYSTEMVERILOG_2005:
          return "+1364-2005ext+.v +1800-2005ext+.sv";
        case Design::Language::SYSTEMVERILOG_2009:
          return "+1800-2009ext+.v +1800-2009ext+.sv";
        case Design::Language::SYSTEMVERILOG_2012:
          return "+1800-2012ext+.v +1800-2012ext+.sv";
        case Design::Language::SYSTEMVERILOG_2017:
          return "+1800-2017ext+.v +1800-2017ext+.sv";
        case Design::Language::VERILOG_NETLIST:
          return "";
        case Design::Language::C:
          return "";
        case Design::Language::CPP:
          return "";
        default:
          return "--invalid-lang-for-verilator";
      }
      break;
    case SimulatorType::Icarus:
      switch (lang) {
        case Design::Language::VERILOG_1995:
          return "-g1995";
        case Design::Language::VERILOG_2001:
          return "-g2001";
        case Design::Language::SYSTEMVERILOG_2005:
          return "-g2005";
        case Design::Language::SYSTEMVERILOG_2009:
          return "-g2009";
        case Design::Language::SYSTEMVERILOG_2012:
          return "-g2012";
        case Design::Language::SYSTEMVERILOG_2017:
          return "-g2012";
        case Design::Language::VERILOG_NETLIST:
          return "";
        case Design::Language::C:
          return "";
        case Design::Language::CPP:
          return "";
        default:
          return "--invalid-lang-for-icarus";
      }
      break;
    case SimulatorType::GHDL:
      switch (lang) {
        case Design::Language::VERILOG_1995:
          return "--invalid-lang-for-ghdl";
        case Design::Language::VERILOG_2001:
          return "--invalid-lang-for-ghdl";
        case Design::Language::SYSTEMVERILOG_2005:
          return "--invalid-lang-for-ghdl";
        case Design::Language::SYSTEMVERILOG_2009:
          return "--invalid-lang-for-ghdl";
        case Design::Language::SYSTEMVERILOG_2012:
          return "--invalid-lang-for-ghdl";
        case Design::Language::SYSTEMVERILOG_2017:
          return "--invalid-lang-for-ghdl";
        case Design::Language::VERILOG_NETLIST:
          return "--invalid-lang-for-ghdl";
        case Design::Language::C:
          return "--invalid-lang-for-ghdl";
        case Design::Language::CPP:
          return "--invalid-lang-for-ghdl";
        case Design::Language::VHDL_1987:
          if (m_simType == SimulationType::Gate ||
              m_simType == SimulationType::PNR) {
            return "--std=08";
          }
          return "--std=87";
        case Design::Language::VHDL_1993:
          if (m_simType == SimulationType::Gate ||
              m_simType == SimulationType::PNR) {
            return "--std=08";
          }
          return "--std=93c";
        case Design::Language::VHDL_2000:
          if (m_simType == SimulationType::Gate ||
              m_simType == SimulationType::PNR) {
            return "--std=08";
          }
          return "--std=00";
        case Design::Language::VHDL_2008:
          return "--std=08";
        case Design::Language::VHDL_2019:
          return "--std=19";
        default:
          return "--invalid-lang-for-ghdl";
      }
      break;
    case SimulatorType::Questa:
      break;
    case SimulatorType::VCS:
      break;
    case SimulatorType::Xcelium:
      break;
    default:
      return "Invalid";
  }
  return "Invalid";
}

std::string Simulator::SimulatorRunCommand(SimulationType simulation,
                                           SimulatorType type) {
  std::string execPath =
      (SimulatorExecPath(type) / SimulatorName(type)).string();
  auto simulationTop{ProjManager()->SimulationTopModule()};
  switch (type) {
    case SimulatorType::Verilator: {
      std::string command = "obj_dir/V" + simulationTop;
      if (!GetSimulatorSimulationOption(simulation, type).empty())
        command += " " + GetSimulatorSimulationOption(simulation, type);
      if (!m_waveFile.empty()) command += " " + m_waveFile;
      return command;
    }
    case SimulatorType::Icarus: {
      std::string command =
          (SimulatorExecPath(type) / "vvp").string() + " ./a.out";
      if (m_waveType == WaveformType::FST) {
        command += " -fst";
      }
      if (!m_waveFile.empty()) command += " -dumpfile=" + m_waveFile;
      return command;
    }
    case SimulatorType::GHDL: {
      std::string command = execPath + " -r -fsynopsys";
      if (!GetSimulatorExtraOption(simulation, type).empty())
        command += " " + GetSimulatorExtraOption(simulation, type);
      if (!simulationTop.empty()) {
        command += TopModuleCmd(type) + simulationTop;
      }
      if (!m_waveFile.empty()) {
        command += " ";
        switch (m_waveType) {
          case WaveformType::VCD:
            command += "--vcd=";
            break;
          case WaveformType::FST:
            command += "--fst=";
            break;
          case WaveformType::GHW:
            command += "--wave=";
            break;
        };
        command += m_waveFile;
      }
      if (!GetSimulatorSimulationOption(simulation, type).empty())
        command += " " + GetSimulatorSimulationOption(simulation, type);
      return command;
    }
    case SimulatorType::Questa:
      return "Todo";
    case SimulatorType::VCS:
      return "simv";
    case SimulatorType::Xcelium:
      return "Todo";
  }
  return "Invalid";
}

std::string Simulator::SimulationFileList(SimulationType action,
                                          SimulatorType type,
                                          const std::string& designFiles) {
  std::string fileList;
  m_compiler->CustomSimulatorSetup(action);
  if (type != SimulatorType::GHDL) {
    auto simulationTop{ProjManager()->SimulationTopModule()};
    if (!simulationTop.empty()) {
      fileList += TopModuleCmd(type) + simulationTop + " ";
    }
  }

  // macroses
  for (auto& macro_value : ProjManager()->macroList()) {
    fileList += MacroDirective(type) + macro_value.first + "=" +
                macro_value.second + " ";
  }

  // includes
  for (const auto& path : ProjManager()->includePathList()) {
    fileList += IncludeDirective(type) + FileUtils::AdjustPath(path) + " ";
  }

  if (type != SimulatorType::GHDL) {
    // VHDL has no include files
    std::set<std::string> designFileDirs;
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      const std::string& fileNames = lang_file.second;
      std::vector<std::string> files;
      StringUtils::tokenize(fileNames, " ", files);
      for (const auto& file : files) {
        std::filesystem::path filePath = file;
        filePath = filePath.parent_path();
        const std::string& path = filePath.string();
        if (designFileDirs.find(path) == designFileDirs.end()) {
          fileList +=
              IncludeDirective(type) + FileUtils::AdjustPath(path) + " ";
          designFileDirs.insert(path);
        }
      }
    }

    // Add simulation files directory as an include dir
    for (const auto& lang_file : ProjManager()->SimulationFiles()) {
      const std::string& fileNames = lang_file.second;
      std::vector<std::string> files;
      StringUtils::tokenize(fileNames, " ", files);
      for (const auto& file : files) {
        std::filesystem::path filePath = file;
        filePath = filePath.parent_path();
        const std::string& path = filePath.string();
        if (designFileDirs.find(path) == designFileDirs.end()) {
          fileList +=
              IncludeDirective(type) + FileUtils::AdjustPath(path) + " ";
          designFileDirs.insert(path);
        }
      }
    }
  }

  // libraries
  for (const auto& path : ProjManager()->libraryPathList()) {
    fileList += LibraryPathDirective(type) + FileUtils::AdjustPath(path) + " ";
  }

  // extensions
  for (const auto& ext : ProjManager()->libraryExtensionList()) {
    fileList += LibraryExtDirective(type) + ext + " ";
  }

  bool langDirective = false;
  // design files
  if (!designFiles.empty()) {
    fileList += designFiles;
    if (type == SimulatorType::GHDL) {
      if (fileList.find("--std=") != std::string::npos) {
        langDirective = true;
      }
    }
  }

  // simulation files
  for (const auto& lang_file : ProjManager()->SimulationFiles()) {
    if (langDirective == false) {
      Design::Language language = (Design::Language)lang_file.first.language;
      std::string directive = LanguageDirective(type, language);
      if (!directive.empty()) {
        langDirective = true;
        fileList += directive + " ";
      }
    }
    if (type == SimulatorType::Verilator) {
      if (lang_file.second.find(".c") != std::string::npos) {
        fileList += "--exe ";
      }
    }
    fileList += lang_file.second + " ";
  }
  return fileList;
}

int Simulator::SimulationJob(SimulationType simulation, SimulatorType type,
                             const std::string& fileList) {
  if (type == SimulatorType::Verilator) {
    std::string verilator_home = SimulatorExecPath(type).parent_path().string();
    m_compiler->SetEnvironmentVariable("VERILATOR_ROOT", verilator_home);
  }

  std::string log{LogFile(simulation)};
  // Simulator Model compilation step
  std::string execPath =
      (SimulatorExecPath(type) / SimulatorName(type)).string();
  std::string command = execPath + " " + SimulatorCompilationOptions(type);
  if (!GetSimulatorCompileOption(simulation, type).empty())
    command += " " + GetSimulatorCompileOption(simulation, type);
  command += " " + fileList;
  int status = m_compiler->ExecuteAndMonitorSystemCommand(command, log);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " simulation compilation failed!\n");
    return status;
  }

  // Extra Simulator Model compilation step (Elaboration or C++ compilation)
  auto simulationTop{ProjManager()->SimulationTopModule()};
  switch (type) {
    case SimulatorType::Verilator: {
      std::string command =
          "make -j -C obj_dir/ -f V" + simulationTop + ".mk V" + simulationTop;
      if (!GetSimulatorElaborationOption(simulation, type).empty())
        command += " " + GetSimulatorElaborationOption(simulation, type);
      status = m_compiler->ExecuteAndMonitorSystemCommand(command, log, true);
      if (status) {
        ErrorMessage("Design " + ProjManager()->projectName() +
                     " simulation compilation failed!\n");
        return status;
      }
      break;
    }
    case SimulatorType::GHDL: {
      std::string command = execPath + " -e -fsynopsys";
      if (!GetSimulatorElaborationOption(simulation, type).empty())
        command += " " + GetSimulatorElaborationOption(simulation, type);
      if (!simulationTop.empty()) {
        command += TopModuleCmd(type) + simulationTop;
      }
      status = m_compiler->ExecuteAndMonitorSystemCommand(command, log, true);
      if (status) {
        ErrorMessage("Design " + ProjManager()->projectName() +
                     " simulation compilation failed!\n");
        return status;
      }
      break;
    }
    default:
      break;
  }

  // Actual simulation
  command = SimulatorRunCommand(simulation, type);
  status = m_compiler->ExecuteAndMonitorSystemCommand(command, log, true);
  return status;
}

bool Simulator::SimulateRTL(SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;

  std::string fileList{};
  bool langDirective = false;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    if (langDirective == false) {
      std::string directive =
          LanguageDirective(type, (Design::Language)(lang_file.first.language));
      if (!directive.empty()) {
        langDirective = true;
        fileList += directive + " ";
      }
    }
    fileList += lang_file.second + " ";
  }

  fileList = SimulationFileList(SimulationType::RTL, type, fileList);
  fileList = StringUtils::rtrim(fileList);

  PERF_LOG("RTL Simulation has started");
  Message("##################################################");
  Message("RTL simulation for design: " + ProjManager()->projectName());
  Message("##################################################");

  bool status = SimulationJob(SimulationType::RTL, type, fileList);

  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " simulation failed!\n");
    return false;
  }

  Message("RTL simulation for design: " + ProjManager()->projectName() +
          " had ended");
  return true;
}

bool Simulator::SimulateGate(SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;
  PERF_LOG("Gate Simulation has started");
  Message("##################################################");
  Message("Gate simulation for design: " + ProjManager()->projectName());
  Message("##################################################");

  std::string fileList = SimulationFileList(SimulationType::Gate, type);

  std::string netlistFile;
  switch (m_compiler->GetNetlistType()) {
    case Compiler::NetlistType::Verilog:
      netlistFile = ProjManager()->projectName() + "_post_synth.v";
      break;
    case Compiler::NetlistType::VHDL:
      netlistFile = ProjManager()->projectName() + "_post_synth.vhd";
      break;
    case Compiler::NetlistType::Edif:
      netlistFile = ProjManager()->projectName() + "_post_synth.edif";
      break;
    case Compiler::NetlistType::Blif:
      netlistFile = ProjManager()->projectName() + "_post_synth.blif";
      break;
    case Compiler::NetlistType::EBlif:
      netlistFile = ProjManager()->projectName() + "_post_synth.eblif";
      break;
  }

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file.second;
        std::filesystem::path the_path = netlistFile;
        if (!the_path.is_absolute()) {
          netlistFile =
              std::filesystem::path(std::filesystem::path("..") / netlistFile)
                  .string();
        }
        break;
      }
      default:
        break;
    }
  }

  fileList += netlistFile + " ";
  for (auto path : m_gateSimulationModels) {
    fileList += LibraryFileDirective(type) + path.string() + " ";
  }
  fileList = StringUtils::rtrim(fileList);

  bool status = SimulationJob(SimulationType::Gate, type, fileList);

  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " simulation failed!\n");
    return false;
  }

  Message("Gate simulation for design: " + ProjManager()->projectName() +
          " had ended");

  return true;
}
bool Simulator::SimulatePNR(SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;
  PERF_LOG("Post-PnR Simulation has started");
  Message("##################################################");
  Message("Post-PnR simulation for design: " + ProjManager()->projectName());
  Message("##################################################");

  std::string fileList = SimulationFileList(SimulationType::PNR, type);

  std::string netlistFile =
      ProjManager()->getDesignTopModule().toStdString() + "_post_synthesis.v";

  fileList += " " + netlistFile + " ";
  for (auto path : m_gateSimulationModels) {
    fileList += LibraryFileDirective(type) + path.string() + " ";
  }
  fileList = StringUtils::rtrim(fileList);

  bool status = SimulationJob(SimulationType::PNR, type, fileList);

  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " Post-PnR simulation failed!\n");
    return false;
  }

  Message("Post-PnR simulation for design: " + ProjManager()->projectName() +
          " had ended");

  return true;
}

bool Simulator::SimulateBitstream(SimulationType sim_type, SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;
  PERF_LOG("Bitstream Simulation has started");
  Message("##################################################");
  Message("Bitstream simulation for design: " + ProjManager()->projectName());
  Message("##################################################");

  std::string fileList;

  bool langDirective = false;
  for (const auto& lang_file : ProjManager()->SimulationFiles()) {
    if (langDirective == false) {
      Design::Language language = (Design::Language)lang_file.first.language;
      std::string directive = LanguageDirective(type, language);
      if (!directive.empty()) {
        langDirective = true;
        fileList += directive + " ";
      }
    }
    if (type == SimulatorType::Verilator) {
      if (lang_file.second.find(".c") != std::string::npos) {
        fileList += "--exe ";
      }
    }
    fileList += lang_file.second + " ";
  }

  for (auto path : ProjManager()->includePathList()) {
    fileList += IncludeDirective(type) + FileUtils::AdjustPath(path) + " ";
  }

  for (auto path : ProjManager()->libraryPathList()) {
    fileList += LibraryPathDirective(type) + FileUtils::AdjustPath(path) + " ";
  }

  for (auto ext : ProjManager()->libraryExtensionList()) {
    fileList += LibraryExtDirective(type) + ext + " ";
  }

  fileList += LanguageDirective(type, Design::Language::SYSTEMVERILOG_2012) +
              " BIT_SIM/fabric_netlists.v";

  if (sim_type == SimulationType::BitstreamBackDoor) {
    fileList += " BIT_SIM/" +
                ProjManager()->getDesignTopModule().toStdString() +
                "_formal_random_top_tb.v";
    fileList += " BIT_SIM/" +
                ProjManager()->getDesignTopModule().toStdString() +
                "_top_formal_verification.v";
  } else {
    fileList += " BIT_SIM/" +
                ProjManager()->getDesignTopModule().toStdString() +
                "_autocheck_top_tb.v";
  }

  bool status = SimulationJob(sim_type, type, fileList);

  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " Bitstream simulation failed!\n");
    return false;
  }

  Message("Bitstream simulation for design: " + ProjManager()->projectName() +
          " had ended");
  return true;
}
