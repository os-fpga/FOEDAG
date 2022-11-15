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

bool Simulator::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  bool ok = true;
  return ok;
}

void Simulator::Message(const std::string& message) {
  m_compiler->Message(message);
}
void Simulator::ErrorMessage(const std::string& message) {
  m_compiler->ErrorMessage(message);
}

bool Simulator::Simulate(SimulationType action, SimulatorType type) {
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

std::string Simulator::SimulatorOptions(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "-cc -Wall";
    case SimulatorType::Icarus:
      return "";
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
    case SimulatorType::Questa:
      return "-D";
    case SimulatorType::VCS:
      return "-D";
    case SimulatorType::Xcelium:
      return "-D";
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
        default:
          return "--invalid-lang-for-verilator";
      }
    case SimulatorType::Icarus:
    case SimulatorType::Questa:
    case SimulatorType::VCS:
    case SimulatorType::Xcelium:
    default:
      return "Invalid";
  }
  return "Invalid";
}

std::string Simulator::SimulatorRunCommand(SimulatorType type) {
  switch (type) {
    case SimulatorType::Verilator:
      return "obj_dir/Vsyn_tb";
    case SimulatorType::Icarus:
      return "Invalid";
    case SimulatorType::Questa:
      return "Invalid";
    case SimulatorType::VCS:
      return "simv";
    case SimulatorType::Xcelium:
      return "Invalid";
  }
  return "Invalid";
}

std::string Simulator::SimulationFileList(SimulatorType type) {
  std::string fileList;

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

  for (const auto& lang_file : ProjManager()->SimulationFiles()) {
    fileList +=
        LanguageDirective(type, (Design::Language)(lang_file.first.language)) +
        " ";
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
    std::cout << "VERILATOR_HOME=" << verilator_home << std::endl;
    m_compiler->SetEnvironmentVariable("VERILATOR_ROOT", verilator_home);
  }

  // Simulator Model compilation step
  std::string execPath =
      (SimulatorExecPath(type) / SimulatorName(type)).string();
  std::string command =
      execPath + " " + SimulatorOptions(type) + " " + fileList;

  int status = m_compiler->ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " simulation compilation failed!\n");
    return status;
  }
  // Extra Simulator Model compilation step
  if (type == SimulatorType::Verilator) {
    std::string command = "make -j -C obj_dir/ -f Vsyn_tb.mk Vsyn_tb";
    status = m_compiler->ExecuteAndMonitorSystemCommand(command);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " simulation compilation failed!\n");
      return status;
    }
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

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    fileList +=
        LanguageDirective(type, (Design::Language)(lang_file.first.language)) +
        " ";
    fileList += lang_file.second + " ";
  }

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

  fileList += " " + netlistFile;
  for (auto path : m_gateSimulationModels) {
    fileList += " -v " + path.string();
  }

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