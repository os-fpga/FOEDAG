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

using namespace FOEDAG;

CompilerOpenFPGA::CompilerOpenFPGA() : Compiler::Compiler() {}

CompilerOpenFPGA::~CompilerOpenFPGA() { delete m_taskManager; }

bool CompilerOpenFPGA::Synthesize() {
  if (m_design == nullptr) {
    std::string name = "noname";
    Design* design = new Design(name);
    SetDesign(design);
    Message(std::string("Created design: ") + name + std::string("\n"));
  }
  (*m_out) << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
  const std::string basicYosysScript = R"(
   <FILE_LIST>
   synth -flatten -top <TOP_MODULE>
   abc -lut 6
   opt_clean
   write_verilog openfpga_synth_out.v
   write_blif openfpga_synth_out.blif
   stat
  )";
  std::string fileList;
  for (auto lang_file : m_design->FileList()) {
    fileList += "read_verilog " + lang_file.second + "\n";
  }
  std::string yosysScript = basicYosysScript;
  yosysScript = replaceAll(yosysScript, "<FILE_LIST>", fileList);
  yosysScript = replaceAll(yosysScript, "<TOP_MODULE>", m_design->TopLevel());
  std::ofstream ofs("openfpga.ys");
  ofs << yosysScript;
  ofs.close();
  ExecuteAndMonitorSystemCommand("./yosys -s openfpga.ys");

  m_state = State::Synthesized;
  (*m_out) << "Design " << m_design->Name() << " is synthesized!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::GlobalPlacement() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Synthesized) {
    (*m_out) << "ERROR: Design needs to be in synthesized state" << std::endl;
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_design->Name() << "..."
           << std::endl;

  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << m_design->Name() << " is globally placed!"
           << std::endl;
  return true;
}

bool CompilerOpenFPGA::Placement() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::Route() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}
