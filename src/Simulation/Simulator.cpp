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

bool Simulator::SimulateRTL(SimulatorType type) {
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

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    fileList +=
        LanguageDirective(type, (Design::Language)(lang_file.first.language)) +
        " ";
    fileList += lang_file.second + " ";
  }
  return true;
}

bool Simulator::SimulateGate(SimulatorType type) { return true; }
bool Simulator::SimulatePNR(SimulatorType type) { return true; }
bool Simulator::SimulateBitstream(SimulatorType type) { return true; }