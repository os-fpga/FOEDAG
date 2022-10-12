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
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/CompilerOpenFPGA.h"
#include "Compiler/Constraints.h"
#include "Log.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;

auto copyLog = [](FOEDAG::ProjectManager* projManager,
                  const std::string& srcFileName,
                  const std::string& destFileName) -> bool {
  bool result = false;
  if (projManager) {
    std::filesystem::path projectPath(projManager->projectPath());
    std::filesystem::path src = projectPath / srcFileName;
    std::filesystem::path dest = projectPath / destFileName;
    if (FileUtils::FileExists(src)) {
      std::filesystem::remove(dest);
      std::filesystem::copy_file(src, dest);
      result = true;
    }
  }
  return result;
};

void CompilerOpenFPGA::Version(std::ostream* out) {
  (*out) << "Foedag OpenFPGA Compiler"
         << "\n";
  PrintVersion(out);
}

void CompilerOpenFPGA::Help(std::ostream* out) {
  (*out) << "----------------------------------" << std::endl;
  (*out) << "-----  FOEDAG OpenFPGA HELP  -----" << std::endl;
  (*out) << "----------------------------------" << std::endl;
  (*out) << "Options:" << std::endl;
  (*out) << "   --help           : This help" << std::endl;
  (*out) << "   --version        : Version" << std::endl;
  (*out) << "   --batch          : Tcl only, no GUI" << std::endl;
  (*out) << "   --replay <script>: Replay GUI test" << std::endl;
  (*out) << "   --script <script>: Execute a Tcl script" << std::endl;
  (*out) << "   --compiler <name>: Compiler name {openfpga...}, default is "
            "a dummy compiler"
         << std::endl;
  (*out) << "   --mute           : Mutes stdout in batch mode" << std::endl;
  (*out) << "   --verific        : Uses Verific parser" << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help                       : This help" << std::endl;
  (*out) << "   create_design <name>       : Creates a design with <name> name"
         << std::endl;
  (*out) << "   target_device <name>       : Targets a device with <name> name"
         << std::endl;
  (*out) << "   architecture <vpr_file.xml> ?<openfpga_file.xml>? :"
         << std::endl;
  (*out) << "                                Uses the architecture file and "
            "optional openfpga arch file (For bitstream generation)"
         << std::endl;
  (*out) << "   bitstream_config_files -bitstream <bitstream_setting.xml> "
            "-sim <sim_setting.xml> -repack <repack_setting.xml>"
         << std::endl;
  (*out) << "                              : Uses alternate bitstream "
            "generation configuration files"
         << std::endl;
  (*out) << "   set_device_size XxY        : Device fabric size selection"
         << std::endl;
  (*out) << "   custom_synth_script <file> : Uses a custom Yosys templatized "
            "script"
         << std::endl;
  (*out) << "   custom_openfpga_script <file> : Uses a custom OpenFPGA "
            "templatized "
            "script"
         << std::endl;
  (*out) << "   set_channel_width <int>    : VPR Routing channel setting"
         << std::endl;
  (*out) << "   add_design_file <file list> ?type?   ?-work <libName>?"
         << std::endl;
  (*out) << "              Each invocation of the command compiles the "
            "file list into a compilation unit "
         << std::endl;
  (*out) << "                       <type> : -VHDL_1987, -VHDL_1993, "
            "-VHDL_2000, -VHDL_2008, -V_1995, "
            "-V_2001, -SV_2005, -SV_2009, -SV_2012, -SV_2017> "
         << std::endl;
  (*out) << "              -work <libName> : Compiles the compilation unit "
            "into library <libName>, default is \"work\""
         << std::endl;
  (*out) << "   read_netlist <file>        : Read a netlist instead of an RTL "
            "design (Skip Synthesis)"
         << std::endl;
  (*out) << "   add_include_path <path1>...: As in +incdir+" << std::endl;
  (*out) << "   add_library_path <path1>...: As in +libdir+" << std::endl;
  (*out) << "   add_library_ext <.v> <.sv> ...: As in +libext+" << std::endl;
  (*out) << "   set_macro <name>=<value>...: As in -D<macro>=<value>"
         << std::endl;
  (*out) << "   set_top_module <top> ?-work <libName>? : Sets the top module"
         << std::endl;
  (*out) << "   add_constraint_file <file> : Sets SDC + location constraints"
         << std::endl;
  (*out) << "                                Constraints: set_pin_loc, "
            "set_mode, set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   keep <signal list> OR all_signals : Keeps the list of signals "
            "or all signals through Synthesis unchanged (unoptimized in "
            "certain cases)"
         << std::endl;
  (*out) << "   add_litex_ip_catalog <directory> : Browses directory for LiteX "
            "IP generators, adds the IP(s) to the IP Catalog"
         << std::endl;
  (*out) << "   ip_catalog ?<ip_name>?     : Lists all available IPs, and "
            "their parameters if <ip_name> is given "
         << std::endl;
  (*out) << "   ip_configure <IP_NAME> -mod_name <name> -out_file <filename> "
            "-version <ver_name> -P<param>=\"<value>\"..."
         << std::endl;
  (*out) << "                              : Configures an IP <IP_NAME> and "
            "generates the corresponding file with module name"
         << std::endl;
  (*out) << "   ipgenerate ?clean?         : Generates all IP instances set by "
            "ip_configure"
         << std::endl;
  (*out) << "   verific_parser <on/off>    : Turns on/off Verific parser"
         << std::endl;
  (*out) << "   message_severity <message_id> <ERROR/WARNING/INFO/IGNORE> : "
            "Upgrade/downgrade RTL compilation message severity"
         << std::endl;
  (*out) << "   synthesis_type Yosys/QL/RS : Selects Synthesis type"
         << std::endl;
  (*out) << "   analyze ?clean?            : Analyzes the RTL design, "
            "generates top-level, pin and hierarchy information"
         << std::endl;
  (*out)
      << "   synthesize <optimization> ?clean? : Optional optimization (area, "
         "delay, mixed, none)"
      << std::endl;
  (*out) << "   pin_loc_assign_method <Method>: "
            "(in_define_order(Default)/random/free)"
         << std::endl;
  (*out) << "   synth_options <option list>: Yosys Options" << std::endl;
  (*out) << "   pnr_options <option list>  : VPR Options" << std::endl;
  (*out) << "   pnr_netlist_lang <blif, verilog> : Chooses vpr input netlist "
            "format"
         << std::endl;
  (*out) << "   packing ?clean?            : Packing" << std::endl;
  (*out) << "   global_placement ?clean?   : Analytical placer" << std::endl;
  (*out) << "   place ?clean?              : Detailed placer" << std::endl;
  (*out) << "   route ?clean?              : Router" << std::endl;
  (*out) << "   sta ?clean?                : Statistical Timing Analysis"
         << std::endl;
  (*out) << "   power ?clean?              : Power estimator" << std::endl;
  (*out) << "   bitstream ?clean?          : Bitstream generation" << std::endl;
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
write_verilog -noexpr -nodec -defparam -norename ${OUTPUT_VERILOG}
write_edif ${OUTPUT_EDIF}
  )";

