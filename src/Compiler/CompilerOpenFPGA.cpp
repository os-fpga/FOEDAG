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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <QDebug>
#include <chrono>
#include <filesystem>
#include <thread>

#include "Compiler/CompilerOpenFPGA.h"
#include "DesignManager.h"

using namespace FOEDAG;

CompilerOpenFPGA::CompilerOpenFPGA() : Compiler::Compiler() {}

CompilerOpenFPGA::~CompilerOpenFPGA() {}

bool CompilerOpenFPGA::Synthesize() {
  if (GetActiveDesign() == nullptr) {
    std::string name = "noname";
    Design* design = new Design(name);
    SetDesign(design);
    Message(std::string("Created design: ") + name + std::string("\n"));
  }
  auto design = m_designManager->activeDesign();
  (*m_out) << "Synthesizing design: " << design->Name() << "..." << std::endl;
  const std::string basicYosysScript = R"(
   <FILE_LIST>
   synth -flatten -top <TOP_MODULE>
   abc -lut 6
   opt_clean
   write_verilog <NETLIST_NAME>.v
   write_blif <NETLIST_NAME>.blif
   stat
  )";
  std::string fileList;
  for (auto lang_file : design->FileList()) {
    fileList += "read_verilog " + lang_file.second + "\n";
  }
  std::string yosysScript = basicYosysScript;
  yosysScript = replaceAll(yosysScript, "<FILE_LIST>", fileList);
  yosysScript = replaceAll(yosysScript, "<TOP_MODULE>", design->TopLevel());
  yosysScript = replaceAll(yosysScript, "<NETLIST_NAME>", "foedag_post_synth");
  std::ofstream ofs("foedag.ys");
  ofs << yosysScript;
  ofs.close();
  std::string command = m_yosysExecutablePath.string() + " -s foedag.ys";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << design->Name() << " synthesis was interrupted!"
             << std::endl;
    return false;
  }
  m_state = State::Synthesized;
  (*m_out) << "Design " << design->Name() << " is synthesized!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::GlobalPlacement() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Synthesized) {
    (*m_out) << "ERROR: Design needs to be in synthesized state" << std::endl;
    return false;
  }
  (*m_out) << "Global Placement for design: " << design->Name() << "..."
           << std::endl;

  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << design->Name() << " is globally placed!"
           << std::endl;
  return true;
}

bool CompilerOpenFPGA::Placement() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::Route() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  auto design = m_designManager->activeDesign();
  if (design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}
