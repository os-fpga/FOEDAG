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
#include "Simulator.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;

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
      for (int i = 3; i < argc; i++) {
        options += std::string(argv[i]) + " ";
      }
      std::string phase;
      Simulator::SimulatorType sim_tool = Simulator::SimulatorType::Verilator;
      for (int i = 1; i < 3; i++) {
        std::string arg = argv[i];
        if (arg == "verilator") {
          sim_tool = Simulator::SimulatorType::Verilator;
        } else if (arg == "icarus") {
          sim_tool = Simulator::SimulatorType::Icarus;
        } else if (arg == "ghdl") {
          sim_tool = Simulator::SimulatorType::GHDL;
        } else if (arg == "vcs") {
          sim_tool = Simulator::SimulatorType::VCS;
        } else if (arg == "questa") {
          sim_tool = Simulator::SimulatorType::Questa;
        } else if (arg == "xcelium") {
          sim_tool = Simulator::SimulatorType::Xcelium;
        } else if (arg == "compilation") {
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
        }
      }
      options = StringUtils::rtrim(options);
      if (phase == "compilation") {
        simulator->SetSimulatorCompileOption(sim_tool, options);
      } else if (phase == "elaboration") {
        simulator->SetSimulatorElaborationOption(sim_tool, options);
      } else if (phase == "simulation") {
        simulator->SetSimulatorRuntimeOption(sim_tool, options);
      }
      return TCL_OK;
    }
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