bool CompilerOpenFPGA::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  Compiler::RegisterCommands(interp, batchMode);
  auto select_architecture_file = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify an architecture file");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (FileUtils::FileExists(expandedFile)) {
        use_orig_path = true;
      }

      if ((!use_orig_path) &&
          (!compiler->GetSession()->CmdLine()->Script().empty())) {
        std::filesystem::path script =
            compiler->GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        std::filesystem::path fullPath = scriptPath;
        fullPath.append(argv[i]);
        expandedFile = fullPath.string();
      }

      std::ifstream stream(expandedFile);
      if (!stream.good()) {
        compiler->ErrorMessage("Cannot find architecture file: " +
                               std::string(expandedFile));
        return TCL_ERROR;
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        const auto& path = std::filesystem::current_path();
        expandedFile = std::filesystem::path(path / expandedFile).string();
      }
      stream.close();
      if (i == 1) {
        compiler->ArchitectureFile(expandedFile);
        compiler->Message("VPR Architecture file: " + expandedFile);
      } else {
        compiler->OpenFpgaArchitectureFile(expandedFile);
        compiler->Message("OpenFPGA Architecture file: " + expandedFile);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("architecture", select_architecture_file, this, 0);

  auto set_bitstream_config_files = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify a bitstream config file");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      std::string fileType;
      if (arg == "-bitstream") {
        fileType = "bitstream";
      } else if (arg == "-sim") {
        fileType = "sim";
      } else if (arg == "-repack") {
        fileType = "repack";
      } else {
        compiler->ErrorMessage(
            "Not a legal option for bitstream_config_files: " + arg);
        return TCL_ERROR;
      }
      i++;
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (FileUtils::FileExists(expandedFile)) {
        use_orig_path = true;
      }

      if ((!use_orig_path) &&
          (!compiler->GetSession()->CmdLine()->Script().empty())) {
        std::filesystem::path script =
            compiler->GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        std::filesystem::path fullPath = scriptPath;
        fullPath.append(argv[i]);
        expandedFile = fullPath.string();
      }

      std::ifstream stream(expandedFile);
      if (!stream.good()) {
        compiler->ErrorMessage("Cannot find bitstream config file: " +
                               std::string(expandedFile));
        return TCL_ERROR;
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
      }
      stream.close();
      if (fileType == "bitstream") {
        compiler->OpenFpgaBitstreamSettingFile(expandedFile);
        compiler->Message("OpenFPGA Bitstream Setting file: " + expandedFile);
      } else if (fileType == "sim") {
        compiler->OpenFpgaSimSettingFile(expandedFile);
        compiler->Message("OpenFPGA Simulation Setting file: " + expandedFile);
      } else if (fileType == "repack") {
        compiler->OpenFpgaRepackConstraintsFile(expandedFile);
        compiler->Message("OpenFPGA Repack Constraint file: " + expandedFile);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("bitstream_config_files", set_bitstream_config_files,
                      this, 0);

  auto custom_openfpga_script = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify an OpenFPGA script");
      return TCL_ERROR;
    }

    std::string expandedFile = argv[1];
    bool use_orig_path = false;
    if (FileUtils::FileExists(expandedFile)) {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(argv[1]);
      expandedFile = fullPath.string();
    }
    std::ifstream stream(expandedFile);
    if (!stream.good()) {
      compiler->ErrorMessage("Cannot find OpenFPGA script: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    std::string script((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
    stream.close();
    compiler->OpenFPGAScript(script);
    return TCL_OK;
  };
  interp->registerCmd("custom_openfpga_script", custom_openfpga_script, this,
                      0);

  auto custom_synth_script = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a Yosys script");
      return TCL_ERROR;
    }

    std::string expandedFile = argv[1];
    bool use_orig_path = false;
    if (FileUtils::FileExists(expandedFile)) {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
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

  auto message_severity = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string message;
    std::string severity;
    if (argc < 3) {
      compiler->ErrorMessage(
          "message_severity <message_id> <ERROR/WARNING/INFO/IGNORE>");
    }
    message = argv[1];
    severity = argv[2];
    MsgSeverity sev = MsgSeverity::Ignore;
    if (severity == "INFO") {
      sev = MsgSeverity::Info;
    } else if (severity == "WARNING") {
      sev = MsgSeverity::Warning;
    } else if (severity == "ERROR") {
      sev = MsgSeverity::Error;
    } else if (severity == "IGNORE") {
      sev = MsgSeverity::Ignore;
    }

    compiler->AddMsgSeverity(message, sev);
    return TCL_OK;
  };
  interp->registerCmd("message_severity", message_severity, this, 0);

  auto keep = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    for (int i = 1; i < argc; i++) {
      name = argv[i];
      if (name == "all_signals") {
        compiler->KeepAllSignals(true);
      } else {
        compiler->getConstraints()->addKeep(name);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("keep", keep, this, 0);

  auto set_device_size = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a device size: xXy");
      return TCL_ERROR;
    }

    const std::string deviceSize{argv[1]};

    if (const auto& [res, errorMsg] = compiler->IsDeviceSizeCorrect(deviceSize);
        !res) {
      compiler->ErrorMessage(errorMsg);
      return TCL_ERROR;
    }

    compiler->DeviceSize(deviceSize);
    return TCL_OK;
  };
  interp->registerCmd("set_device_size", set_device_size, this, 0);

  auto pnr_netlist_lang = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (argc != 2) {
      compiler->ErrorMessage("Specify the netlist type: verilog or blif");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (arg == "verilog") {
      compiler->UseVerilogNetlist(true);
    } else if (arg == "edif") {
      compiler->UseEdifNetlist(true);
    } else if (arg == "blif") {
      compiler->UseVerilogNetlist(false);
    } else {
      compiler->ErrorMessage(
          "Invalid arg to netlist_type (verilog or blif), was: " + arg);
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("pnr_netlist_lang", pnr_netlist_lang, this, 0);

  auto verific_parser = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify on/off");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    compiler->SetUseVerific((arg == "on") ? true : false);
    return TCL_OK;
  };
  interp->registerCmd("verific_parser", verific_parser, this, 0);

  auto target_device = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Please select a device");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (compiler->LoadDeviceData(arg)) {
      compiler->ProjManager()->setTargetDevice(arg);
    } else {
      compiler->ErrorMessage("Invalid target device: " + arg);
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("target_device", target_device, this, 0);
  auto synthesis_type = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify type: Yosys/RS/QL");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (arg == "Yosys") {
      compiler->SynthType(SynthesisType::Yosys);
    } else if (arg == "RS") {
      compiler->SynthType(SynthesisType::RS);
    } else if (arg == "QL") {
      compiler->SynthType(SynthesisType::QL);
    } else {
      compiler->ErrorMessage("Illegal synthesis type: " + arg);
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("synthesis_type", synthesis_type, this, 0);
  return true;
}

std::pair<bool, std::string> CompilerOpenFPGA::IsDeviceSizeCorrect(
    const std::string& size) const {
  if (m_architectureFile.empty())
    return std::make_pair(false,
                          "Please specify target device or architecture file.");
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
  std::filesystem::path devicefile = datapath / "etc" / m_architectureFile;
  QFile file(devicefile.string().c_str());
  if (!file.open(QFile::ReadOnly)) {
    return std::make_pair(false,
                          "Cannot open device file: " + devicefile.string());
  }
  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return std::make_pair(false,
                          "Incorrect device file: " + devicefile.string());
  }
  file.close();
  auto fixedLayout = doc.elementsByTagName("fixed_layout");
  if (fixedLayout.isEmpty())
    return std::make_pair(false, "Architecture file: fixed_layout is missing");
  for (int i = 0; i < fixedLayout.count(); i++) {
    auto node = fixedLayout.at(i).toElement();
    if (node.attribute("name").toStdString() == size)
      return std::make_pair(true, std::string{});
  }
  return std::make_pair(false, std::string{"Device size is not correct"});
}

