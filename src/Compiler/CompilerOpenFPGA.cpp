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
#include "Compiler/Constraints.h"

using namespace FOEDAG;

void CompilerOpenFPGA::help(std::ostream* out) {
  (*out) << "----------------------------------" << std::endl;
  (*out) << "-----  FOEDAG OpenFPGA HELP  -----" << std::endl;
  (*out) << "----------------------------------" << std::endl;
  (*out) << "Options:" << std::endl;
  (*out) << "   --help           : This help" << std::endl;
  (*out) << "   --batch          : Tcl only, no GUI" << std::endl;
  (*out) << "   --replay <script>: Replay GUI test" << std::endl;
  (*out) << "   --script <script>: Execute a Tcl script" << std::endl;
  (*out) << "   --compiler <name>: Compiler name {openfpga...}, default is "
            "a dummy compiler"
         << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help" << std::endl;
  (*out) << "   gui_start" << std::endl;
  (*out) << "   gui_stop" << std::endl;
  (*out) << "   create_design <name>" << std::endl;
  (*out) << "   architecture <file> : Sets the architecture file" << std::endl;
  (*out) << "   add_design_file <file> <type> (VHDL_1987, VHDL_1993, "
            "VHDL_2008, V_1995, "
            "V_2001, SV_2005, SV_2009, SV_2012, SV_2017) "
         << std::endl;
  (*out) << "   set_top_module <top>" << std::endl;
  (*out) << "   add_constraint_file <file>: Sets SDC + location constraints"
         << std::endl;
  (*out) << "     Constraints: set_pin_loc, set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   batch { cmd1 ... cmdn } : Runs compilation script using the "
            "commands below"
         << std::endl;
  (*out) << "   ipgenerate" << std::endl;
  (*out) << "   synthesize" << std::endl;
  (*out) << "   packing" << std::endl;
  (*out) << "   global_placement" << std::endl;
  (*out) << "   place" << std::endl;
  (*out) << "   route" << std::endl;
  (*out) << "   sta" << std::endl;
  (*out) << "   power" << std::endl;
  (*out) << "   bitstream" << std::endl;
  (*out) << "   tcl_exit" << std::endl;
  (*out) << "----------------------------------" << std::endl;
}

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

bool CompilerOpenFPGA::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  Compiler::RegisterCommands(interp, batchMode);
  auto select_architecture_file = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify an architecture file");
      return TCL_ERROR;
    }
    std::ifstream stream(argv[1]);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find architecture file: " +
                             std::string(argv[1]));
      return TCL_ERROR;
    }
    stream.close();
    compiler->setArchitectureFile(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("architecture", select_architecture_file, this, 0);
  return true;
}

bool CompilerOpenFPGA::IPGenerate() {
  if ((m_design == nullptr) && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " IPs are generated!"
           << std::endl;
  m_state = IPGenerated;
  return true;
}

bool CompilerOpenFPGA::Synthesize() {
  if ((m_design == nullptr) && !CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
  for (auto constraint : m_constraints->getConstraints()) {
    (*m_out) << "Constraint: " << constraint << "\n";
  }
  for (auto keep : m_constraints->GetKeeps()) {
    (*m_out) << "Keep name: " << keep << "\n";
  }
  if (m_yosysScript.empty()) {
    m_yosysScript = basicYosysScript;
  }
  std::string fileList;
  for (const auto& lang_file : m_design->FileList()) {
    fileList += lang_file.second + " ";
  }
  std::string yosysScript = m_yosysScript;
  yosysScript = replaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", "");
  yosysScript = replaceAll(yosysScript, "${LUT_SIZE}", std::to_string(6));
  yosysScript = replaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  yosysScript = replaceAll(yosysScript, "${TOP_MODULE}", m_design->TopLevel());
  yosysScript = replaceAll(yosysScript, "${OUTPUT_BLIF}",
                           std::string(m_design->Name() + "_post_synth.blif"));
  std::ofstream ofs(std::string(m_design->Name() + ".ys"));
  ofs << yosysScript;
  ofs.close();
  std::string command = m_yosysExecutablePath.string() + " -s " +
                        std::string(m_design->Name() + ".ys");
  (*m_out) << "Synthesis command: " << command << std::endl;
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
    ErrorMessage("No design specified");
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
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed state");
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
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
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
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Placed) {
    ErrorMessage("Design needs to be in placed state");
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
    ErrorMessage("No design specified");
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
    ErrorMessage("No design specified");
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
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Routed) {
    ErrorMessage("Design needs to be in routed state");
    return false;
  }
  // TODO:
  (*m_out) << "Bitstream generation for design: " << m_design->Name() << "..."
           << std::endl;
  (*m_out) << "Design " << m_design->Name() << " bitstream is generated!"
           << std::endl;
  return true;
}
