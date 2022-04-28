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

void CompilerOpenFPGA::Help(std::ostream* out) {
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
  (*out) << "   --verific        : Uses Verific parser" << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help                       : This help" << std::endl;
  (*out) << "   create_design <name>       : Creates a design with <name> name"
         << std::endl;
  (*out) << "   architecture <file>        : Uses the architecture file"
         << std::endl;
  (*out) << "   custom_synth_script <file> : Uses a custom Yosys templatized "
            "script"
         << std::endl;
  (*out) << "   set_channel_width <int>    : VPR Routing channel setting"
         << std::endl;
  (*out) << "   add_design_file <file>... <type> (-VHDL_1987, -VHDL_1993, "
            "-VHDL_2000, "
            "-VHDL_2008, -V_1995, "
            "-V_2001, -SV_2005, -SV_2009, -SV_2012, -SV_2017) "
         << std::endl;
  (*out) << "   add_include_path <path1>...: As in +incdir+" << std::endl;
  (*out) << "   add_library_path <path1>...: As in +libdir+" << std::endl;
  (*out) << "   set_macro <name>=<value>...: As in -D<macro>=<value>"
         << std::endl;
  (*out) << "   set_top_module <top>       : Sets the top module" << std::endl;
  (*out) << "   add_constraint_file <file> : Sets SDC + location constraints"
         << std::endl;
  (*out) << "     Constraints: set_pin_loc, set_region_loc, all SDC commands"
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
# Read source files
${READ_DESIGN_FILES}

# Technology mapping
hierarchy -top ${TOP_MODULE}
proc
${KEEP_NAMES}
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
    std::string expandedFile = argv[1];
    if (!compiler->GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(argv[1]);
      expandedFile = fullPath.string();
    }
    std::ifstream stream(expandedFile);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find architecture file: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    stream.close();
    compiler->ArchitectureFile(expandedFile);
    return TCL_OK;
  };
  interp->registerCmd("architecture", select_architecture_file, this, 0);

  auto custom_synth_script = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a Yosys script");
      return TCL_ERROR;
    }
    std::string expandedFile = argv[1];
    if (!compiler->GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(argv[1]);
      expandedFile = fullPath.string();
    }
    std::ifstream stream(expandedFile);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find Yosys script: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    std::string script((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
    stream.close();
    compiler->YosysScript(script);
    return TCL_OK;
  };
  interp->registerCmd("custom_synth_script", custom_synth_script, this, 0);

  auto set_channel_width = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a channel width");
      return TCL_ERROR;
    }
    compiler->ChannelWidth(std::strtoul(argv[1], 0, 10));
    return TCL_OK;
  };
  interp->registerCmd("set_channel_width", set_channel_width, this, 0);

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

  std::ofstream ofssdc(std::string(m_design->Name() + "_openfpga.sdc"));
  // TODO: Massage the SDC so VPR can understand them
  for (auto constraint : m_constraints->getConstraints()) {
    (*m_out) << "Constraint: " << constraint << "\n";
    // Parse RTL and expand the get_ports, get_nets
    // Temporary dirty filtering:
    std::vector<std::string> tokens;
    Tokenize(constraint, " ", tokens);
    constraint = "";
    // VPR Does not understand: -name <logical_name>
    for (uint32_t i = 0; i < tokens.size(); i++) {
      const std::string& tok = tokens[i];
      if (tok == "-name") {
        // skip
        i++;
      } else {
        constraint += tok + " ";
      }
    }
    // VPR Does not understand collections commands:
    constraint = ReplaceAll(constraint, "[get_ports", "");
    constraint = ReplaceAll(constraint, "[get_nets", "");
    constraint = ReplaceAll(constraint, "]", "");

    // pin location constraints have to be translated to .place:
    if (constraint.find("set_pin_loc") != std::string::npos) {
      continue;
    }

    ofssdc << constraint << "\n";
  }
  ofssdc.close();

  // Keeps for Synthesis, preserve nodes used in constraints
  std::string keeps;
  if (m_keepAllSignals) {
    keeps += "setattr -set keep 1 w:\\*\n";
  }
  for (auto keep : m_constraints->GetKeeps()) {
    (*m_out) << "Keep name: " << keep << "\n";
    keeps += "setattr -set keep 1 " + keep + "\n";
  }

  // Default or custom Yosys script
  if (m_yosysScript.empty()) {
    m_yosysScript = basicYosysScript;
  }
  std::string yosysScript = m_yosysScript;

  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    std::string includes;
    for (auto path : m_design->IncludePathList()) {
      includes += path + " ";
    }
    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : m_design->LibraryPathList()) {
      libraries += path + " ";
    }
    fileList += "verific -vlog-libdir " + libraries + "\n";

    std::string macros;
    for (auto& macro_value : m_design->MacroList()) {
      macros += macro_value.first + "=" + macro_value.second + " ";
    }
    fileList += "verific -vlog-define " + macros + "\n";

    for (const auto& lang_file : m_design->FileList()) {
      std::string lang;
      switch (lang_file.first) {
        case Design::Language::VHDL_1987:
          lang = "-vhdl87";
          break;
        case Design::Language::VHDL_1993:
          lang = "-vhdl93";
          break;
        case Design::Language::VHDL_2000:
          lang = "-vhdl2k";
          break;
        case Design::Language::VHDL_2008:
          lang = "-vhdl2008";
          break;
        case Design::Language::VERILOG_1995:
          lang = "-vlog95";
          break;
        case Design::Language::VERILOG_2001:
          lang = "-vlog2k";
          break;
        case Design::Language::SYSTEMVERILOG_2005:
          lang = "-sv2005";
          break;
        case Design::Language::SYSTEMVERILOG_2009:
          lang = "-sv2009";
          break;
        case Design::Language::SYSTEMVERILOG_2012:
          lang = "-sv2012";
          break;
        case Design::Language::SYSTEMVERILOG_2017:
          lang = "-sv";
          break;
      }
      fileList += "verific " + lang + " " + lang_file.second + "\n";
    }
    fileList += "verific -import " + m_design->TopLevel() + "\n";
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
  } else {
    // Default Yosys parser
    std::string macros = "verilog_defines ";
    for (auto& macro_value : m_design->MacroList()) {
      macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    }
    macros += "\n";
    std::string includes;
    for (auto path : m_design->IncludePathList()) {
      includes += "-I" + path + " ";
    }
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}",
                             macros +
                                 "read_verilog ${READ_VERILOG_OPTIONS} "
                                 "${INCLUDE_PATHS} ${VERILOG_FILES}");
    std::string fileList;
    std::string lang;
    for (const auto& lang_file : m_design->FileList()) {
      fileList += lang_file.second + " ";
      switch (lang_file.first) {
        case Design::Language::VHDL_1987:
        case Design::Language::VHDL_1993:
        case Design::Language::VHDL_2000:
        case Design::Language::VHDL_2008:
          ErrorMessage("Unsupported language (Yosys default parser)!");
          break;
        case Design::Language::VERILOG_1995:
        case Design::Language::VERILOG_2001:
        case Design::Language::SYSTEMVERILOG_2005:
          break;
        case Design::Language::SYSTEMVERILOG_2009:
        case Design::Language::SYSTEMVERILOG_2012:
        case Design::Language::SYSTEMVERILOG_2017:
          lang = "-sv";
          break;
      }
    }
    yosysScript = ReplaceAll(yosysScript, "${INCLUDE_PATHS}", includes);
    std::string options = lang;

    yosysScript = ReplaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", options);
    yosysScript = ReplaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  }

  yosysScript = ReplaceAll(yosysScript, "${KEEP_NAMES}", keeps);
  yosysScript = ReplaceAll(yosysScript, "${LUT_SIZE}", std::to_string(6));
  yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}", m_design->TopLevel());
  yosysScript = ReplaceAll(yosysScript, "${OUTPUT_BLIF}",
                           std::string(m_design->Name() + "_post_synth.blif"));

  // Create Yosys command and execute
  std::ofstream ofs(std::string(m_design->Name() + ".ys"));
  ofs << yosysScript;
  ofs.close();
  if (!FileExists(m_yosysExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
    return false;
  }
  std::string command = m_yosysExecutablePath.string() + " -s " +
                        std::string(m_design->Name() + ".ys");
  (*m_out) << "Synthesis command: " << command << std::endl;
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " synthesis failed!");
    return false;
  } else {
    m_state = State::Synthesized;
    (*m_out) << "Design " << m_design->Name() << " is synthesized!"
             << std::endl;
    return true;
  }
}

std::string CompilerOpenFPGA::getBaseVprCommand() {
  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(m_design->Name() + "_post_synth.blif" +
                  std::string(" --sdc_file ") +
                  std::string(m_design->Name() + "_openfpga.sdc") +
                  std::string(" --route_chan_width ") +
                  std::to_string(m_channel_width));
  return command;
}

bool CompilerOpenFPGA::Packing() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  std::string command = getBaseVprCommand() + " --pack";
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " packing failed!");
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
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  std::string command = getBaseVprCommand() + " --place";
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " placement failed!");
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
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  std::string command = getBaseVprCommand() + " --route";
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " routing failed!");
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
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  std::string command = getBaseVprCommand() + " --analysis";
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " timing analysis failed!");
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
  if (!FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_design->Name() + " power analysis failed!");
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