bool CompilerOpenFPGA::VerifyTargetDevice() const {
  const bool target = Compiler::VerifyTargetDevice();
  const bool archFile = FileUtils::FileExists(m_architectureFile);
  return target || archFile;
}

bool CompilerOpenFPGA::IPGenerate() {
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  if (!HasTargetDevice()) return false;
  if (!HasIPInstances()) {
    // No instances configured, no-op w/o error
    return true;
  }
  PERF_LOG("IPGenerate has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "IP generation for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  bool status = GetIPGenerator()->Generate();
  if (status) {
    (*m_out) << "Design " << m_projManager->projectName()
             << " IPs are generated!" << std::endl;
    m_state = State::IPGenerated;
  } else {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " IPs generation failed!");
  }
  return true;
}

bool CompilerOpenFPGA::DesignChanged(
    const std::string& synth_script,
    const std::filesystem::path& synth_scrypt_path,
    const std::filesystem::path& outputFile) {
  bool result = false;
  auto path = std::filesystem::current_path();                  // getting path
  std::filesystem::current_path(ProjManager()->projectPath());  // setting path
  time_t time_netlist = FileUtils::Mtime(outputFile);
  if (time_netlist == -1) {
    result = true;
  }
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(lang_file.second, " ", tokens);
    for (auto file : tokens) {
      file = StringUtils::trim(file);
      if (file.size()) {
        time_t tf = FileUtils::Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : ProjManager()->includePathList()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(AdjustPath(path), " ", tokens);
    for (auto file : tokens) {
      file = StringUtils::trim(file);
      if (file.size()) {
        time_t tf = FileUtils::Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : ProjManager()->libraryPathList()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(AdjustPath(path), " ", tokens);
    for (auto file : tokens) {
      file = StringUtils::trim(file);
      if (file.size()) {
        time_t tf = FileUtils::Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }

  std::ifstream script(synth_scrypt_path);
  if (!script.good()) {
    result = true;
  }
  std::stringstream buffer;
  buffer << script.rdbuf();
  if (synth_script != buffer.str()) {
    result = true;
  }
  std::filesystem::current_path(path);
  return result;
}

bool CompilerOpenFPGA::Analyze() {
  if (AnalyzeOpt() == DesignAnalysisOpt::Clean) {
    Message("Cleaning analysis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    AnalyzeOpt(DesignAnalysisOpt::None);
    // Remove generated json files
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) / "port_info.json");
    return true;
  }
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  if (!HasTargetDevice()) return false;

  PERF_LOG("Analysis has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Analysis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;

  // TODO: Awaiting interface from analyzer exec.
  std::string analysisScript;

  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    std::string includes;
    for (auto path : ProjManager()->includePathList()) {
      includes += AdjustPath(path) + " ";
    }
    fileList += "-vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : ProjManager()->libraryPathList()) {
      libraries += AdjustPath(path) + " ";
    }
    fileList += "-vlog-libdir " + libraries + "\n";

    for (auto ext : ProjManager()->libraryExtensionList()) {
      fileList += "-vlog-libext " + ext + "\n";
    }

    std::string macros;
    for (auto& macro_value : ProjManager()->macroList()) {
      macros += macro_value.first + "=" + macro_value.second + " ";
    }
    fileList += "-vlog-define " + macros + "\n";

    std::string importLibs;
    auto commandsLibs = ProjManager()->DesignLibraries();
    size_t filesIndex{0};
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      std::string lang;
      std::string designLibraries;
      switch (lang_file.first.language) {
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
        case Design::Language::VERILOG_NETLIST:
          lang = "";
          break;
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          lang = "BLIF";
          ErrorMessage("Unsupported file format:" + lang);
          return false;
      }
      if (filesIndex < commandsLibs.size()) {
        const auto& filesCommandsLibs = commandsLibs[filesIndex];
        for (size_t i = 0; i < filesCommandsLibs.first.size(); ++i) {
          auto libName = filesCommandsLibs.second[i];
          if (!libName.empty()) {
            auto commandLib = "-work " + libName + " ";
            designLibraries += commandLib;
          }
        }
      }
      ++filesIndex;

      if (designLibraries.empty())
        fileList += lang + " " + lang_file.second + "\n";
      else
        fileList +=
            designLibraries + " " + lang + " " + lang_file.second + "\n";
    }
    if (!ProjManager()->DesignTopModule().empty()) {
      fileList += "-top " + ProjManager()->DesignTopModule() + "\n";
    }
    analysisScript = fileList;
  } else {
    // TODO: develop an analysis step with only Yosys parser (no synthesis)
    // Default Yosys parser
    /*
       std::string macros = "verilog_defines ";
       for (auto& macro_value : ProjManager()->macroList()) {
       macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
       }
       macros += "\n";
       std::string includes;
       for (auto path : ProjManager()->includePathList()) {
       includes += "-I" + path + " ";
       }
       analysisScript = ReplaceAll(analysisScript, "${READ_DESIGN_FILES}",
       macros +
       "read_verilog ${READ_VERILOG_OPTIONS} "
       "${INCLUDE_PATHS} ${VERILOG_FILES}");
       std::string fileList;
       std::string lang;
       for (const auto& lang_file : ProjManager()->DesignFiles()) {
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
       case Design::Language::VERILOG_NETLIST:
       case Design::Language::BLIF:
       case Design::Language::EBLIF:
       ErrorMessage("Unsupported language (Yosys default parser)!");
       break;
       }
       analysisScript = fileList;
       */
  }

  std::string script_path = ProjManager()->projectName() + "_analyzer.cmd";
  script_path =
      (std::filesystem::path(ProjManager()->projectPath()) / script_path)
          .string();
  std::filesystem::path output_path =
      std::filesystem::path(ProjManager()->projectPath()) / "port_info.json";
  if (!DesignChanged(analysisScript, script_path, output_path)) {
    (*m_out) << "Design didn't change: " << ProjManager()->projectName()
             << ", skipping analysis." << std::endl;
    return true;
  }
  // Create Analyser command and execute
  std::ofstream ofs(script_path);
  ofs << analysisScript;
  ofs.close();
  std::string command;
  int status = 0;
  if (m_useVerific) {
    if (!FileUtils::FileExists(m_analyzeExecutablePath)) {
      ErrorMessage("Cannot find executable: " +
                   m_analyzeExecutablePath.string());
      return false;
    }
    command = m_analyzeExecutablePath.string() + " -f " + script_path;
    (*m_out) << "Analyze command: " << command << std::endl;
    status = ExecuteAndMonitorSystemCommand(command);
  }
  // TODO: read back the Json file produced
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " analysis failed!");
    return false;
  } else {
    m_state = State::Analyzed;
    (*m_out) << "Design " << ProjManager()->projectName() << " is analyzed!"
             << std::endl;
  }
  return true;
}