std::string Simulator::GetSimulatorCompileOption(SimulatorType type) {
  std::map<SimulatorType, std::string>::iterator itr =
      m_simulatorCompileOptionMap.find(type);
  if (itr != m_simulatorCompileOptionMap.end()) return (*itr).second;
  return "";
}
std::string Simulator::GetSimulatorElaborationOption(SimulatorType type) {
  std::map<SimulatorType, std::string>::iterator itr =
      m_simulatorElaborationOptionMap.find(type);
  if (itr != m_simulatorElaborationOptionMap.end()) return (*itr).second;
  return "";
}
std::string Simulator::GetSimulatorRuntimeOption(SimulatorType type) {
  std::map<SimulatorType, std::string>::iterator itr =
      m_simulatorRuntimeOptionMap.find(type);
  if (itr != m_simulatorRuntimeOptionMap.end()) return (*itr).second;
  return "";
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

bool Simulator::Simulate(SimulationType action, SimulatorType type,
                         const std::string& wave_file) {
  m_simType = action;
  if (SimulationOption() == SimulationOpt::Clean) return Clean(action);
  WaveFile(action, wave_file);
  m_waveFile = wave_file;
  if (wave_file.empty()) {
    m_waveFile = WaveFile(action);
  }
  if (m_waveFile.find(".vcd") != std::string::npos) {
    m_waveType = WaveformType::VCD;
  } else if (m_waveFile.find(".fst") != std::string::npos) {
    m_waveType = WaveformType::FST;
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
    case SimulationType::Bitstream: {
      return SimulateBitstream(type);
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

std::string Simulator::SimulatorName(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "verilator";
    case SimulatorType::Icarus:
      return "icarus";
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
      return "-v ";
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
      return "+libext+";
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
          "-Wno-WIDTH ";
      switch (m_waveType) {
        case WaveformType::VCD:
          options += "--trace ";
          break;
        case WaveformType::FST:
          options += "--trace-fst ";
          break;
      }
      return options;
      break;
    }
    case SimulatorType::Icarus:
      return "";
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
      return "Todo";
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

std::string Simulator::SimulatorRunCommand(SimulatorType type) {
  std::string execPath =
      (SimulatorExecPath(type) / SimulatorName(type)).string();
  auto simulationTop{ProjManager()->SimulationTopModule()};
  switch (type) {
    case SimulatorType::Verilator: {
      std::string command = "obj_dir/V" + simulationTop;
      if (!GetSimulatorRuntimeOption(type).empty())
        command += " " + GetSimulatorRuntimeOption(type);
      if (!m_waveFile.empty()) command += " " + m_waveFile;
      return command;
    }
    case SimulatorType::Icarus:
      return "Todo";
    case SimulatorType::GHDL: {
      std::string command = execPath + " -r -fsynopsys";
      if (!simulationTop.empty()) {
        command += TopModuleCmd(type) + simulationTop;
      }
      if (!GetSimulatorRuntimeOption(type).empty())
        command += " " + GetSimulatorRuntimeOption(type);
      if (!m_waveFile.empty()) {
        command += " ";
        command += (m_waveType == WaveformType::VCD) ? "--vcd=" : "--fst=";
        command += m_waveFile;
      }
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

std::string Simulator::SimulationFileList(SimulatorType type) {
  std::string fileList;
  m_compiler->CustomSimulatorSetup();
  if (type != SimulatorType::GHDL) {
    auto simulationTop{ProjManager()->SimulationTopModule()};
    if (!simulationTop.empty()) {
      fileList += TopModuleCmd(type) + simulationTop + " ";
    }
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

  for (auto& macro_value : ProjManager()->macroList()) {
    fileList += MacroDirective(type) + macro_value.first + "=" +
                macro_value.second + " ";
  }

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
  return fileList;
}

int Simulator::SimulationJob(SimulatorType type, const std::string& fileList) {
  if (type == SimulatorType::Verilator) {
    std::string verilator_home = SimulatorExecPath(type).parent_path().string();
    m_compiler->SetEnvironmentVariable("VERILATOR_ROOT", verilator_home);
  }

  // Simulator Model compilation step
  std::string execPath =
      (SimulatorExecPath(type) / SimulatorName(type)).string();
  std::string command = execPath + " " + SimulatorCompilationOptions(type);
  if (!GetSimulatorCompileOption(type).empty())
    command += " " + GetSimulatorCompileOption(type);
  command += " " + fileList;
  int status = m_compiler->ExecuteAndMonitorSystemCommand(command);
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
      if (!GetSimulatorElaborationOption(type).empty())
        command += " " + GetSimulatorElaborationOption(type);
      status = m_compiler->ExecuteAndMonitorSystemCommand(command);
      if (status) {
        ErrorMessage("Design " + ProjManager()->projectName() +
                     " simulation compilation failed!\n");
        return status;
      }
      break;
    }
    case SimulatorType::GHDL: {
      std::string command = execPath + " -e -fsynopsys";
      if (!GetSimulatorElaborationOption(type).empty())
        command += " " + GetSimulatorElaborationOption(type);
      if (!simulationTop.empty()) {
        command += TopModuleCmd(type) + simulationTop;
      }
      status = m_compiler->ExecuteAndMonitorSystemCommand(command);
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
  command = SimulatorRunCommand(type);
  status = m_compiler->ExecuteAndMonitorSystemCommand(command);
  return status;
}

bool Simulator::SimulateRTL(SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;

  std::string fileList = SimulationFileList(type);
  bool langDirective = false;
  if (type == SimulatorType::GHDL) {
    if (fileList.find("--std=") != std::string::npos) {
      langDirective = true;
    }
  }
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

  fileList = StringUtils::rtrim(fileList);

  PERF_LOG("RTL Simulation has started");
  Message("##################################################");
  Message("RTL simulation for design: " + ProjManager()->projectName());
  Message("##################################################");

  bool status = SimulationJob(type, fileList);

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

  std::string fileList = SimulationFileList(type);

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

  bool status = SimulationJob(type, fileList);

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

  std::string fileList = SimulationFileList(type);

  std::string netlistFile =
      ProjManager()->getDesignTopModule().toStdString() + "_post_synthesis.v";

  fileList += " " + netlistFile + " ";
  for (auto path : m_gateSimulationModels) {
    fileList += LibraryFileDirective(type) + path.string() + " ";
  }
  fileList = StringUtils::rtrim(fileList);

  bool status = SimulationJob(type, fileList);

  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " simulation failed!\n");
    return false;
  }

  Message("Post-PnR simulation for design: " + ProjManager()->projectName() +
          " had ended");

  return true;
}
bool Simulator::SimulateBitstream(SimulatorType type) {
  if (!ProjManager()->HasDesign() && !m_compiler->CreateDesign("noname"))
    return false;
  if (!m_compiler->HasTargetDevice()) return false;
  PERF_LOG("Bitstream Simulation has started");
  Message("##################################################");
  Message("Bitstream simulation for design: " + ProjManager()->projectName());
  Message("##################################################");
  return true;
}
