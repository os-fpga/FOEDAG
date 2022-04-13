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

// https://github.com/lnis-uofu/OpenFPGA/blob/master/openfpga_flow/misc/ys_tmpl_yosys_vpr_flow.ys
const std::string basicYosysScript = R"( 
# Yosys synthesis script for ${TOP_MODULE}
# Read verilog files
read_verilog -nolatches ${READ_VERILOG_OPTIONS} ${VERILOG_FILES}

# Technology mapping
hierarchy -top ${TOP_MODULE}
proc
techmap -D NO_LUT -map +/adff2dff.v

# Synthesis
flatten
opt_expr
opt_clean
check
opt -nodffe -nosdff
fsm
opt -nodffe -nosdff
wreduce
peepopt
opt_clean
opt -nodffe -nosdff
memory -nomap
opt_clean
opt -fast -full -nodffe -nosdff
memory_map
opt -full -nodffe -nosdff
techmap
opt -fast -nodffe -nosdff
clean

# LUT mapping
abc -lut ${LUT_SIZE}

# Check
synth -run check

# Clean and output blif
opt_clean -purge
write_blif ${OUTPUT_BLIF}
  )";

bool CompilerOpenFPGA::IPGenerate() {
  if (!CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " IPs are generated!"
           << std::endl;
  m_state = IPGenerated;
  return true;
}

bool CompilerOpenFPGA::Synthesize() {
  if (!CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << m_design->Name() << "..." << std::endl;

  if (m_yosysScript.empty()) {
    m_yosysScript = basicYosysScript;
  }
  std::string fileList;
  for (const auto &lang_file : m_design->FileList()) {
    fileList += lang_file.second + " ";
  }
  std::string yosysScript = m_yosysScript;
  yosysScript = replaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", "");
  yosysScript = replaceAll(yosysScript, "${LUT_SIZE}", std::to_string(6));
  yosysScript = replaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  yosysScript = replaceAll(yosysScript, "${TOP_MODULE}", m_design->TopLevel());
  yosysScript = replaceAll(yosysScript, "${OUTPUT_BLIF}",
                           std::string(m_design->Name() + "_post_synth.blif"));
  std::ofstream ofs("foedag.ys");
  ofs << yosysScript;
  ofs.close();
  std::string command = m_yosysExecutablePath.string() + " -s foedag.ys";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " synthesis was interrupted!"
             << std::endl;
    return false;
  }
  m_state = State::Synthesized;
  (*m_out) << "Design " << m_design->Name() << " is synthesized!" << std::endl;
  return true;
}

std::string CompilerOpenFPGA::getBaseVprCommand() {
  std::string command = m_vprExecutablePath.string() + std::string(" ") +
                        m_architectureFile.string() + std::string(" ") +
                        std::string(m_design->Name() + "_post_synth.blif");
  return command;
}

bool CompilerOpenFPGA::Packing() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  std::string command = getBaseVprCommand() + " --pack";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " packing was interrupted!"
             << std::endl;
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << m_design->Name() << " is packed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA::GlobalPlacement() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    (*m_out) << "ERROR: Design needs to be in packed state" << std::endl;
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_design->Name() << "..."
           << std::endl;
  // TODO:
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
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    (*m_out) << "ERROR: Design needs to be in packed or globally placed state"
             << std::endl;
    return false;
  }
  (*m_out) << "Placement for design: " << m_design->Name() << "..."
           << std::endl;
  std::string command = getBaseVprCommand() + " --place";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " placement was interrupted!"
             << std::endl;
    return false;
  }
  m_state = State::Placed;
  (*m_out) << "Design " << m_design->Name() << " is placed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA::Route() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Placed) {
    (*m_out) << "ERROR: Design needs to be in placed state" << std::endl;
    return false;
  }
  (*m_out) << "Routing for design: " << m_design->Name() << "..." << std::endl;
  std::string command = getBaseVprCommand() + " --route";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " routing was interrupted!"
             << std::endl;
    return false;
  }
  m_state = State::Routed;
  (*m_out) << "Design " << m_design->Name() << " is routed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }

  (*m_out) << "Analysis for design: " << m_design->Name() << "..." << std::endl;
  std::string command = getBaseVprCommand() + " --analysis";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " analysis was interrupted!"
             << std::endl;
    return false;
  }

  (*m_out) << "Design " << m_design->Name() << " is analysed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }

  (*m_out) << "Analysis for design: " << m_design->Name() << "..." << std::endl;
  std::string command = getBaseVprCommand() + " --analysis";
  bool status = ExecuteAndMonitorSystemCommand(command);
  if (status == false) {
    (*m_out) << "Design " << m_design->Name() << " analysis was interrupted!"
             << std::endl;
    return false;
  }

  (*m_out) << "Design " << m_design->Name() << " is analysed!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Routed) {
    (*m_out) << "ERROR: Design needs to be in routed state" << std::endl;
    return false;
  }
  // TODO:
  (*m_out) << "Bitstream generation for design: " << m_design->Name() << "..."
           << std::endl;
  (*m_out) << "Design " << m_design->Name() << " bitstream is generated!"
           << std::endl;
  return true;
}