bool CompilerOpenFPGA::Synthesize() {
  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    SynthOpt(SynthesisOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.blif"));
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.v"));
    return true;
  }
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  if (!HasTargetDevice()) return false;

  PERF_LOG("Synthesize has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Synthesis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  std::string yosysScript = InitSynthesisScript();

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF:
        Message("Skipping synthesis, gate-level design.");
        return true;
        break;
      default:
        break;
    }
  }

  // update constraints
  const auto& constrFiles = ProjManager()->getConstrFiles();
  for (const auto& file : constrFiles) {
    int res{TCL_OK};
    auto status =
        m_interp->evalCmd(std::string("read_sdc {" + file + "}").c_str(), &res);
    if (res != TCL_OK) {
      ErrorMessage(status);
      return false;
    }
  }

  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    std::string includes;

    for (auto msg_sev : MsgSeverityMap()) {
      switch (msg_sev.second) {
        case MsgSeverity::Ignore:
          fileList += "verific -set-ignore " + msg_sev.first + "\n";
          break;
        case MsgSeverity::Info:
          fileList += "verific -set-info " + msg_sev.first + "\n";
          break;
        case MsgSeverity::Warning:
          fileList += "verific -set-warning " + msg_sev.first + "\n";
          break;
        case MsgSeverity::Error:
          fileList += "verific -set-error " + msg_sev.first + "\n";
          break;
      }
    }

    for (auto path : ProjManager()->includePathList()) {
      includes += AdjustPath(path) + " ";
    }
    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : ProjManager()->libraryPathList()) {
      libraries += AdjustPath(path) + " ";
    }
    fileList += "verific -vlog-libdir " + libraries + "\n";

    for (auto ext : ProjManager()->libraryExtensionList()) {
      fileList += "verific -vlog-libext " + ext + "\n";
    }

    std::string macros;
    for (auto& macro_value : ProjManager()->macroList()) {
      macros += macro_value.first + "=" + macro_value.second + " ";
    }
    fileList += "verific -vlog-define " + macros + "\n";

    std::string importLibs;
    auto importDesignFilesLibs = false;

    auto topModuleLib = ProjManager()->DesignTopModuleLib();
    auto commandsLibs = ProjManager()->DesignLibraries();
    size_t filesIndex{0};
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      std::string lang;
      std::string designLibraries;
      switch (lang_file.first.language) {
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
          importDesignFilesLibs = true;
          break;
        case Design::Language::SYSTEMVERILOG_2005:
          lang = "-sv2005";
          importDesignFilesLibs = true;
          break;
        case Design::Language::SYSTEMVERILOG_2009:
          lang = "-sv2009";
          importDesignFilesLibs = true;
          break;
        case Design::Language::SYSTEMVERILOG_2012:
          lang = "-sv2012";
          importDesignFilesLibs = true;
          break;
        case Design::Language::SYSTEMVERILOG_2017:
          lang = "-sv";
          importDesignFilesLibs = true;
          break;
        case Design::Language::VERILOG_NETLIST:
          lang = "";
          break;
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          lang = "BLIF";
          ErrorMessage("Unsupported file format:" + lang);
          return false;
      }
      if (filesIndex < commandsLibs.size()) {
        const auto& filesCommandsLibs = commandsLibs[filesIndex];
        for (size_t i = 0; i < filesCommandsLibs.first.size(); ++i) {
          auto libName = filesCommandsLibs.second[i];
          if (!libName.empty()) {
            auto commandLib = "-work " + libName + " ";
            designLibraries += commandLib;
            if (importDesignFilesLibs && libName != topModuleLib)
              importLibs += "-L " + libName + " ";
          }
        }
      }
      ++filesIndex;

      if (designLibraries.empty())
        fileList += "verific " + lang + " " + lang_file.second + "\n";
      else
        fileList +=
            "verific " + designLibraries + lang + " " + lang_file.second + "\n";
    }
    auto topModuleLibImport = std::string{};
    if (!topModuleLib.empty())
      topModuleLibImport = "-work " + topModuleLib + " ";
    fileList += "verific " + topModuleLibImport + importLibs + "-import " +
                ProjManager()->DesignTopModule() + "\n";
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
  } else {
    // Default Yosys parser

    for (const auto& commandLib : ProjManager()->DesignLibraries()) {
      if (!commandLib.first.empty()) {
        ErrorMessage(
            "Yosys default parser doesn't support '-work' design file "
            "command!");
        break;
      }
    }
    std::string macros = "verilog_defines ";
    for (auto& macro_value : ProjManager()->macroList()) {
      macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    }
    macros += "\n";
    std::string includes;
    for (auto path : ProjManager()->includePathList()) {
      includes += "-I" + AdjustPath(path) + " ";
    }

    std::string designFiles;
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      std::string filesScript =
          "read_verilog ${READ_VERILOG_OPTIONS} ${INCLUDE_PATHS} "
          "${VERILOG_FILES}";
      std::string lang;

      auto files = lang_file.second + " ";
      switch (lang_file.first.language) {
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
        case Design::Language::VERILOG_NETLIST:
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          ErrorMessage("Unsupported language (Yosys default parser)!");
          break;
      }
      filesScript = ReplaceAll(filesScript, "${READ_VERILOG_OPTIONS}", lang);
      filesScript = ReplaceAll(filesScript, "${INCLUDE_PATHS}", includes);
      filesScript = ReplaceAll(filesScript, "${VERILOG_FILES}", files);

      designFiles += filesScript + "\n";
    }
    yosysScript =
        ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", macros + designFiles);
  }

  yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}",
                           ProjManager()->DesignTopModule());
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_BLIF}",
      std::string(ProjManager()->projectName() + "_post_synth.blif"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_VERILOG}",
                 std::string(ProjManager()->projectName() + "_post_synth.v"));
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_EDIF}",
      std::string(ProjManager()->projectName() + "_post_synth.edif"));

  yosysScript = FinishSynthesisScript(yosysScript);

  std::string script_path = ProjManager()->projectName() + ".ys";
  std::string output_path;
  if (UseVerilogNetlist()) {
    output_path = ProjManager()->projectName() + "_post_synth.v";
  } else if (UseEdifNetlist()) {
    output_path = ProjManager()->projectName() + "_post_synth.edif";
  } else {
    output_path = ProjManager()->projectName() + "_post_synth.blif";
  }

  if (!DesignChanged(yosysScript, script_path, output_path)) {
    (*m_out) << "Design didn't change: " << ProjManager()->projectName()
             << ", skipping synthesis." << std::endl;
    return true;
  }
  std::filesystem::remove(
      std::filesystem::path(ProjManager()->projectPath()) /
      std::string(ProjManager()->projectName() + "_post_synth.blif"));
  std::filesystem::remove(
      std::filesystem::path(ProjManager()->projectPath()) /
      std::string(ProjManager()->projectName() + "_post_synth.v"));
  // Create Yosys command and execute
  script_path =
      (std::filesystem::path(ProjManager()->projectPath()) / script_path)
          .string();
  std::ofstream ofs(script_path);
  ofs << yosysScript;
  ofs.close();
  if (!FileUtils::FileExists(m_yosysExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
    return false;
  }
  std::string command =
      m_yosysExecutablePath.string() + " -s " +
      std::string(ProjManager()->projectName() + ".ys -l " +
                  ProjManager()->projectName() + "_synth.log");
  (*m_out) << "Synthesis command: " << command << std::endl;
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " synthesis failed!");
    return false;
  } else {
    m_state = State::Synthesized;
    (*m_out) << "Design " << ProjManager()->projectName() << " is synthesized!"
             << std::endl;

    copyLog(ProjManager(), "synth.log", "synthesis.rpt");
    return true;
  }
}

std::string CompilerOpenFPGA::InitSynthesisScript() {
  // Default or custom Yosys script
  if (m_yosysScript.empty()) {
    m_yosysScript = basicYosysScript;
  }
  return m_yosysScript;
}

std::string CompilerOpenFPGA::FinishSynthesisScript(const std::string& script) {
  std::string result = script;
  // Keeps for Synthesis, preserve nodes used in constraints
  std::string keeps;
  if (m_keepAllSignals) {
    keeps += "setattr -set keep 1 w:\\*\n";
  }
  for (auto keep : m_constraints->GetKeeps()) {
    keep = ReplaceAll(keep, "@", "[");
    keep = ReplaceAll(keep, "%", "]");
    (*m_out) << "Keep name: " << keep << "\n";
    keeps += "setattr -set keep 1 w:\\" + keep + "\n";
  }
  result = ReplaceAll(result, "${KEEP_NAMES}", keeps);
  result = ReplaceAll(result, "${OPTIMIZATION}", SynthMoreOpt());
  result = ReplaceAll(result, "${PLUGIN_LIB}", YosysPluginLibName());
  result = ReplaceAll(result, "${PLUGIN_NAME}", YosysPluginName());
  result = ReplaceAll(result, "${MAP_TO_TECHNOLOGY}", YosysMapTechnology());
  result = ReplaceAll(result, "${LUT_SIZE}", std::to_string(m_lut_size));
  return result;
}

std::string CompilerOpenFPGA::BaseVprCommand() {
  std::string device_size = "";
  if (!m_deviceSize.empty()) {
    device_size = " --device " + m_deviceSize;
  }
  std::string netlistFile;
  if (UseVerilogNetlist()) {
    netlistFile = ProjManager()->projectName() + "_post_synth.v";
  } else if (UseEdifNetlist()) {
    netlistFile = ProjManager()->projectName() + "_post_synth.edif";
  } else {
    netlistFile = ProjManager()->projectName() + "_post_synth.blif";
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

  std::string pnrOptions;
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile + std::string(" --sdc_file ") +
                  std::string(ProjManager()->projectName() + "_openfpga.sdc") +
                  std::string(" --route_chan_width ") +
                  std::to_string(m_channel_width) + device_size + pnrOptions);

  return command;
}

std::string CompilerOpenFPGA::BaseStaCommand() {
  std::string command =
      m_staExecutablePath.string() +
      std::string(
          " -exit ");  // allow open sta exit its tcl shell even there is error
  return command;
}

std::string CompilerOpenFPGA::BaseStaScript(std::string libFileName,
                                            std::string netlistFileName,
                                            std::string sdfFileName,
                                            std::string sdcFileName) {
  std::string script =
      std::string("read_liberty ") + libFileName +
      std::string("\n") +  // add lib for test only, need to research on this
      std::string("read_verilog ") + netlistFileName + std::string("\n") +
      std::string("link_design ") + ProjManager()->projectName() +
      std::string("\n") + std::string("read_sdf ") + sdfFileName +
      std::string("\n") + std::string("read_sdc ") + sdcFileName +
      std::string("\n") +
      std::string("report_checks\n");  // to do: add more check/report flavors
  const std::string openStaFile =
      (std::filesystem::path(ProjManager()->projectPath()) /
       std::string(ProjManager()->projectName() + "_opensta.tcl"))
          .string();
  std::ofstream ofssta(openStaFile);
  ofssta << script << "\n";
  ofssta.close();
  return openStaFile;
}

bool CompilerOpenFPGA::Packing() {
  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.net"));
    return true;
  }
  if (!HasTargetDevice()) return false;
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  PERF_LOG("Packing has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Packing for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  const std::string sdcOut =
      (std::filesystem::path(ProjManager()->projectPath()) /
       std::string(ProjManager()->projectName() + "_openfpga.sdc"))
          .string();
  std::ofstream ofssdc(sdcOut);
  // TODO: Massage the SDC so VPR can understand them
  for (auto constraint : m_constraints->getConstraints()) {
    // Parse RTL and expand the get_ports, get_nets
    // Temporary dirty filtering:
    constraint = ReplaceAll(constraint, "@", "[");
    constraint = ReplaceAll(constraint, "%", "]");
    (*m_out) << "Constraint: " << constraint << "\n";
    std::vector<std::string> tokens;
    StringUtils::tokenize(constraint, " ", tokens);
    constraint = "";
    // VPR does not understand: create_clock -period 2 clk -name <logical_name>
    // Pass the constraint as-is anyway
    for (uint32_t i = 0; i < tokens.size(); i++) {
      const std::string& tok = tokens[i];
      constraint += tok + " ";
    }

    // pin location constraints have to be translated to .place:
    if (constraint.find("set_pin_loc") != std::string::npos) {
      continue;
    }
    if (constraint.find("set_mode") != std::string::npos) {
      continue;
    }
    ofssdc << constraint << "\n";
  }
  ofssdc.close();

  std::string command = BaseVprCommand() + " --pack";
  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_pack.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();

  if (FileUtils::IsUptoDate(
          GetNetlistPath(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.net"))
              .string())) {
    m_state = State::Packed;
    (*m_out) << "Design " << ProjManager()->projectName() << " packing reused"
             << std::endl;
    return true;
  }

  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " packing failed!");
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is packed!"
           << std::endl;

  copyLog(ProjManager(), "vpr_stdout.log", "packing.rpt");
  return true;
}

bool CompilerOpenFPGA::GlobalPlacement() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (GlobPlacementOpt() == GlobalPlacementOpt::Clean) {
    Message("Cleaning global placement results for " +
            ProjManager()->projectName());
    m_state = State::Packed;
    GlobPlacementOpt(GlobalPlacementOpt::None);
    return true;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  if (!HasTargetDevice()) return false;

  PERF_LOG("GlobalPlacement has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Global Placement for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  // TODO:
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << ProjManager()->projectName()
           << " is globally placed!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::Placement() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (PlaceOpt() == PlacementOpt::Clean) {
    Message("Cleaning placement results for " + ProjManager()->projectName());
    m_state = State::GloballyPlaced;
    PlaceOpt(PlacementOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.place"));
    return true;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;

  PERF_LOG("Placement has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Placement for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  const std::string pcfOut =
      (std::filesystem::path(ProjManager()->projectName()) /
       std::string(ProjManager()->projectName() + "_openfpga.pcf"))
          .string();

  std::string previousConstraints;
  std::ifstream ifspcf(pcfOut);
  if (ifspcf.good()) {
    std::stringstream buffer;
    buffer << ifspcf.rdbuf();
    previousConstraints = buffer.str();
  }
  ifspcf.close();

  bool userConstraint = false;
  std::vector<std::string> constraints;
  for (auto constraint : m_constraints->getConstraints()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(constraint, " ", tokens);
    constraint = "";
    constraint += tokens[0];
    // last token tokens[tokens.size() - 1]  is "" (why?)
    for (uint32_t i = 1; i < tokens.size() - 1; i++) {
      const std::string& tok = tokens[i];
      constraint += " " + tok;
    }
    constraint = ReplaceAll(constraint, "@", "[");
    constraint = ReplaceAll(constraint, "%", "]");
    // pin location constraints have to be translated to .place:
    if ((constraint.find("set_pin_loc") != std::string::npos)) {
      userConstraint = true;
      constraint = ReplaceAll(constraint, "set_pin_loc", "set_io");
      constraints.push_back(constraint);
    } else if (constraint.find("set_mode") != std::string::npos) {
      constraints.push_back(constraint);
    } else {
      continue;
    }
  }

  // sanity check and convert to pcf format
  if (!ConvertSdcPinConstrainToPcf(constraints)) {
    ErrorMessage("Error in SDC file for placement constraint");
    return false;
  }

  // write to file
  std::ofstream ofspcf(pcfOut);
  for (auto constraint : constraints) {
    ofspcf << constraint << "\n";
  }
  ofspcf.close();

  std::string newConstraints;
  ifspcf.open(pcfOut);
  if (ifspcf.good()) {
    std::stringstream buffer;
    buffer << ifspcf.rdbuf();
    newConstraints = buffer.str();
  }
  ifspcf.close();

  if ((previousConstraints == newConstraints) &&
      FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.net"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.place"))
              .string())) {
    m_state = State::Placed;
    (*m_out) << "Design " << ProjManager()->projectName() << " placement reused"
             << std::endl;
    return true;
  }

  std::string pin_loc_constraint_file;

  std::string netlistFile = ProjManager()->projectName() + "_post_synth.blif";

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

  std::string command = BaseVprCommand() + " --place";
  std::string pincommand = m_pinConvExecutablePath.string();
  if (PinConstraintEnabled() && (PinAssignOpts() != PinAssignOpt::Free) &&
      FileUtils::FileExists(pincommand) && (!m_OpenFpgaPinMapCSV.empty())) {
    if (!std::filesystem::is_regular_file(m_OpenFpgaPinMapCSV)) {
      ErrorMessage(
          "No pin description csv file available for this device, required "
          "for set_pin_loc constraints");
      return false;
    }
    // pin_c executable can work with either xml and csv or csv only file
    if (!m_OpenFpgaPinMapXml.empty() &&
        std::filesystem::is_regular_file(m_OpenFpgaPinMapXml)) {
      pincommand += " --xml " + m_OpenFpgaPinMapXml.string();
    }
    pincommand += " --csv " + m_OpenFpgaPinMapCSV.string();

    if (userConstraint) {
      pincommand += " --pcf " +
                    std::string(ProjManager()->projectName() + "_openfpga.pcf");
    }

    // TODO: accept both blif or verilog format
    pincommand += " --blif " + netlistFile;
    std::string pin_locFile = ProjManager()->projectName() + "_pin_loc.place";
    pincommand += " --output " + pin_locFile;

    // for design pins that are not explicitly constrained by user,
    // pin_c will assign legal device pins to them
    // this is configured at top level raptor shell/gui through command
    // "pin_loc_assign_method"
    pincommand += " --assign_unconstrained_pins";
    if (PinAssignOpts() == PinAssignOpt::Random) {
      pincommand += " random";
    } else if (PinAssignOpts() == PinAssignOpt::In_Define_Order) {
      pincommand += " in_define_order";
    } else {  // default behavior
      pincommand += " in_define_order";
    }

    std::ofstream ofsp(
        (std::filesystem::path(ProjManager()->projectName()) /
         std::string(ProjManager()->projectName() + "_pin_loc.cmd"))
            .string());
    ofsp << pincommand << std::endl;
    ofsp.close();

    int status = ExecuteAndMonitorSystemCommand(pincommand);

    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " pin conversion failed!");
      return false;
    } else {
      pin_loc_constraint_file = pin_locFile;
    }

    if (PinConstraintEnabled() && (!pin_loc_constraint_file.empty())) {
      command += " --fix_clusters " + pin_loc_constraint_file;
    }
  }

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_place.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " placement failed!");
    return false;
  }
  m_state = State::Placed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is placed!"
           << std::endl;

  copyLog(ProjManager(), "vpr_stdout.log", "placement.rpt");
  return true;
}

bool CompilerOpenFPGA::ConvertSdcPinConstrainToPcf(
    std::vector<std::string>& constraints) {
  // do some simple sanity check during conversion
  std::vector<std::string> constraint_and_mode;
  std::map<std::string, std::string> pin_mode_map;
  // capture pin and mode map
  for (unsigned int i = 0; i < constraints.size(); i++) {
    if (constraints[i].find("set_mode") != std::string::npos) {
      std::vector<std::string> tokens;
      StringUtils::tokenize(constraints[i], " ", tokens);
      if (tokens.size() != 3) {
        ErrorMessage("Invalid set_mode command: <" + constraints[i] + ">");
        return false;
      }
      pin_mode_map.insert(
          std::pair<std::string, std::string>(tokens[2], tokens[1]));
    }
  }
  for (unsigned int i = 0; i < constraints.size(); i++) {
    if (constraints[i].find("set_io") != std::string::npos) {
      std::vector<std::string> tokens;
      StringUtils::tokenize(constraints[i], " ", tokens);
      if (tokens.size() != 3) {
        ErrorMessage("Invalid set_io command: <" + constraints[i] + ">");
        return false;
      }
      std::string constraint_with_mode = constraints[i];
      if (pin_mode_map.find(tokens[2]) != pin_mode_map.end()) {
        constraint_with_mode +=
            std::string(" -mode ") + pin_mode_map[tokens[2]];
      } else {
        constraint_with_mode += std::string(" -mode Mode_GPIO");
      }
      constraint_and_mode.push_back(constraint_with_mode);
    }
  }
  constraints.clear();
  for (unsigned int i = 0; i < constraint_and_mode.size(); i++) {
    constraints.push_back(constraint_and_mode[i]);
  }
  return true;
}

bool CompilerOpenFPGA::Route() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (RouteOpt() == RoutingOpt::Clean) {
    Message("Cleaning routing results for " + ProjManager()->projectName());
    m_state = State::Placed;
    RouteOpt(RoutingOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.route"));
    return true;
  }
  if (m_state != State::Placed) {
    ErrorMessage("Design needs to be in placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;
  PERF_LOG("Route has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Routing for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.place"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string())) {
    m_state = State::Routed;
    (*m_out) << "Design " << ProjManager()->projectName() << " routing reused"
             << std::endl;
    return true;
  }

  std::string command = BaseVprCommand() + " --route";
  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_route.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " routing failed!");
    return false;
  }
  m_state = State::Routed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is routed!"
           << std::endl;

  copyLog(ProjManager(), "vpr_stdout.log", "routing.rpt");
  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!HasTargetDevice()) return false;

  if (TimingAnalysisOpt() == STAOpt::Clean) {
    Message("Cleaning TimingAnalysis results for " +
            ProjManager()->projectName());
    TimingAnalysisOpt(STAOpt::None);
    m_state = State::Routed;
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_sta.cmd"));
    return true;
  }

  PERF_LOG("TimingAnalysis has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Timing Analysis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  if (TimingAnalysisOpt() == STAOpt::View) {
    TimingAnalysisOpt(STAOpt::None);
    const std::string command = BaseVprCommand() + " --analysis --disp on";
    const int status = ExecuteAndMonitorSystemCommand(command);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " place and route view failed!");
      return false;
    }
    return true;
  }

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_sta.cmd"))
              .string())) {
    (*m_out) << "Design " << ProjManager()->projectName()
             << " timing didn't change" << std::endl;
    return true;
  }
  int status = 0;
  std::string taCommand;
  // use OpenSTA to do the job
  if (TimingAnalysisOpt() == STAOpt::Opensta) {
    // allows SDF to be generated for OpenSTA
    std::string command = BaseVprCommand() + " --gen_post_synthesis_netlist on";
    std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                       std::string(ProjManager()->projectName() + "_sta.cmd"))
                          .string());
    ofs.close();
    int status = ExecuteAndMonitorSystemCommand(command);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " timing analysis failed!");
      return false;
    }
    // find files
    std::string libFileName =
        (std::filesystem::current_path() /
         std::string(ProjManager()->projectName() + ".lib"))
            .string();  // this is the standard sdc file
    std::string netlistFileName =
        (std::filesystem::path(ProjManager()->projectPath()) /
         std::string(ProjManager()->projectName() + "_post_synthesis.v"))
            .string();
    std::string sdfFileName =
        (std::filesystem::path(ProjManager()->projectPath()) /
         std::string(ProjManager()->projectName() + "_post_synthesis.sdf"))
            .string();
    // std::string sdcFile = ProjManager()->getConstrFiles();
    std::string sdcFileName =
        (std::filesystem::current_path() /
         std::string(ProjManager()->projectName() + ".sdc"))
            .string();  // this is the standard sdc file
    if (std::filesystem::is_regular_file(libFileName) &&
        std::filesystem::is_regular_file(netlistFileName) &&
        std::filesystem::is_regular_file(sdfFileName) &&
        std::filesystem::is_regular_file(sdcFileName)) {
      taCommand =
          BaseStaCommand() + " " +
          BaseStaScript(libFileName, netlistFileName, sdfFileName, sdcFileName);
      std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                         std::string(ProjManager()->projectName() + "_sta.cmd"))
                            .string());
      ofs << taCommand << std::endl;
      ofs.close();
    } else {
      ErrorMessage(
          "No required design info generated for user design, required "
          "for timing analysis");
      return false;
    }
  } else {  // use vpr/tatum engine
    taCommand = BaseVprCommand() + " --analysis";
    std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                       std::string(ProjManager()->projectName() + "_sta.cmd"))
                          .string());
    ofs << taCommand << " --disp on" << std::endl;
    ofs.close();
  }

  status = ExecuteAndMonitorSystemCommand(taCommand);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " timing analysis failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName()
           << " is timing analysed!" << std::endl;

  copyLog(ProjManager(), "vpr_stdout.log", "timing_analysis.rpt");
  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!HasTargetDevice()) return false;

  if (PowerAnalysisOpt() == PowerOpt::Clean) {
    Message("Cleaning PoweAnalysis results for " +
            ProjManager()->projectName());
    PowerAnalysisOpt(PowerOpt::None);
    m_state = State::Routed;
    return true;
  }

  PERF_LOG("PowerAnalysis has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Power Analysis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_sta.cmd"))
              .string())) {
    (*m_out) << "Design " << ProjManager()->projectName()
             << " power didn't change" << std::endl;
    return true;
  }

  std::string command = BaseVprCommand() + " --analysis";
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " power analysis failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName() << " is power analysed!"
           << std::endl;

  copyLog(ProjManager(), "vpr_stdout.log", "power_analysis.rpt");
  return true;
}

const std::string basicOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT} --analysis ${PNR_OPTIONS}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation settings
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

read_openfpga_bitstream_setting -f ${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --frame_view --compress_routing --duplicate_grid_pin ${OPENFPGA_BUILD_FABRIC_OPTION}

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream

build_fabric_bitstream
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

std::string CompilerOpenFPGA::InitOpenFPGAScript() {
  // Default or custom OpenFPGA script
  if (m_openFPGAScript.empty()) {
    m_openFPGAScript = basicOpenFPGABitstreamScript;
  }
  return m_openFPGAScript;
}

std::string CompilerOpenFPGA::FinishOpenFPGAScript(const std::string& script) {
  std::string result = script;

  std::string netlistFilePrefix = ProjManager()->projectName() + "_post_synth";

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        std::filesystem::path the_path = lang_file.second;
        std::filesystem::path filename = the_path.filename();
        std::filesystem::path stem = filename.stem();
        netlistFilePrefix = stem.string();
        break;
      }
      default:
        break;
    }
  }

  result = ReplaceAll(result, "${VPR_ARCH_FILE}", m_architectureFile.string());
  result = ReplaceAll(result, "${NET_FILE}", netlistFilePrefix + ".net");
  result = ReplaceAll(result, "${PLACE_FILE}", netlistFilePrefix + ".place");
  result = ReplaceAll(result, "${ROUTE_FILE}", netlistFilePrefix + ".route");
  result = ReplaceAll(result, "${SDC_FILE}",
                      ProjManager()->projectName() + "_openfpga.sdc");

  std::string pnrOptions;
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
  result = ReplaceAll(result, "${PNR_OPTIONS}", pnrOptions);
  std::string netlistFile;
  if (UseVerilogNetlist()) {
    netlistFile = ProjManager()->projectName() + "_post_synth.v";
  } else if (UseEdifNetlist()) {
    netlistFile = ProjManager()->projectName() + "_post_synth.edif";
  } else {
    netlistFile = ProjManager()->projectName() + "_post_synth.blif";
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
  result = ReplaceAll(result, "${VPR_TESTBENCH_BLIF}", netlistFile);

  std::string netlistFormat = "blif";
  if (UseVerilogNetlist()) {
    netlistFormat = "verilog";
  }
  if (UseEdifNetlist()) {
    netlistFormat = "edif";
  }
  result = ReplaceAll(result, "${OPENFPGA_VPR_CIRCUIT_FORMAT}", netlistFormat);
  if (m_deviceSize.size()) {
    result = ReplaceAll(result, "${OPENFPGA_VPR_DEVICE_LAYOUT}",
                        " --device " + m_deviceSize);
  } else {
    result = ReplaceAll(result, "${OPENFPGA_VPR_DEVICE_LAYOUT}", "");
  }
  result = ReplaceAll(result, "${OPENFPGA_VPR_ROUTE_CHAN_WIDTH}",
                      std::to_string(m_channel_width));
  result = ReplaceAll(result, "${OPENFPGA_ARCH_FILE}",
                      m_OpenFpgaArchitectureFile.string());

  result = ReplaceAll(result, "${OPENFPGA_SIM_SETTING_FILE}",
                      m_OpenFpgaSimSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                      m_OpenFpgaBitstreamSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_REPACK_CONSTRAINTS}",
                      m_OpenFpgaRepackConstraintsFile.string());
  if (m_OpenFpgaFabricKeyFile == "") {
    result = ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}", "");
  } else {
    result =
        ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}",
                   "--load_fabric_key " + m_OpenFpgaFabricKeyFile.string());
  }
  return result;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!HasTargetDevice()) return false;
  const bool openFpgaArch = FileUtils::FileExists(m_OpenFpgaArchitectureFile);
  if (!openFpgaArch) {
    ErrorMessage("Please specify OpenFPGA architecture file");
    return false;
  }
  if (BitsOpt() == BitstreamOpt::Clean) {
    Message("Cleaning bitstream results for " + ProjManager()->projectName());
    m_state = State::Routed;
    BitsOpt(BitstreamOpt::DefaultBitsOpt);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string("fabric_bitstream.bit"));
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string("fabric_independent_bitstream.xml"));
    return true;
  }
  if (!ProjManager()->getTargetDevice().empty()) {
    if (!LicenseDevice(ProjManager()->getTargetDevice())) {
      ErrorMessage(
          "Device is not licensed: " + ProjManager()->getTargetDevice() + "\n");
      return false;
    }
  }
  PERF_LOG("GenerateBitstream has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Bitstream generation for design \""
           << ProjManager()->projectName() << "\" on device \""
           << ProjManager()->getTargetDevice() << "\"" << std::endl;
  (*m_out) << "##################################################" << std::endl;
  if ((m_state != State::Routed) && (m_state != State::BistreamGenerated)) {
    ErrorMessage("Design needs to be in routed state");
    return false;
  }

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string("fabric_bitstream.bit"))
              .string())) {
    (*m_out) << "Design " << ProjManager()->projectName()
             << " bitstream didn't change" << std::endl;
    m_state = State::BistreamGenerated;
    return true;
  }

  if (BitsOpt() == BitstreamOpt::DefaultBitsOpt) {
#ifdef PRODUCTION_BUILD
    if (BitstreamEnabled() == false) {
      (*m_out) << "Device " << ProjManager()->getTargetDevice()
               << " bitstream is not enabled, skipping!" << std::endl;
      return true;
    }
#endif
  } else if (BitsOpt() == BitstreamOpt::Force) {
    // Force bitstream generation
  }

  std::string command = m_openFpgaExecutablePath.string() + " -batch -f " +
                        ProjManager()->projectName() + ".openfpga";

  std::string script = InitOpenFPGAScript();

  script = FinishOpenFPGAScript(script);

  std::string script_path = ProjManager()->projectName() + ".openfpga";

  std::filesystem::remove(std::filesystem::path(ProjManager()->projectName()) /
                          std::string("fabric_bitstream.bit"));
  std::filesystem::remove(std::filesystem::path(ProjManager()->projectName()) /
                          std::string("fabric_independent_bitstream.xml"));
  // Create OpenFpga command and execute
  script_path =
      (std::filesystem::path(ProjManager()->projectPath()) / script_path)
          .string();
  std::ofstream sofs(script_path);
  sofs << script;
  sofs.close();
  if (!FileUtils::FileExists(m_openFpgaExecutablePath)) {
    ErrorMessage("Cannot find executable: " +
                 m_openFpgaExecutablePath.string());
    return false;
  }

  std::ofstream ofs(
      (std::filesystem::path(ProjManager()->projectPath()) /
       std::string(ProjManager()->projectName() + "_bitstream.cmd"))
          .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " bitstream generation failed!");
    return false;
  }
  m_state = State::BistreamGenerated;

  (*m_out) << "Design " << ProjManager()->projectName()
           << " bitstream is generated!" << std::endl;
  return true;
}

bool CompilerOpenFPGA::LoadDeviceData(const std::string& deviceName) {
  bool status = true;
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
  std::filesystem::path devicefile =
      datapath / std::string("etc") / std::string("device.xml");
  QFile file(devicefile.string().c_str());
  if (!file.open(QFile::ReadOnly)) {
    ErrorMessage("Cannot open device file: " + devicefile.string());
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    ErrorMessage("Incorrect device file: " + devicefile.string());
    return false;
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  bool foundDevice = false;
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      std::string name = e.attribute("name").toStdString();
      if (name == deviceName) {
        foundDevice = true;
        QDomNodeList list = e.childNodes();
        for (int i = 0; i < list.count(); i++) {
          QDomNode n = list.at(i);
          if (!n.isNull() && n.isElement()) {
            if (n.nodeName() == "internal") {
              std::string file_type =
                  n.toElement().attribute("type").toStdString();
              std::string file = n.toElement().attribute("file").toStdString();
              std::string name = n.toElement().attribute("name").toStdString();
              std::string num = n.toElement().attribute("num").toStdString();
              std::filesystem::path fullPath;
              if (FileUtils::FileExists(file)) {
                fullPath = file;  // Absolute path
              } else {
                fullPath = datapath / std::string("etc") /
                           std::string("devices") / file;
              }
              if (!FileUtils::FileExists(fullPath.string())) {
                ErrorMessage(
                    "Invalid device config file: " + fullPath.string() + "\n");
                status = false;
              }
              if (file_type == "vpr_arch") {
                ArchitectureFile(fullPath.string());
              } else if (file_type == "openfpga_arch") {
                OpenFpgaArchitectureFile(fullPath.string());
              } else if (file_type == "bitstream_settings") {
                OpenFpgaBitstreamSettingFile(fullPath.string());
              } else if (file_type == "sim_settings") {
                OpenFpgaSimSettingFile(fullPath.string());
              } else if (file_type == "repack_settings") {
                OpenFpgaRepackConstraintsFile(fullPath.string());
              } else if (file_type == "fabric_key") {
                OpenFpgaFabricKeyFile(fullPath.string());
              } else if (file_type == "pinmap_xml") {
                OpenFpgaPinmapXMLFile(fullPath.string());
              } else if (file_type == "pinmap_csv") {
                OpenFpgaPinmapCSVFile(fullPath);
              } else if (file_type == "plugin_lib") {
                YosysPluginLibName(name);
              } else if (file_type == "plugin_func") {
                YosysPluginName(name);
              } else if (file_type == "technology") {
                YosysMapTechnology(name);
              } else if (file_type == "synth_type") {
                if (name == "QL")
                  SynthType(SynthesisType::QL);
                else if (name == "RS")
                  SynthType(SynthesisType::RS);
                else if (name == "Yosys")
                  SynthType(SynthesisType::Yosys);
                else {
                  ErrorMessage("Invalid synthesis type: " + name + "\n");
                  status = false;
                }
              } else if (file_type == "synth_opts") {
                PerDeviceSynthOptions(name);
              } else if (file_type == "vpr_opts") {
                PerDevicePnROptions(name);
              } else if (file_type == "device_size") {
                DeviceSize(name);
              } else if (file_type == "lut_size") {
                LutSize(std::strtoul(num.c_str(), nullptr, 10));
              } else if (file_type == "channel_width") {
                ChannelWidth(std::strtoul(num.c_str(), nullptr, 10));
              } else if (file_type == "bitstream_enabled") {
                if (num == "true") {
                  BitstreamEnabled(true);
                } else if (num == "false") {
                  BitstreamEnabled(false);
                } else {
                  ErrorMessage("Invalid bitstream_enabled num (true, false): " +
                               num + "\n");
                  status = false;
                }
              } else if (file_type == "pin_constraint_enabled") {
                if (num == "true") {
                  PinConstraintEnabled(true);
                } else if (num == "false") {
                  PinConstraintEnabled(false);
                } else {
                  ErrorMessage(
                      "Invalid pin_constraint_enabled num (true, false): " +
                      num + "\n");
                  status = false;
                }
              } else {
                ErrorMessage("Invalid device config type: " + file_type + "\n");
                status = false;
              }
            }
          }
        }
      }
    }

    node = node.nextSibling();
  }
  if (!foundDevice) {
    ErrorMessage("Incorrect device: " + deviceName + "\n");
    status = false;
  }
  if (!LicenseDevice(deviceName)) {
    ErrorMessage("Device is not licensed: " + deviceName + "\n");
    status = false;
  }
  return status;
}

bool CompilerOpenFPGA::LicenseDevice(const std::string& deviceName) {
  // No need for licenses
  return true;
}
