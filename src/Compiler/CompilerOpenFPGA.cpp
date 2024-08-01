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

#include "Compiler/CompilerOpenFPGA.h"

#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <filesystem>
#include <regex>
#include <sstream>
#include <thread>

#include "Compiler/Constraints.h"
#include "Configuration/CFGCommon/CFGCommon.h"
#include "Log.h"
#include "Main/Settings.h"
#include "NewProject/ProjectManager/config.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
#include "scope_guard.hpp"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

using namespace FOEDAG;

void CompilerOpenFPGA::Version(std::ostream* out) {
  (*out) << "Foedag OpenFPGA Compiler"
         << "\n";
  LogUtils::PrintVersion(out);
}

bool isRegexValid(const std::string& str) {
  try {
    const std::regex regex{str};
    return true;
  } catch (...) {
    return false;
  }
}

// In Analyze mode, returns ports (input_only: w/o outputs and ios)
// In Synthesize mode, returns inferred clocks
std::pair<bool, std::string> CompilerOpenFPGA::isRtlClock(
    const std::string& str, bool regex, bool input_only) {
  std::string signal = StringUtils::replaceAll(str, "@", "[");
  signal = StringUtils::replaceAll(signal, "%", "]");
  if (regex && !isRegexValid(str))
    return std::make_pair(false, "Invalid regular expession");
  std::string synth_script;
  std::filesystem::path synth_scrypt_path;
  std::filesystem::path outputFile;
  if (DesignChangedForAnalysis(synth_script, synth_scrypt_path, outputFile)) {
    bool ok =
        SwitchCompileContext(Action::Analyze, [this]() { return Analyze(); });
    if (!ok)
      return std::make_pair(false, "Failed to retrieve ports information");
  }
  bool vhdl{false};
  std::vector<std::string> rtl_clocks;
  if (m_state == State::Synthesized) {
    auto config_info = FilePath(Action::Synthesis, "config.json");
    if (!FileUtils::FileExists(config_info)) {
      return std::make_pair(false, "Failed to retrieve synthesis information");
    }
    rtl_clocks =
        m_tclCmdIntegration->GetClockList(config_info, vhdl, true, input_only);
  } else {
    auto port_info = FilePath(Action::Analyze, "hier_info.json");
    if (!FileUtils::FileExists(port_info)) {
      return std::make_pair(false, "Failed to retrieve ports information");
    }
    rtl_clocks =
        m_tclCmdIntegration->GetClockList(port_info, vhdl, false, input_only);
  }

  if (regex) {
    auto flags = std::regex_constants::ECMAScript;  // default value
    if (vhdl) flags |= std::regex_constants::icase;
    const std::regex regexp{signal, flags};
    for (const auto& clk : rtl_clocks) {
      if (std::regex_match(clk, regexp))
        return std::make_pair(true, std::string{});
      if (vhdl) {
        if (StringUtils::toLower(clk) == StringUtils::toLower(signal))
          return std::make_pair(true, std::string{});
      } else {
        if (clk == signal) return std::make_pair(true, std::string{});
      }
    }
  } else {
    for (const auto& clk : rtl_clocks) {
      if (vhdl) {
        if (StringUtils::toLower(clk) == StringUtils::toLower(signal))
          return std::make_pair(true, std::string{});
      } else {
        if (clk == signal) return std::make_pair(true, std::string{});
      }
    }
  }
  return std::make_pair(false, std::string{});
}

std::vector<std::string> CompilerOpenFPGA::helpTags() const {
  return {"openfpga"};
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
write_blif -param ${OUTPUT_EBLIF}
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

  auto routing_graph = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify a routing graph file");
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
      compiler->ErrorMessage("Cannot find routing graph file: " +
                             std::string(expandedFile));
      return TCL_ERROR;
    }
    std::filesystem::path the_path = expandedFile;
    if (!the_path.is_absolute()) {
      const auto& path = std::filesystem::current_path();
      expandedFile = std::filesystem::path(path / expandedFile).string();
    }
    stream.close();

    compiler->RoutingGraphFile(expandedFile);
    compiler->Message("VPR routing graph file: " + expandedFile);
    return TCL_OK;
  };
  interp->registerCmd("routing_graph", routing_graph, this, 0);

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
      } else if (arg == "-key") {
        fileType = "key";
      } else {
        compiler->ErrorMessage(
            "Not a legal option for bitstream_config_files: " + arg);
        return TCL_ERROR;
      }
      i++;
      std::string expandedFile = argv[i];
      if (!expandedFile.empty()) {
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
      }
      if (fileType == "bitstream") {
        compiler->OpenFpgaBitstreamSettingFile(expandedFile);
        compiler->Message("OpenFPGA Bitstream Setting file: " + expandedFile);
      } else if (fileType == "sim") {
        compiler->OpenFpgaSimSettingFile(expandedFile);
        compiler->Message("OpenFPGA Simulation Setting file: " + expandedFile);
      } else if (fileType == "repack") {
        compiler->OpenFpgaRepackConstraintsFile(expandedFile);
        compiler->Message("OpenFPGA Repack Constraint file: " + expandedFile);
      } else if (fileType == "key") {
        compiler->OpenFpgaFabricKeyFile(expandedFile);
        compiler->Message("OpenFPGA Fabric Key Constraint file: " +
                          expandedFile);
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

  auto set_limits = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (argc != 3) {
      compiler->ErrorMessage(
          "Specify a limit type and a value ie: set_limits bram 20");
      return TCL_ERROR;
    }
    std::string type = argv[1];
    if (type == "dsp") {
      compiler->MaxUserDSPCount(std::strtoul(argv[2], 0, 10));
    } else if (type == "bram") {
      compiler->MaxUserBRAMCount(std::strtoul(argv[2], 0, 10));
    } else if (type == "carry_length") {
      compiler->MaxUserCarryLength(std::strtoul(argv[2], 0, 10));
    } else {
      compiler->ErrorMessage("Unknown limit type");
      return TCL_ERROR;
    }
    if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
    return TCL_OK;
  };
  interp->registerCmd("set_limits", set_limits, this, 0);

  auto flat_routing = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (argc != 2) {
      compiler->ErrorMessage("Specify: flat_routing true/false");
      return TCL_ERROR;
    }
    std::string type = argv[1];
    if (type == "true") {
      compiler->FlatRouting(true);
    } else if (type == "false") {
      compiler->FlatRouting(false);
    } else {
      compiler->ErrorMessage("Specify: flat_routing true/false");
      return TCL_ERROR;
    }
    if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
    return TCL_OK;
  };
  interp->registerCmd("flat_routing", flat_routing, this, 0);

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
      compiler->SetNetlistType(NetlistType::Verilog);
    } else if (arg == "edif") {
      compiler->SetNetlistType(NetlistType::Edif);
    } else if (arg == "blif") {
      compiler->SetNetlistType(NetlistType::Blif);
    } else if (arg == "eblif") {
      compiler->SetNetlistType(NetlistType::EBlif);
    } else if (arg == "vhdl") {
      compiler->SetNetlistType(NetlistType::VHDL);
    } else {
      compiler->ErrorMessage(
          "Invalid arg to netlist_type (verilog or blif), was: " + arg);
      return TCL_ERROR;
    }
    if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
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
    if (arg == "on") {
      compiler->SetParserType(ParserType::Verific);
    } else {
      compiler->SetParserType(ParserType::Default);
    }
    return TCL_OK;
  };
  interp->registerCmd("verific_parser", verific_parser, this, 0);

  auto parser_type = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify on/off");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (arg == "verific") {
      compiler->SetParserType(ParserType::Verific);
    } else if (arg == "surelog") {
      compiler->SetParserType(ParserType::Surelog);
    } else if (arg == "ghdl") {
      compiler->SetParserType(ParserType::GHDL);
    } else if (arg == "yosys") {
      compiler->SetParserType(ParserType::Default);
    } else {
      compiler->ErrorMessage("Unknown parser type: " + arg);
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("parser_type", parser_type, this, 0);

  auto target_device = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc != 2) {
      compiler->ErrorMessage("Please select a device");
      return TCL_ERROR;
    }
    std::string arg = argv[1];
    if (!compiler->GetSession()->CmdLine()->Device().empty()) {
      arg = compiler->GetSession()->CmdLine()->Device();
    }
    if (compiler->LoadDeviceData(arg)) {
      compiler->ProjManager()->setTargetDevice(arg);
      auto deviceData = compiler->deviceData();
      compiler->ProjManager()->setTargetDeviceData(
          deviceData.family, deviceData.series, deviceData.package);
      compiler->Message("Target device: " + arg);
      if (!compiler->DeviceTagVersion().empty())
        compiler->Message("Device version: " + compiler->DeviceTagVersion());
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
  auto packing_options = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    CompilerOpenFPGA* compiler = (CompilerOpenFPGA*)clientData;
    for (int i = 1; i < argc; i++) {
      std::string opt = argv[i];
      if (opt == "-clb_packing") {
        if (i < (argc - 1)) {
          ClbPacking packing{ClbPacking::Auto};
          std::string type{argv[i + 1]};
          if (type == "auto") {
            packing = ClbPacking::Auto;
          } else if (type == "dense") {
            packing = ClbPacking::Dense;
          } else if (type == "timing_driven") {
            packing = ClbPacking::Timing_driven;
          } else {
            compiler->ErrorMessage("Allowed types: auto/dense/timing_driven");
            return TCL_ERROR;
          }
          compiler->ClbPackingOption(packing);
          if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
          i++;
        } else {
          compiler->ErrorMessage("-clb_packing argument is missing");
          return TCL_ERROR;
        }
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("packing_options", packing_options, this, nullptr);
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
    if (node.attribute("name").toStdString() == size) {
      std::string device_dimension =
          CFG_print("%sx%s", node.attribute("width").toStdString().c_str(),
                    node.attribute("height").toStdString().c_str());
      return std::make_pair(true, device_dimension);
    }
  }
  return std::make_pair(false, std::string{"Device size is not correct"});
}

void CompilerOpenFPGA::SetDeviceResources() {
  if (m_taskManager) {
    auto reports = m_taskManager->getReportManagerRegistry().ids();
    Resources resources{};
    resources.bram.bram_36k = MaxDeviceBRAMCount();
    resources.bram.bram_18k = MaxDeviceBRAMCount() * 2;
    resources.dsp.dsp_9_10 = MaxDeviceDSPCount();
    resources.dsp.dsp_18_20 = MaxDeviceDSPCount() * 2;
    resources.logic.dff = MaxDeviceFFCount();
    resources.logic.latch = MaxDeviceFFCount();
    resources.logic.clb = MaxDeviceLUTCount() / 8;
    resources.logic.fa2Bits = MaxDeviceLUTCount();
    resources.logic.lut6 = MaxDeviceLUTCount();
    resources.logic.lut5 = MaxDeviceLUTCount() * 2;
    resources.inouts.io = MaxDeviceIOCount();
    resources.inouts.inputs = MaxDeviceIOCount();
    resources.inouts.outputs = MaxDeviceIOCount();
    for (auto id : reports) {
      m_taskManager->getReportManagerRegistry()
          .getReportManager(id)
          ->setAvailableResources(resources);
    }
  }
}

bool CompilerOpenFPGA::VerifyTargetDevice() const {
  const bool target = Compiler::VerifyTargetDevice();
  const bool archFile = FileUtils::FileExists(m_architectureFile);
  return target || archFile;
}

std::filesystem::path CompilerOpenFPGA::copyLog(
    FOEDAG::ProjectManager* projManager, const std::string& srcFileName,
    const std::string& destFileName) {
  std::filesystem::path dest{};

  if (projManager) {
    std::filesystem::path projectPath(fs::current_path());
    std::filesystem::path src = projectPath / srcFileName;
    if (FileUtils::FileExists(src)) {
      dest = projectPath / destFileName;
      std::filesystem::remove(dest);
      std::filesystem::copy_file(src, dest);
    }
  }

  return dest;
}

bool CompilerOpenFPGA::DesignChangedForAnalysis(
    std::string& synth_script, std::filesystem::path& synth_scrypt_path,
    std::filesystem::path& outputFile) {
  synth_script = InitAnalyzeScript();
  synth_script = FinishAnalyzeScript(synth_script);

  synth_scrypt_path =
      FilePath(Action::Analyze, ProjManager()->projectName() + "_analyzer.cmd");
  outputFile = FilePath(Action::Analyze, "port_info.json");
  return DesignChanged(synth_script, synth_scrypt_path, outputFile);
}

void CompilerOpenFPGA::processCustomLayout() {
  fs::path impl{ProjectManager::implPath(ProjManager()->projectPath())};
  auto customLayoutPath = impl / "custom_layout.txt";
  auto layouts = Config::Instance()->layoutsPath();
  std::string targetDevice = ProjManager()->getTargetDevice();
  auto files =
      FileUtils::FindFileInDirs(targetDevice + ".xml", {layouts}, true);
  if (!files.empty()) {
    std::filesystem::copy_file(
        files.at(0), customLayoutPath,
        std::filesystem::copy_options::overwrite_existing);
  }
}

void CompilerOpenFPGA::RenamePostSynthesisFiles(Action action) {
  auto routePath = FilePath(action);
  auto regex = std::regex{".+_post_synthesis\\..+"};
  auto files = FileUtils::FindFilesByName(routePath, regex);
  for (auto& fileName : files) {
    auto oldName = fileName.filename().string();
    std::string newName = std::regex_replace(
        oldName, std::regex{"_post_synthesis"}, "_post_route");
    FileUtils::RenameFile(fileName, std::filesystem::path{newName});
  }
}

bool CompilerOpenFPGA::IPGenerate() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!HasTargetDevice()) return false;
  if (!HasIPInstances()) {
    // No instances configured, no-op w/o error
    return true;
  }
  PERF_LOG("IPGenerate has started");
  Message("##################################################");
  Message("IP generation for design: " + ProjManager()->projectName());
  Message("##################################################");
  bool status = GetIPGenerator()->Generate();
  if (status) {
    Message("Design " + m_projManager->projectName() + " IPs are generated");
    m_state = State::IPGenerated;
  } else {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " IPs generation failed");
  }
  return true;
}

bool CompilerOpenFPGA::DesignChanged(
    const std::string& synth_script,
    const std::filesystem::path& synth_scrypt_path,
    const std::filesystem::path& outputFile) {
  bool result = false;
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
  for (auto file : ProjManager()->getConstrFiles()) {
    file = StringUtils::trim(file);
    if (file.size()) {
      time_t tf = FileUtils::Mtime(file);
      if ((tf > time_netlist) || (tf == -1)) {
        result = true;
        break;
      }
    }
  }
  for (auto path : ProjManager()->includePathList()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string(), " ",
        tokens);
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
    StringUtils::tokenize(
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string(), " ",
        tokens);
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
  return result;
}

void CompilerOpenFPGA::reloadSettings() {
  FOEDAG::Settings* settings = GlobalSession->GetSettings();
  try {
    auto& synth = settings->getJson()["Tasks"]["Synthesis"];
    synth["dsp_spinbox_ex"]["maxVal"] = MaxDeviceDSPCount();
    synth["bram_spinbox_ex"]["maxVal"] = MaxDeviceBRAMCount();
    synth["carry_chain_spinbox_ex"]["maxVal"] = MaxDeviceCarryLength();
  } catch (std::exception& e) {
    ErrorMessage(e.what());
  }
  if (getConstraints() != nullptr) {
    getConstraints()->clear_property();
  }
}

std::string CompilerOpenFPGA::InitAnalyzeScript() {
  std::string analysisScript;
  switch (GetParserType()) {
    case ParserType::Verific: {
      // Verific parser
      std::string fileList;
      fileList += "-set-warning VERI-1063\n";
      std::string includes;
      for (auto path : ProjManager()->includePathList()) {
        includes +=
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
      }

      // Add Tcl project directory as an include dir
      if (!GetSession()->CmdLine()->Script().empty()) {
        std::filesystem::path script = GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        includes += FileUtils::AdjustPath(scriptPath.string(),
                                          ProjManager()->projectPath())
                        .string() +
                    " ";
      }

      // Add design files directory as an include dir
      std::set<std::string> designFileDirs;
      for (const auto& lang_file : ProjManager()->DesignFiles()) {
        const std::string& fileNames = lang_file.second;
        std::vector<std::string> files;
        StringUtils::tokenize(fileNames, " ", files);
        for (auto file : files) {
          std::filesystem::path filePath = file;
          filePath = filePath.parent_path();
          const std::string& path = filePath.string();
          if (designFileDirs.find(path) == designFileDirs.end()) {
            includes +=
                FileUtils::AdjustPath(path, ProjManager()->projectPath())
                    .string() +
                " ";
            designFileDirs.insert(path);
          }
        }
      }

      if (!includes.empty()) fileList += "-vlog-incdir " + includes + "\n";

      std::string libraries;
      for (auto path : ProjManager()->libraryPathList()) {
        libraries +=
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
      }
      if (!libraries.empty()) fileList += "-vlog-libdir " + libraries + "\n";

      for (auto ext : ProjManager()->libraryExtensionList()) {
        fileList += "-vlog-libext " + ext + "\n";
      }

      std::string macros;
      for (auto& macro_value : ProjManager()->macroList()) {
        macros += macro_value.first + "=" + macro_value.second + " ";
      }
      if (!macros.empty()) fileList += "-vlog-define " + macros + "\n";

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
          case Design::Language::VHDL_2019:
            lang = "-vhdl2019";
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
            lang = "-sv";
            break;
          case Design::Language::BLIF:
          case Design::Language::EBLIF:
            lang = "BLIF";
            SetError("Unsupported file format: " + lang);
            return "";
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
      auto topModuleLib = ProjManager()->DesignTopModuleLib();
      if (!topModuleLib.empty()) {
        fileList += "-work " + topModuleLib + "\n";
      }
      analysisScript = fileList;
      break;
    }
    case ParserType::Default: {
      std::string designFiles = YosysDesignParsingCommmands();
      analysisScript = designFiles;
      analysisScript += "\n";
      if (ProjManager()->DesignTopModule().empty()) {
        analysisScript += "analyze -auto-top ";
      } else {
        analysisScript += "analyze -top " + ProjManager()->DesignTopModule();
      }
      analysisScript += "\n";
      break;
    }
    case ParserType::Surelog: {
      std::string designFiles = SurelogDesignParsingCommmands();
      analysisScript = designFiles;
      analysisScript += "\n";
      if (ProjManager()->DesignTopModule().empty()) {
        analysisScript += "analyze -auto-top ";
      } else {
        analysisScript += "analyze -top " + ProjManager()->DesignTopModule();
      }
      analysisScript += "\n";
      break;
    }
    case ParserType::GHDL: {
      std::string designFiles = GhdlDesignParsingCommmands();
      analysisScript = designFiles;
      analysisScript += "\n";
      if (ProjManager()->DesignTopModule().empty()) {
        analysisScript += "analyze -auto-top ";
      } else {
        analysisScript += "analyze -top " + ProjManager()->DesignTopModule();
      }
      analysisScript += "\n";
      break;
    }
  }
  return analysisScript;
}

std::string CompilerOpenFPGA::FinishAnalyzeScript(const std::string& script) {
  std::string result = script;
  return result;
}

bool CompilerOpenFPGA::Analyze() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Log generated by ExecuteAndMonitorSystemCommand, we just need to add
    // header info to the log
    std::filesystem::path logPath = FilePath(Action::Analyze, ANALYSIS_LOG);
    LogUtils::AddHeaderToLog(logPath);
  });

  auto printTopModules = [this](const std::filesystem::path& filePath,
                                std::ostream* out) {
    // Check for "topModule" in a given json filePath
    // Assumed json format is [ { "topModule" : "some_value"} ]
    if (out) {
      auto topModules = TopModules(filePath);
      if (!topModules.empty()) {
        if (m_projManager->DesignTopModule().empty()) {
          for (const auto& top : topModules) {
            if (!top.empty()) {
              m_projManager->setCurrentFileSet(
                  m_projManager->getDesignActiveFileSet());
              m_projManager->setTopModule(QString::fromStdString(top));
            }
          }
        }
        (*out) << "Top Modules: " << StringUtils::join(topModules, ", ")
               << std::endl;
      }
    }
  };

  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (AnalyzeOpt() == DesignAnalysisOpt::Clean) {
    Message("Cleaning analysis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    AnalyzeOpt(DesignAnalysisOpt::None);
    CleanFiles(Action::Analyze);
    return true;
  }
  if (!HasTargetDevice()) return false;

  PERF_LOG("Analysis has started");
  Message("##################################################");
  Message("Analysis for design: " + ProjManager()->projectName());
  Message("##################################################");

  if (GetParserType() == ParserType::Default) {
    bool hasVhdl = false;
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      switch (lang_file.first.language) {
        case Design::Language::VHDL_1987:
        case Design::Language::VHDL_1993:
        case Design::Language::VHDL_2000:
        case Design::Language::VHDL_2008:
        case Design::Language::VHDL_2019:
          hasVhdl = true;
          break;
        default:
          break;
      }
    }
    if (hasVhdl) {
      // For GHDL parser
      SetParserType(ParserType::GHDL);
    }
  }

  std::string analysisScript;
  std::filesystem::path script_path;
  std::filesystem::path output_path;
  if (!DesignChangedForAnalysis(analysisScript, script_path, output_path)) {
    Message("Design didn't change: " + ProjManager()->projectName() +
            ", skipping analysis.");
    std::stringstream tempOut{};
    printTopModules(output_path, &tempOut);
    Message(tempOut.str());
    return true;
  }
  // Create Analyser command and execute
  FileUtils::WriteToFile(script_path, analysisScript, false);
  std::string command;
  int status = 0;
  std::filesystem::path analyse_path = ANALYSIS_LOG;
  if (GetParserType() == ParserType::Default ||
      GetParserType() == ParserType::Surelog ||
      GetParserType() == ParserType::GHDL) {
    m_analyzeExecutablePath = m_analyzeExecutablePath.parent_path();
    m_analyzeExecutablePath = m_analyzeExecutablePath / "yosys";
  }
  if (!FileUtils::FileExists(m_analyzeExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_analyzeExecutablePath.string());
    return false;
  }
  if (GetParserType() == ParserType::Default ||
      GetParserType() == ParserType::Surelog ||
      GetParserType() == ParserType::GHDL) {
    // Yosys-based analyze
    command = m_analyzeExecutablePath.string() + " -s " + script_path.string();
  } else {
    // Verific-based analyze
    command = m_analyzeExecutablePath.string() + " -f " + script_path.string();
  }
  Message("Analyze command: " + command);
  status = ExecuteAndMonitorSystemCommand(command, analyse_path.string(), false,
                                          FilePath(Action::Analyze));

  if (status) {
    if (GetParserType() == ParserType::Default) {
      std::ifstream raptor_log(analyse_path.string());
      if (raptor_log.good()) {
        std::stringstream buffer;
        buffer << raptor_log.rdbuf();
        const std::string& buf = buffer.str();
        raptor_log.close();
        if (buf.find("Executing HIERARCHY pass") == std::string::npos) {
          // If Default Yosys parser fails, attempt with the Surelog parser
          ErrorMessage("Default parser failed, re-attempting with SV parser");
          SetParserType(ParserType::Surelog);
          bool subResult = Analyze();
          if (subResult) {
            status = 0;
          }
        }
      }
    }
  }
  std::ifstream raptor_log(analyse_path.string());
  if (raptor_log.good()) {
    std::stringstream buffer;
    buffer << raptor_log.rdbuf();
    const std::string& buf = buffer.str();
    raptor_log.close();
    if (buf.find("VERI-1063") != std::string::npos) {
      std::string modules = "";
      int pos = 0;
      while (buf.find("instantiating unknown module ", pos) !=
             std::string::npos) {
        pos = buf.find("instantiating unknown module ", pos) +
              29;  // the searched line size
        std::string tmp = " ";
        while (buf[pos] != ' ') {
          tmp += buf[pos];
          ++pos;
        }
        if (modules.find(tmp) == std::string::npos) modules += tmp;
      }

      modules += " ";
      ErrorMessage(
          "Design " + ProjManager()->projectName() +
          " has an incomplete hierarchy, unknown module(s) error(s). ");
      ErrorMessage("Unknown modules:" + modules);
      status = true;
    }
  }
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " analysis failed");
    return false;
  } else {
    m_state = State::Analyzed;
    Message("Design " + ProjManager()->projectName() + " is analyzed");
  }

  std::stringstream tempOut{};
  printTopModules(output_path, &tempOut);
  Message(tempOut.str());
  return true;
}

std::string CompilerOpenFPGA::GhdlDesignParsingCommmands() {
  // GHDL parser
  std::string fileList;

  for (auto msg_sev : MsgSeverityMap()) {
    switch (msg_sev.second) {
      case MsgSeverity::Ignore:
        break;
      case MsgSeverity::Info:
        break;
      case MsgSeverity::Warning:
        break;
      case MsgSeverity::Error:
        break;
    }
  }
  std::string searchPath;
  std::string includes;
  std::set<std::string> designFileDirs;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    const std::string& fileNames = lang_file.second;
    std::vector<std::string> files;
    StringUtils::tokenize(fileNames, " ", files);
    for (auto file : files) {
      std::filesystem::path filePath = file;
      filePath = filePath.parent_path();
      const std::string& path = filePath.string();
      if (designFileDirs.find(path) == designFileDirs.end()) {
        searchPath +=
            "-P" +
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
        includes +=
            "-I" +
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
        designFileDirs.insert(path);
      }
    }
  }

  std::string macros;
  for (auto& macro_value : ProjManager()->macroList()) {
    macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
  }

  for (auto path : ProjManager()->includePathList()) {
    includes +=
        "-I" +
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
        " ";
  }

  // Add Tcl project directory as an include dir
  if (!GetSession()->CmdLine()->Script().empty()) {
    std::filesystem::path script = GetSession()->CmdLine()->Script();
    std::filesystem::path scriptPath = script.parent_path();
    includes +=
        "-I" +
        FileUtils::AdjustPath(scriptPath.string(), ProjManager()->projectPath())
            .string() +
        " ";
  }

  auto topModuleLib = ProjManager()->DesignTopModuleLib();
  auto commandsLibs = ProjManager()->DesignLibraries();
  size_t filesIndex{0};
  std::string lang;
  std::string designLibraries;
  std::string verilogFiles;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VHDL_1987:
        lang = "--std=87";
        break;
      case Design::Language::VHDL_1993:
        lang = "--std=93";
        break;
      case Design::Language::VHDL_2000:
        lang = "vhdl2000";
        SetError("Unsupported file format: " + lang);
        return "";
      case Design::Language::VHDL_2008:
        lang = "--std=08";
        break;
      case Design::Language::VHDL_2019:
        lang = "vhdl2019";
        SetError("Unsupported file format: " + lang);
        return "";
      case Design::Language::VERILOG_1995:
        ++filesIndex;
        verilogFiles += lang_file.second + " ";
        continue;
      case Design::Language::VERILOG_2001:
        ++filesIndex;
        verilogFiles += lang_file.second + " ";
        continue;
      case Design::Language::SYSTEMVERILOG_2005:
        ++filesIndex;
        verilogFiles += "-sv " + lang_file.second + " ";
        continue;
      case Design::Language::SYSTEMVERILOG_2009:
        ++filesIndex;
        verilogFiles += "-sv " + lang_file.second + " ";
        continue;
      case Design::Language::SYSTEMVERILOG_2012:
        ++filesIndex;
        verilogFiles += "-sv " + lang_file.second + " ";
        continue;
      case Design::Language::SYSTEMVERILOG_2017:
        ++filesIndex;
        verilogFiles += "-sv " + lang_file.second + " ";
        continue;
      case Design::Language::VERILOG_NETLIST:
        lang = "verilog";
        SetError("Unsupported file format: " + lang);
        return "";
      case Design::Language::BLIF:
      case Design::Language::EBLIF:
        lang = "blif";
        SetError("Unsupported file format: " + lang);
        return "";
    }
    if (filesIndex < commandsLibs.size()) {
      const auto& filesCommandsLibs = commandsLibs[filesIndex];
      for (size_t i = 0; i < filesCommandsLibs.first.size(); ++i) {
        auto libName = filesCommandsLibs.second[i];
        if (!libName.empty()) {
          designLibraries = libName;
        }
      }
    }
    ++filesIndex;
    fileList += lang_file.second + " ";
  }
  std::filesystem::path binpath = GetSession()->Context()->BinaryPath();
  std::filesystem::path prefixPackagePath =
      binpath / "HDL_simulator" / "GHDL" / "lib" / "ghdl";
  std::string verilogcmd;
  if (!verilogFiles.empty()) {
    if (verilogFiles.find("-sv") != std::string::npos) {
      verilogcmd = "plugin -i systemverilog\nread_systemverilog -synth " +
                   macros + includes + verilogFiles + "\n";
    } else {
      if (!macros.empty()) verilogcmd += "verilog_defines " + macros + "\n";
      verilogcmd += "read_verilog " + includes + verilogFiles + "\n";
    }
  }
  fileList =
      "plugin -i ghdl\nghdl -frelaxed-rules --no-formal -fsynopsys -fexplicit "
      "--PREFIX=" +
      prefixPackagePath.string() + " " + searchPath + lang + " " + fileList +
      " -e " + designLibraries + "\n";
  fileList += verilogcmd;

  return fileList;
}

std::string CompilerOpenFPGA::SurelogDesignParsingCommmands() {
  // Surelog parser
  std::string fileList;
  std::string includes;

  for (auto msg_sev : MsgSeverityMap()) {
    switch (msg_sev.second) {
      case MsgSeverity::Ignore:
        break;
      case MsgSeverity::Info:
        break;
      case MsgSeverity::Warning:
        break;
      case MsgSeverity::Error:
        break;
    }
  }

  for (auto path : ProjManager()->includePathList()) {
    includes +=
        "-I" +
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
        " ";
  }

  // Add Tcl project directory as an include dir
  if (!GetSession()->CmdLine()->Script().empty()) {
    std::filesystem::path script = GetSession()->CmdLine()->Script();
    std::filesystem::path scriptPath = script.parent_path();
    includes +=
        "-I" +
        FileUtils::AdjustPath(scriptPath.string(), ProjManager()->projectPath())
            .string() +
        " ";
  }

  std::set<std::string> designFileDirs;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    const std::string& fileNames = lang_file.second;
    std::vector<std::string> files;
    StringUtils::tokenize(fileNames, " ", files);
    for (auto file : files) {
      std::filesystem::path filePath = file;
      filePath = filePath.parent_path();
      const std::string& path = filePath.string();
      if (designFileDirs.find(path) == designFileDirs.end()) {
        includes +=
            "-I" +
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
        designFileDirs.insert(path);
      }
    }
  }

  std::string libraries;
  for (auto path : ProjManager()->libraryPathList()) {
    libraries +=
        "-y " +
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
        " ";
  }

  std::string extensions;
  if (!ProjManager()->libraryExtensionList().empty()) extensions = "+libext";
  for (auto ext : ProjManager()->libraryExtensionList()) {
    extensions += "+" + ext;
  }
  extensions += " ";

  std::string macros;
  for (auto& macro_value : ProjManager()->macroList()) {
    macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
  }

  std::string importLibs;
  auto importDesignFilesLibs = false;

  auto topModuleLib = ProjManager()->DesignTopModuleLib();
  auto commandsLibs = ProjManager()->DesignLibraries();
  size_t filesIndex{0};
  std::set<std::string> fileSet;
  std::string lang;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    if (fileSet.find(lang_file.second) != fileSet.end()) {
      continue;
    }
    fileSet.insert(lang_file.second);
    std::string designLibraries;
    switch (lang_file.first.language) {
      case Design::Language::VHDL_1987:
      case Design::Language::VHDL_1993:
      case Design::Language::VHDL_2000:
      case Design::Language::VHDL_2008:
      case Design::Language::VHDL_2019:
        lang = "vhdl";
        SetError("Unsupported file format: " + lang);
        return "";
      case Design::Language::VERILOG_1995:
        lang = "";
        break;
      case Design::Language::VERILOG_2001:
        lang = "";
        importDesignFilesLibs = true;
        break;
      case Design::Language::SYSTEMVERILOG_2005:
        lang = "-sv";
        importDesignFilesLibs = true;
        break;
      case Design::Language::SYSTEMVERILOG_2009:
        lang = "-sv";
        importDesignFilesLibs = true;
        break;
      case Design::Language::SYSTEMVERILOG_2012:
        lang = "-sv";
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
        lang = "blif";
        SetError("Unsupported file format: " + lang);
        return "";
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
    fileList += lang_file.second + " \\\n";
  }
  auto topModuleLibImport = std::string{};
  if (!topModuleLib.empty()) topModuleLibImport = "-work " + topModuleLib + " ";
  std::string top;
  if (!ProjManager()->DesignTopModule().empty()) {
    top = " -top " + ProjManager()->DesignTopModule() + " ";
  }
  fileList = "plugin -i systemverilog\nread_systemverilog -synth " + top +
             macros + libraries + includes + extensions + lang + " " + fileList;
  return fileList;
}

std::string CompilerOpenFPGA::YosysDesignParsingCommmands() {
  // Default Yosys parser

  for (const auto& commandLib : ProjManager()->DesignLibraries()) {
    if (!commandLib.first.empty()) {
      ErrorMessage(
          "Yosys default parser doesn't support '-work' design file "
          "command");
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
    includes +=
        "-I" +
        FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
        " ";
  }

  std::vector<std::string> extentions{".v"};
  if (!ProjManager()->libraryExtensionList().empty()) {
    extentions.clear();
    for (auto entry : ProjManager()->libraryExtensionList()) {
      extentions.push_back(entry);
    }
  }

  // Add Tcl project directory as an include dir
  if (!GetSession()->CmdLine()->Script().empty()) {
    std::filesystem::path script = GetSession()->CmdLine()->Script();
    std::filesystem::path scriptPath = script.parent_path();
    includes +=
        "-I" +
        FileUtils::AdjustPath(scriptPath.string(), ProjManager()->projectPath())
            .string() +
        " ";
  }

  std::set<std::string> designFileDirs;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    const std::string& fileNames = lang_file.second;
    std::vector<std::string> files;
    StringUtils::tokenize(fileNames, " ", files);
    for (auto file : files) {
      std::filesystem::path filePath = file;
      filePath = filePath.parent_path();
      const std::string& path = filePath.string();
      if (designFileDirs.find(path) == designFileDirs.end()) {
        includes +=
            "-I" +
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
        designFileDirs.insert(path);
      }
    }
  }

  std::set<std::string> designFileSet;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    const std::string& fileNames = lang_file.second;
    std::vector<std::string> files;
    StringUtils::tokenize(fileNames, " ", files);
    for (auto file : files) {
      std::filesystem::path filePath = file;
      designFileSet.insert(filePath.filename().string());
    }
  }

  std::string designFiles = macros;
  for (auto path : ProjManager()->libraryPathList()) {
    std::filesystem::path libPath =
        FileUtils::AdjustPath(path, ProjManager()->projectPath());
    if (std::filesystem::exists(libPath) &&
        std::filesystem::is_directory(libPath)) {
      for (auto const& dir_entry :
           std::filesystem::directory_iterator{libPath}) {
        for (auto ext : extentions) {
          if (ext == dir_entry.path().extension()) {
            if (designFileSet.find(dir_entry.path().filename().string()) ==
                designFileSet.end()) {
              bool fileContainsModuleOfSameName = false;
              std::ifstream ifs(dir_entry.path());
              if (ifs.good()) {
                std::stringstream buffer;
                buffer << ifs.rdbuf();
                std::string moduleName = dir_entry.path().stem().string();
                const std::regex regexpMod{"(module)[ \t]+(" + moduleName +
                                           ")"};
                if (std::regex_search(buffer.str(), regexpMod)) {
                  fileContainsModuleOfSameName = true;
                }
                const std::regex regexpPrim{"(primitive)[ \t]+(" + moduleName +
                                            ")"};
                if (std::regex_search(buffer.str(), regexpPrim)) {
                  fileContainsModuleOfSameName = true;
                }
                const std::regex regexpPack{"(package)[ \t]"};
                if (std::regex_search(buffer.str(), regexpPack)) {
                  // Files containing packages cannot be imported with -y
                  fileContainsModuleOfSameName = false;
                }
              }
              ifs.close();
              if (fileContainsModuleOfSameName) {
                designFiles += "read_verilog " + includes +
                               dir_entry.path().string() + "\n";
              }
            }
          }
        }
      }
    }
  }

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
      case Design::Language::VHDL_2019:
        ErrorMessage("Unsupported language (Yosys default parser)");
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
        break;
      case Design::Language::BLIF:
      case Design::Language::EBLIF:
        ErrorMessage("Unsupported language (Yosys default parser)");
        break;
    }
    filesScript = ReplaceAll(filesScript, "${READ_VERILOG_OPTIONS}", lang);
    filesScript = ReplaceAll(filesScript, "${INCLUDE_PATHS}", includes);
    filesScript = ReplaceAll(filesScript, "${VERILOG_FILES}", files);

    designFiles += filesScript + "\n";
  }
  return designFiles;
}

bool CompilerOpenFPGA::Synthesize() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    std::filesystem::path configJsonPath =
        FilePath(Action::Synthesis) / "config.json";
    std::filesystem::path fabricJsonPath =
        FilePath(Compiler::Action::Synthesis) / "fabric_netlist_info.json";
    getNetlistEditData()->ReadData(configJsonPath, fabricJsonPath);

    // Rename log file
    copyLog(ProjManager(), ProjManager()->projectName() + "_synth.log",
            SYNTHESIS_LOG);
  });

  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    SynthOpt(SynthesisOpt::None);
    CleanFiles(Action::Synthesis);
    return true;
  }
  if (!HasTargetDevice()) return false;

  PERF_LOG("Synthesize has started");
  Message("##################################################");
  Message("Synthesis for design: " + ProjManager()->projectName());
  Message("##################################################");
  std::string yosysScript = InitSynthesisScript();

  // update constraints
  const auto& constrFiles = ProjManager()->getConstrFiles();
  m_constraints->reset();
  for (const auto& file : constrFiles) {
    int res{TCL_OK};
    auto status =
        m_interp->evalCmd(std::string("read_sdc {" + file + "}").c_str(), &res);
    if (res != TCL_OK) {
      ErrorMessage(status);
      return false;
    }
  }

  const std::string sdcOut =
      "pin_location_" + ProjManager()->projectName() + ".sdc";
  std::ofstream ofssdc(sdcOut);
  for (auto constraint : m_constraints->getConstraints()) {
    constraint = ReplaceAll(constraint, "@", "[");
    constraint = ReplaceAll(constraint, "%", "]");
    // pin location constraints have to be translated to .place:
    if ((constraint.find("set_pin_loc") != std::string::npos)) {
      ofssdc << constraint << std::endl;
    } else if (constraint.find("set_mode") != std::string::npos) {
      ofssdc << constraint << std::endl;
    } else if ((constraint.find("set_property") != std::string::npos) &&
               (constraint.find(" mode ") != std::string::npos)) {
      constraint = ReplaceAll(constraint, " mode ", " ");
      constraint = ReplaceAll(constraint, "set_property", "set_mode");
      ofssdc << constraint << std::endl;
    }
  }
  ofssdc.close();

  if (GetParserType() == ParserType::Default) {
    bool hasVhdl = false;
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      switch (lang_file.first.language) {
        case Design::Language::VHDL_1987:
        case Design::Language::VHDL_1993:
        case Design::Language::VHDL_2000:
        case Design::Language::VHDL_2008:
        case Design::Language::VHDL_2019:
          hasVhdl = true;
          break;
        default:
          break;
      }
    }
    if (hasVhdl) {
      // For GHDL parser
      SetParserType(ParserType::GHDL);
    }
  }

  switch (GetParserType()) {
    case ParserType::Verific: {
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
        includes +=
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
      }

      // Add Tcl project directory as an include dir
      if (!GetSession()->CmdLine()->Script().empty()) {
        std::filesystem::path script = GetSession()->CmdLine()->Script();
        std::filesystem::path scriptPath = script.parent_path();
        includes += FileUtils::AdjustPath(scriptPath.string(),
                                          ProjManager()->projectPath())
                        .string() +
                    " ";
      }

      std::set<std::string> designFileDirs;
      for (const auto& lang_file : ProjManager()->DesignFiles()) {
        const std::string& fileNames = lang_file.second;
        std::vector<std::string> files;
        StringUtils::tokenize(fileNames, " ", files);
        for (auto file : files) {
          std::filesystem::path filePath = file;
          filePath = filePath.parent_path();
          const std::string& path = filePath.string();
          if (designFileDirs.find(path) == designFileDirs.end()) {
            includes +=
                FileUtils::AdjustPath(path, ProjManager()->projectPath())
                    .string() +
                " ";
            designFileDirs.insert(path);
          }
        }
      }

      fileList += "verific -vlog-incdir " + includes + "\n";

      std::string libraries;
      for (auto path : ProjManager()->libraryPathList()) {
        libraries +=
            FileUtils::AdjustPath(path, ProjManager()->projectPath()).string() +
            " ";
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
          case Design::Language::VHDL_2019:
            lang = "-vhdl2019";
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
            SetError("Unsupported file format: " + lang);
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
          fileList += "verific " + designLibraries + lang + " " +
                      lang_file.second + "\n";
      }
      auto topModuleLibImport = std::string{};
      if (!topModuleLib.empty())
        topModuleLibImport = "-work " + topModuleLib + " ";
      if (ProjManager()->DesignTopModule().empty()) {
        fileList += "verific -import -all\n";
      } else {
        fileList += "verific " + topModuleLibImport + importLibs + "-import " +
                    ProjManager()->DesignTopModule() + "\n";
      }
      yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
      break;
    }
    case ParserType::Default: {
      std::string designFiles = YosysDesignParsingCommmands();
      yosysScript =
          ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", designFiles);
      break;
    }
    case ParserType::Surelog: {
      std::string fileList = SurelogDesignParsingCommmands();
      yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
      break;
    }
    case ParserType::GHDL: {
      std::string fileList = GhdlDesignParsingCommmands();
      yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
      break;
    }
    default:
      break;
  }
  if (!ProjManager()->DesignTopModule().empty()) {
    yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE_DIRECTIVE}",
                             "-top " + ProjManager()->DesignTopModule());
    yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}",
                             ProjManager()->DesignTopModule());
  } else {
    yosysScript =
        ReplaceAll(yosysScript, "${TOP_MODULE_DIRECTIVE}", "-auto-top");
  }

  yosysScript = FinishSynthesisScript(yosysScript);

  yosysScript = ReplaceAll(yosysScript, "${PIN_LOCATION_SDC}", sdcOut);

  yosysScript = ReplaceAll(yosysScript, "${CONFIG_JSON}", "config.json");

  // Simulation files
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_BLIF}",
      std::string(ProjManager()->projectName() + "_post_synth.blif"));
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_EBLIF}",
      std::string(ProjManager()->projectName() + "_post_synth.eblif"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_VERILOG}",
                 std::string(ProjManager()->projectName() + "_post_synth.v"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_VHDL}",
                 std::string(ProjManager()->projectName() + "_post_synth.vhd"));
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_EDIF}",
      std::string(ProjManager()->projectName() + "_post_synth.edif"));

  // Periphery wrapper files
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_WRAPPER_BLIF}",
                 std::string("wrapper_" + ProjManager()->projectName() +
                             "_post_synth.blif"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_WRAPPER_EBLIF}",
                 std::string("wrapper_" + ProjManager()->projectName() +
                             "_post_synth.eblif"));
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_WRAPPER_VERILOG}",
      std::string("wrapper_" + ProjManager()->projectName() + "_post_synth.v"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_WRAPPER_VHDL}",
                 std::string("wrapper_" + ProjManager()->projectName() +
                             "_post_synth.vhd"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_WRAPPER_EDIF}",
                 std::string("wrapper_" + ProjManager()->projectName() +
                             "_post_synth.edif"));

  // Fabric files
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_FABRIC_BLIF}",
                 std::string("fabric_" + ProjManager()->projectName() +
                             "_post_synth.blif"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_FABRIC_EBLIF}",
                 std::string("fabric_" + ProjManager()->projectName() +
                             "_post_synth.eblif"));
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_FABRIC_VERILOG}",
      std::string("fabric_" + ProjManager()->projectName() + "_post_synth.v"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_FABRIC_VHDL}",
                 std::string("fabric_" + ProjManager()->projectName() +
                             "_post_synth.vhd"));
  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_FABRIC_EDIF}",
                 std::string("fabric_" + ProjManager()->projectName() +
                             "_post_synth.edif"));

  std::string script_path = ProjManager()->projectName() + ".ys";
  std::string output_path;
  switch (GetNetlistType()) {
    case NetlistType::Verilog:
      output_path = ProjManager()->projectName() + "_post_synth.v";
      break;
    case NetlistType::VHDL:
      // Until we have a VHDL netlist reader in VPR
      output_path = ProjManager()->projectName() + "_post_synth.v";
      break;
    case NetlistType::Edif:
      output_path = ProjManager()->projectName() + "_post_synth.edif";
      break;
    case NetlistType::Blif:
      output_path = ProjManager()->projectName() + "_post_synth.blif";
      break;
    case NetlistType::EBlif:
      output_path = ProjManager()->projectName() + "_post_synth.eblif";
      break;
  }

  if (!DesignChanged(yosysScript, script_path, output_path)) {
    m_state = State::Synthesized;
    Message("Design didn't change: " + ProjManager()->projectName() +
            ", skipping synthesis.");
    return true;
  }
  std::filesystem::remove(
      std::string(ProjManager()->projectName() + "_post_synth.blif"));
  std::filesystem::remove(
      std::string(ProjManager()->projectName() + "_post_synth.eblif"));
  std::filesystem::remove(
      std::string(ProjManager()->projectName() + "_post_synth.v"));
  // Create Yosys command and execute
  FileUtils::WriteToFile(script_path, yosysScript, false);
  if (!FileUtils::FileExists(m_yosysExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
    return false;
  }
  std::string command =
      m_yosysExecutablePath.string() + " -s " +
      std::string(script_path + " -l " + ProjManager()->projectName() +
                  "_synth.log");
  Message("Synthesis command: " + command);
  int status = ExecuteAndMonitorSystemCommand(
      command, {}, false, FilePath(Action::Synthesis).string());
  if (status) {
    if (GetParserType() == ParserType::Default) {
      std::ifstream raptor_log(ProjManager()->projectName() + "_synth.log");
      if (raptor_log.good()) {
        std::stringstream buffer;
        buffer << raptor_log.rdbuf();
        const std::string& buf = buffer.str();
        raptor_log.close();
        if (buf.find("Executing HIERARCHY pass") == std::string::npos) {
          // If Default Yosys parser fails, attempt with the Surelog parser
          ErrorMessage("Default parser failed, re-attempting with SV parser");
          SetParserType(ParserType::Surelog);
          bool subResult = Synthesize();
          if (subResult) {
            status = 0;
          }
        }
      }
    }
  }
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " Synthesis failed");
    return false;
  } else {
    m_state = State::Synthesized;
    Message("Design " + ProjManager()->projectName() + " is synthesized");
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
    // Message("Keep name: " + keep);
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
  BaseVprDefaults defaults;
  return BaseVprCommand(defaults);
}

std::string CompilerOpenFPGA::BaseVprCommand(BaseVprDefaults defaults) {
  std::string device_size = "";
  if (PackOpt() == Compiler::PackingOpt::Debug) {
    device_size = " --device auto";
  } else if (!m_deviceSize.empty()) {
    device_size = " --device " + m_deviceSize;
  }
  std::string netlistFile;
  switch (GetNetlistType()) {
    case NetlistType::Verilog:
      netlistFile = ProjManager()->projectName() + "_post_synth.v";
      break;
    case NetlistType::VHDL:
      // Until we have a VHDL netlist reader in VPR
      netlistFile = ProjManager()->projectName() + "_post_synth.v";
      break;
    case NetlistType::Edif:
      netlistFile = ProjManager()->projectName() + "_post_synth.edif";
      break;
    case NetlistType::Blif:
      netlistFile = ProjManager()->projectName() + "_post_synth.blif";
      break;
    case NetlistType::EBlif:
      netlistFile = ProjManager()->projectName() + "_post_synth.eblif";
      break;
  }
  netlistFile = FilePath(Action::Synthesis, netlistFile).string();

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
  if (ClbPackingOption() == ClbPacking::Timing_driven) {
    pnrOptions += " --allow_unrelated_clustering off";
  } else {
    pnrOptions += " --allow_unrelated_clustering on";
  }
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
  auto sdcFile =
      FilePath(Action::Pack,
               "fabric_" + ProjManager()->projectName() + "_openfpga.sdc")
          .string();
  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile + std::string(" --sdc_file ") + sdcFile +
                  std::string(" --clock_modeling ideal --route_chan_width ") +
                  std::to_string(m_channel_width) + device_size + pnrOptions);

  fs::path netlistFileName{netlistFile};
  netlistFileName = netlistFileName.filename();
  auto name = netlistFileName.stem().string();
  if (m_flatRouting) {
    command += " --flat_routing true";
  }
  if (!m_routingGraphFile.empty()) {
    command += " --read_rr_graph " + m_routingGraphFile.string();
  }
  command += " --net_file " + FilePath(Action::Pack, name + ".net").string();
  command +=
      " --place_file " + FilePath(Action::Placement, name + ".place").string();
  command +=
      " --route_file " + FilePath(Action::Routing, name + ".route").string();
  processCustomLayout();
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
  const std::string openStaFile = ProjManager()->projectName() + "_opensta.tcl";
  FileUtils::WriteToFile(openStaFile, script);
  return openStaFile;
}

bool CompilerOpenFPGA::WriteTimingConstraints() {
  // Read config.json dumped during synthesis stage by design edit plugin
  std::filesystem::path configJsonPath =
      FilePath(Action::Synthesis) / "config.json";
  std::filesystem::path fabricJsonPath =
      FilePath(Compiler::Action::Synthesis) / "fabric_netlist_info.json";
  getNetlistEditData()->ReadData(configJsonPath, fabricJsonPath);

  // update constraints
  const auto& constrFiles = ProjManager()->getConstrFiles();
  m_constraints->reset();
  for (const auto& file : constrFiles) {
    int res{TCL_OK};
    auto status =
        m_interp->evalCmd(std::string("read_sdc {" + file + "}").c_str(), &res);
    if (res != TCL_OK) {
      ErrorMessage(status);
      return false;
    }
  }

  const std::string sdcOut =
      "fabric_" + ProjManager()->projectName() + "_openfpga.sdc";
  std::ofstream ofssdc(sdcOut);
  // TODO: Massage the SDC so VPR can understand them
  for (auto constraint : m_constraints->getConstraints()) {
    // Parse RTL and expand the get_ports, get_nets
    // Temporary dirty filtering:
    constraint = ReplaceAll(constraint, "@", "[");
    constraint = ReplaceAll(constraint, "%", "]");
    Message("Constraint: " + constraint);
    std::vector<std::string> tokens;
    StringUtils::tokenize(constraint, " ", tokens);
    constraint = "";
    for (uint32_t i = 0; i < tokens.size(); i++) {
      const std::string& tok = tokens[i];
      tokens[i] = getNetlistEditData()->PIO2InnerNet(tok);
      tokens[i] = m_constraints->SafeParens(tokens[i]);
    }

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
    if (constraint.find("set_property") != std::string::npos) {
      continue;
    }
    if (constraint.find("set_clock_pin") != std::string::npos) {
      continue;
    }
    ofssdc << constraint << "\n";
  }
  ofssdc.close();
  return true;
}

bool CompilerOpenFPGA::Packing() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", PACKING_LOG);
  });

  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    CleanFiles(Action::Pack);
    return true;
  }
  if (!HasTargetDevice()) return false;
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
  PERF_LOG("Packing has started");
  Message("##################################################");
  Message("Packing for design: " + ProjManager()->projectName());
  Message("##################################################");
  if (ProjManager()->projectType() == ProjectType::GateLevel) {
    // update constraints
    const auto& constrFiles = ProjManager()->getConstrFiles();
    m_constraints->reset();
    for (const auto& file : constrFiles) {
      int res{TCL_OK};
      auto status = m_interp->evalCmd(
          std::string("read_sdc {" + file + "}").c_str(), &res);
      if (res != TCL_OK) {
        ErrorMessage(status);
        return false;
      }
    }
  }

  if (!WriteTimingConstraints()) {
    return false;
  }

  auto prevOpt = PackOpt();
  PackOpt(PackingOpt::None);

  std::string command = BaseVprCommand({}) + " --pack";
  auto file = ProjManager()->projectName() + "_pack.cmd";
  FileUtils::WriteToFile(file, command);

  fs::path netlistPath = GetNetlistPath();
  netlistPath = netlistPath.filename();
  if (FileUtils::IsUptoDate(GetNetlistPath(),
                            netlistPath.stem().string() + ".net") &&
      (prevOpt != PackingOpt::Debug)) {
    m_state = State::Packed;
    Message("Design " + ProjManager()->projectName() + " packing reused");
    return true;
  }

  PackOpt(prevOpt);
  auto workingDir = FilePath(Action::Pack);
  int status = ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " packing failed");
    if (PackOpt() == PackingOpt::Debug) {
      std::string command = BaseVprCommand({}) + " --pack";
      FileUtils::WriteToFile(file, command);
      ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
    }
    return false;
  }
  m_state = State::Packed;
  Message("Design " + ProjManager()->projectName() + " is packed");
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
  Message("##################################################");
  Message("Global Placement for design: " + ProjManager()->projectName());
  Message("##################################################");
  // TODO:
  m_state = State::GloballyPlaced;
  Message("Design " + ProjManager()->projectName() + " is globally placed");
  return true;
}

bool CompilerOpenFPGA::Placement() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", PLACEMENT_LOG);
  });

  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (PlaceOpt() == PlacementOpt::Clean) {
    Message("Cleaning placement results for " + ProjManager()->projectName());
    m_state = State::GloballyPlaced;
    PlaceOpt(PlacementOpt::None);
    CleanFiles(Action::Placement);
    return true;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;

  PERF_LOG("Placement has started");
  Message("##################################################");
  Message("Placement for design: " + ProjManager()->projectName());
  Message("##################################################");
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  const std::string pcfOut = ProjManager()->projectName() + "_openfpga.pcf";
  std::string previousConstraints;
  std::ifstream ifspcf(pcfOut);
  if (ifspcf.good()) {
    std::stringstream buffer;
    buffer << ifspcf.rdbuf();
    previousConstraints = buffer.str();
  }
  ifspcf.close();

  bool userConstraint = false;
  bool repackConstraint = false;
  std::vector<std::string> constraints;
  std::vector<std::string> set_clks;
#if 1
  std::filesystem::path design_edit_sdc =
      FilePath(Action::Synthesis) / "design_edit.sdc";
  // Read the INI before the m_constraints is reset
  nlohmann::json properties = m_constraints->get_simplified_property_json();
  if (properties.contains("INI")) {
    if (properties["INI"].contains("OVERWRITE_PIN_LOCATION_AND_MODE_FILE")) {
      std::string manual_pin_file =
          properties["INI"]["OVERWRITE_PIN_LOCATION_AND_MODE_FILE"];
      if (!std::filesystem::exists(manual_pin_file)) {
        std::filesystem::path pp = ProjManager()->projectPath();
        std::filesystem::path mp =
            std::filesystem::absolute(pp / ".." / manual_pin_file);
        if (std::filesystem::exists(mp)) {
          manual_pin_file = mp.string();
        } else {
          mp = std::filesystem::absolute(pp / manual_pin_file);
          if (std::filesystem::exists(mp)) {
            manual_pin_file = mp.string();
          } else {
            manual_pin_file = "";
          }
        }
      }
      if (manual_pin_file.size()) {
        Message("Overwrite pin location and mode: " + manual_pin_file);
        design_edit_sdc = manual_pin_file;
      }
    }
  }
  if (std::filesystem::exists(design_edit_sdc)) {
    std::ifstream sdc_text(design_edit_sdc.c_str());
    if (sdc_text.is_open()) {
      std::string line = "";
      while (getline(sdc_text, line)) {
        CFG_get_rid_whitespace(line);
        line = CFG_replace_string(line, "\t", " ");
        line = CFG_replace_string(line, "  ", " ");
        line = CFG_replace_string(line, "{", "");
        line = CFG_replace_string(line, "}", "");
        if (line.size() > 0 && line.find("#") != 0) {
          if (line.find("set_clock_pin ") == 0) {
            set_clks.push_back(line);
            repackConstraint = true;
            // so there is a diff if changed
            constraints.push_back("# " + line);
          } else if (line.find("set_pin_loc ") == 0) {
            line = CFG_replace_string(line, "set_pin_loc", "set_io");
            userConstraint = true;
            constraints.push_back(line);
          } else if (line.find("set_property mode ") == 0) {
            line = CFG_replace_string(line, "set_property mode", "set_mode");
            userConstraint = true;
            constraints.push_back(line);
          } else if (line.find("set_mode ") == 0 || line.find("set_io ") == 0) {
            userConstraint = true;
            constraints.push_back(line);
          }
        }
      }
    } else {
      ErrorMessage("Fail to read SDC file: " + design_edit_sdc.string());
      return false;
    }
    sdc_text.close();
  } else {
#else
  {
#endif
    for (auto constraint : m_constraints->getConstraints()) {
      constraint = ReplaceAll(constraint, "@", "[");
      constraint = ReplaceAll(constraint, "%", "]");
      // pin location constraints have to be translated to .place:
      if ((constraint.find("set_pin_loc") != std::string::npos)) {
        userConstraint = true;
        constraint = ReplaceAll(constraint, "set_pin_loc", "set_io");
        constraints.push_back(constraint);
      } else if (constraint.find("set_mode") != std::string::npos) {
        constraints.push_back(constraint);
        userConstraint = true;
      } else if ((constraint.find("set_property") != std::string::npos) &&
                 (constraint.find(" mode ") != std::string::npos)) {
        constraint = ReplaceAll(constraint, " mode ", " ");
        constraint = ReplaceAll(constraint, "set_property", "set_mode");
        constraints.push_back(constraint);
        userConstraint = true;
      } else if (constraint.find("set_clock_pin") != std::string::npos) {
        set_clks.push_back(constraint);
        repackConstraint = true;
        constraints.push_back("# " +
                              constraint);  // so there is a diff if changed
      } else {
        continue;
      }
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

  fs::path netlistPath = GetNetlistPath();
  auto netlistFileName = netlistPath.filename().stem().string();
  if ((previousConstraints == newConstraints) &&
      FileUtils::IsUptoDate(
          FilePath(Action::Pack, netlistFileName + ".net").string(),
          FilePath(Action::Placement, netlistFileName + ".place").string())) {
    m_state = State::Placed;
    Message("Design " + ProjManager()->projectName() + " placement reused");
    return true;
  }

  std::string netlistFile = "fabric_";
  netlistFile += ProjManager()->projectName() + "_post_synth.eblif";
  netlistFile = FilePath(Action::Synthesis, netlistFile).string();

  bool netlistInput = false;
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistInput = true;
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

  std::string command = BaseVprCommand({}) + " --place";
  std::string pincommand = m_pinConvExecutablePath.string();
  if ((PinAssignOpts() != PinAssignOpt::Pin_constraint_disabled) &&
      FileUtils::FileExists(pincommand) && (!m_PinMapCSV.empty())) {
    if (!std::filesystem::is_regular_file(m_PinMapCSV)) {
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
    pincommand += " --csv " + m_PinMapCSV.string();

    if (userConstraint) {
      pincommand += " --pcf " +
                    std::string(ProjManager()->projectName() + "_openfpga.pcf");
    }

    if (GetNetlistType() == NetlistType::Edif) {
      pincommand += " --port_info ";
      pincommand += FilePath(Action::Pack, "post_synth_ports.json").string();
    } else if (netlistInput == true) {
      pincommand += " --blif " + GetNetlistPath();
    } else {
      pincommand += " --blif " + netlistFile;
    }

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
    } else if (PinAssignOpts() == PinAssignOpt::Pin_constraint_disabled) {
      pincommand += " free";
    } else {  // default behavior
      pincommand += " in_define_order";
    }

    // user want to map its design clocks to fabric
    // clocks. Example clocks clk[0],clk[1]....,clk[15]. And user
    // clocks are clk_a,clk_b and want to map clk_a with clk[15] like it in such
    // case, we need to make sure a xml repack constraint file is properly
    // generated to guide bitstream generation correctly.

    std::string repack_constraints =
        ProjManager()->projectName() + "_repack_constraints.xml";

    if (!set_clks.empty() && repackConstraint) {
      const std::string repack_out =
          ProjManager()->projectName() + ".temp_file_clkmap";
      std::ofstream ofsclkmap(repack_out);

      for (auto constraint : set_clks) {
        ofsclkmap << constraint << "\n";
      }
      ofspcf.close();
      pincommand += " --clk_map " + std::string(ProjManager()->projectName() +
                                                ".temp_file_clkmap");
      pincommand +=
          " --read_repack " + m_OpenFpgaRepackConstraintsFile.string();
      pincommand += " --write_repack " + repack_constraints;
    }

    // pass config.json dumped during synthesis stage by design edit plugin
    std::filesystem::path configJsonPath =
        FilePath(Action::Synthesis) / "config.json";
    if (FileUtils::FileExists(configJsonPath)) {
      pincommand += " --edits " + configJsonPath.string();
    }

    std::string pin_loc_constraint_file;

    auto file = ProjManager()->projectName() + "_pin_loc.cmd";
    FileUtils::WriteToFile(file, pincommand);

    auto workingDir = FilePath(Action::Placement);
    int status =
        ExecuteAndMonitorSystemCommand(pincommand, {}, false, workingDir);

    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " pin conversion failed");
      return false;
    } else {
      pin_loc_constraint_file = pin_locFile;
    }

    if ((PinAssignOpts() != PinAssignOpt::Pin_constraint_disabled) &&
        PinConstraintEnabled() && (!pin_loc_constraint_file.empty())) {
      command += " --fix_clusters " + pin_loc_constraint_file;
    }
  }

  auto file = ProjManager()->projectName() + "_place.cmd";
  FileUtils::WriteToFile(file, command);
  auto workingDir = FilePath(Action::Placement);
  int status = ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " placement failed");
    return false;
  }
  m_state = State::Placed;
  Message("Design " + ProjManager()->projectName() + " is placed");
  return true;
}

static std::string find_gbox_mode(const std::vector<std::string>* A, int i,
                                  const std::string& gbox) noexcept {
  for (; i >= 0; i--) {
    const std::vector<std::string>& line = A[i];
    if (line.size() < 3 or line.front() != "set_mode") continue;
    if (line[2] == gbox) return line[1];
  }
  return {};
}

bool CompilerOpenFPGA::ConvertSdcPinConstrainToPcf(
    std::vector<std::string>& constraints) {
  using std::string;
  using std::vector;

  size_t in_sz = constraints.size();
  if (in_sz == 0) return true;

  constexpr bool trace = false;
  if (trace) {
    ::puts("\n");
    ::printf(" TRACE-IN compilerOpenFPGA: vec<str> constraints (%zu) :\n\n",
             in_sz);
    for (size_t i = 0; i < in_sz; i++) {
      const string& ss = constraints[i];
      ::printf("\t |%zu|  %s\n", i, ss.c_str());
    }
    ::puts("--- end trace-in -\n");
  }

  vector<string> constraint_and_mode;
  vector<vector<string>> tokenized(in_sz);
  string constraint_with_mode;

  // tokenize all lines
  for (size_t i = 0; i < in_sz; i++)
    StringUtils::tokenize(constraints[i], " ", tokenized[i]);

  // error-check
  for (size_t i = 0; i < in_sz; i++) {
    if (constraints[i].find("set_mode") == string::npos) continue;
    const vector<string>& tokens = tokenized[i];
    if (tokens.size() != 3) {
      ErrorMessage("Invalid set_mode command: <" + constraints[i] + ">");
      return false;
    }
  }

  for (size_t i = 0; i < in_sz; i++) {
    if (constraints[i].find("set_io") == string::npos) continue;

    const vector<string>& tokens = tokenized[i];
    if (!(tokens.size() == 3 || tokens.size() == 4 ||
          (tokens.size() == 7 && tokens[0] == "set_io" &&
           tokens[3] == "-mode" && tokens[5] == "-internal_pin"))) {
      ErrorMessage("Invalid set_pin_loc command: <" + constraints[i] + ">");
      return false;
    }
    if (tokens.size() == 7) {
      constraint_with_mode = constraints[i];
    } else {
      constraint_with_mode = tokens[0];
      constraint_with_mode.push_back(' ');
      constraint_with_mode += tokens[1];
      constraint_with_mode.push_back(' ');
      constraint_with_mode += tokens[2];

      string mod = find_gbox_mode(tokenized.data(), i, tokens[2]);
      if (mod.empty()) mod = "Mode_GPIO";

      constraint_with_mode += " -mode ";
      constraint_with_mode += mod;

      if (tokens.size() == 4) {
        constraint_with_mode += string(" -internal_pin ") + tokens[3];
      }
    }
    constraint_and_mode.push_back(constraint_with_mode);
  }

  size_t out_sz = constraint_and_mode.size();
  constraints.clear();
  for (size_t i = 0; i < out_sz; i++) {
    constraints.push_back(constraint_and_mode[i]);
  }

  if (trace) {
    ::printf(" TRACE-OUT compilerOpenFPGA: PCF output (%zu)\n\n", out_sz);
    for (size_t i = 0; i < out_sz; i++) {
      const string& ss = constraint_and_mode[i];
      ::printf("\t |%zu|  %s\n", i, ss.c_str());
    }
    ::puts("\n");
  }

  return true;
}

bool CompilerOpenFPGA::Route() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  ProcessUtilization routingUtilization{};
  auto guard = sg::make_scope_guard([this, &routingUtilization] {
    // Rename log file
    m_utils = routingUtilization;
    copyLog(ProjManager(), "vpr_stdout.log", ROUTING_LOG);
    RenamePostSynthesisFiles(Action::Routing);
  });

  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (RouteOpt() == RoutingOpt::Clean) {
    Message("Cleaning routing results for " + ProjManager()->projectName());
    m_state = State::Placed;
    RouteOpt(RoutingOpt::None);
    CleanFiles(Action::Routing);
    return true;
  }
  if (m_state != State::Placed && m_state != State::Routed) {
    ErrorMessage("Design needs to be in placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;
  PERF_LOG("Route has started");
  Message("##################################################");
  Message("Routing for design: " + ProjManager()->projectName());
  Message("##################################################");
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  fs::path netlistPath = GetNetlistPath();
  auto netlistFileName = netlistPath.filename().stem().string();
  if (FileUtils::IsUptoDate(
          FilePath(Action::Placement, netlistFileName + ".place").string(),
          FilePath(Action::Routing, netlistFileName + ".route").string())) {
    m_state = State::Routed;
    Message("Design " + ProjManager()->projectName() + " routing reused");
    return true;
  }

  auto routingPath = FilePath(Action::Routing);
  std::string command = BaseVprCommand({}) + " --route";
  FileUtils::WriteToFile(
      routingPath / std::string(ProjManager()->projectName() + "_route.cmd"),
      command);
  routingUtilization = m_utils;
  int status = ExecuteAndMonitorSystemCommand(command, {}, false, routingPath);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " routing failed");
    return false;
  }

  // call finalize
  if (!FileUtils::FileExists(m_ReConstructVExecPath)) {
    ErrorMessage("Cannot find executable: " + m_ReConstructVExecPath.string());
    return false;
  }

  auto postRouteVfile =
      FileUtils::FindFileByExtension(routingPath, ".v").filename().string();
  std::filesystem::path ReConstructInFile = routingPath / postRouteVfile;
  std::filesystem::path ReConstructOutFile =
      routingPath / std::string(postRouteVfile + "_");
  std::string reconstruct_cmd = m_ReConstructVExecPath.string() + "  " +
                                ReConstructInFile.string() + " " +
                                ReConstructOutFile.string();
  int status_ =
      ExecuteAndMonitorSystemCommand(reconstruct_cmd, {}, false, routingPath);
  if (status_) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " post routing finalize stage 1/3 failed");
    return false;
  }
  if (!FileUtils::removeFile(ReConstructInFile)) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " post routing finalize stage 2/3 failed");
    return false;
  }
  if (!FileUtils::RenameFile(ReConstructOutFile, ReConstructInFile)) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " post routing finalize stage 3/3 failed");
    return false;
  }

  m_state = State::Routed;
  Message("Design " + ProjManager()->projectName() + " is routed");
  return true;
}

bool CompilerOpenFPGA::TimingAnalysis() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", TIMING_ANALYSIS_LOG);
    RenamePostSynthesisFiles(Action::STA);
  });

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
    CleanFiles(Action::STA);
    return true;
  }

  PERF_LOG("TimingAnalysis has started");
  Message("##################################################");
  Message("Timing Analysis for design: " + ProjManager()->projectName());
  Message("##################################################");
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  auto workingDir = FilePath(Action::STA).string();
  if (TimingAnalysisOpt() == STAOpt::View) {
    TimingAnalysisOpt(STAOpt::None);
    const std::string command = BaseVprCommand({}) + " --analysis --disp on";
    const int status =
        ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " place and route view failed");
      return false;
    }
    return true;
  }

  fs::path netlistPath = GetNetlistPath();
  auto netlistFileName = netlistPath.filename().stem().string();
  if (FileUtils::IsUptoDate(
          FilePath(Action::Routing, netlistFileName + ".route").string(),
          FilePath(Action::STA, ProjManager()->projectName() + "_sta.cmd")
              .string())) {
    Message("Design " + ProjManager()->projectName() + " timing didn't change");
    return true;
  }
  int status = 0;
  std::string taCommand;
  // use OpenSTA to do the job
  if (TimingAnalysisEngineOpt() == STAEngineOpt::Opensta) {
    // allows SDF to be generated for OpenSTA
    std::string command =
        BaseVprCommand({}) + " --gen_post_synthesis_netlist on";
    auto file = std::string(ProjManager()->projectName() + "_sta.cmd");
    FileUtils::WriteToFile(file, command);
    int status = ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " timing analysis failed");
      return false;
    }
    RenamePostSynthesisFiles(Action::STA);
    // find files
    auto projName = ProjManager()->projectName();
    auto libFileName = FilePath(Action::STA, projName + ".lib");
    auto netlistFileName = FilePath(Action::STA, projName + "_post_route.v");
    auto sdfFileName = FilePath(Action::STA, projName + "_post_route.sdf");
    auto sdcFileName = FilePath(Action::STA, "fabric_" + projName + ".sdc");
    if (std::filesystem::is_regular_file(libFileName) &&
        std::filesystem::is_regular_file(netlistFileName) &&
        std::filesystem::is_regular_file(sdfFileName) &&
        std::filesystem::is_regular_file(sdcFileName)) {
      taCommand = BaseStaCommand() + " " +
                  BaseStaScript(libFileName.string(), netlistFileName.string(),
                                sdfFileName.string(), sdcFileName.string());
      auto file = std::string(ProjManager()->projectName() + "_sta.cmd");
      FileUtils::WriteToFile(file, taCommand);
    } else {
      auto fileList =
          StringUtils::join({libFileName.string(), netlistFileName.string(),
                             sdfFileName.string(), sdcFileName.string()},
                            "\n");
      ErrorMessage(
          "No required design info generated for user design, required for "
          "timing analysis:\n" +
          fileList);
      return false;
    }
  } else {  // use vpr/tatum engine
    taCommand = BaseVprCommand({}) + " --analysis";
    auto file = std::string(ProjManager()->projectName() + "_sta.cmd");
    FileUtils::WriteToFile(file, taCommand + " --disp on");
  }

  status = ExecuteAndMonitorSystemCommand(taCommand, {}, false, workingDir);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " timing analysis failed");
    return false;
  }

  Message("Design " + ProjManager()->projectName() + " is timing analysed");
  return true;
}

bool CompilerOpenFPGA::PowerAnalysis() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", POWER_ANALYSIS_LOG);
  });

  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (!HasTargetDevice()) return false;

  if (PowerAnalysisOpt() == PowerOpt::Clean) {
    Message("Cleaning PowerAnalysis results for " +
            ProjManager()->projectName());
    PowerAnalysisOpt(PowerOpt::None);
    m_state = State::Routed;
    CleanFiles(Action::Power);
    return true;
  }

  PERF_LOG("PowerAnalysis has started");
  Message("##################################################");
  Message("Power Analysis for design: " + ProjManager()->projectName());
  Message("##################################################");

  if (FileUtils::IsUptoDate(
          FilePath(Action::Routing,
                   ProjManager()->projectName() + "_post_synth.route")
              .string(),
          FilePath(Action::STA, ProjManager()->projectName() + "_sta.cmd")
              .string())) {
    Message("Design " + ProjManager()->projectName() + " power didn't change");
    return true;
  }

  std::string command = BaseVprCommand({}) + " --analysis";
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }

  auto file = ProjManager()->projectName() + "_power.cmd";
  FileUtils::WriteToFile(file, command);

  int status = ExecuteAndMonitorSystemCommand(command, {}, false,
                                              FilePath(Action::Power).string());
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " power analysis failed");
    return false;
  }

  Message("Design " + ProjManager()->projectName() + " is power analysed");
  return true;
}

const std::string basicOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --skip_sync_clustering_and_routing_results ${VPR_PB_PIN_FIXUP} --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT} --analysis ${PNR_OPTIONS} ${TOP_MODULE_INFO}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation settings
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

${PB_PIN_FIXUP}

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --compress_routing ${OPENFPGA_BUILD_FABRIC_OPTION}

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream ${BUILD_ARCHITECTURE_BITSTREAM_OPTIONS}

build_fabric_bitstream
write_fabric_bitstream ${DONT_CARE_BITS_OPTION} ${WL_ORDER_OPTION} --format plain_text --file fabric_bitstream.bit
${WRITE_FABRIC_BITSTREAM_XML}
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

const std::string simulationOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --skip_sync_clustering_and_routing_results ${VPR_PB_PIN_FIXUP} --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT} --analysis ${PNR_OPTIONS} ${TOP_MODULE_INFO}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation settings
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

${PB_PIN_FIXUP}

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --compress_routing ${OPENFPGA_BUILD_FABRIC_OPTION}

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream --verbose \
                             --write_file fabric_independent_bitstream.xml

build_fabric_bitstream

write_fabric_verilog --file BIT_SIM \
                     --explicit_port_mapping \
                     --default_net_type wire \
                     --no_time_stamp \
                     --use_relative_path \
                     --verbose

write_fabric_bitstream --format plain_text --file fabric_bitstream.bit

write_fabric_bitstream --format xml --file fabric_bitstream.xml

write_full_testbench --file BIT_SIM \
                     --bitstream fabric_bitstream.bit --pin_constraints_file ${OPENFPGA_PIN_CONSTRAINTS}

write_preconfigured_fabric_wrapper --file BIT_SIM --embed_bitstream iverilog --pin_constraints_file ${OPENFPGA_PIN_CONSTRAINTS}

write_preconfigured_testbench --file BIT_SIM --pin_constraints_file ${OPENFPGA_PIN_CONSTRAINTS}

write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

std::string CompilerOpenFPGA::InitOpenFPGAScript() {
  // Default, Simulation enabled or custom OpenFPGA script
  if (m_openFPGAScript.empty()) {
    if (BitsFlags() == BitstreamFlags::EnableSimulation) {
#ifdef PRODUCTION_BUILD
      ErrorMessage("Cannot generate bitstream netlist in production build");
      m_openFPGAScript = basicOpenFPGABitstreamScript;
#else
      m_openFPGAScript = simulationOpenFPGABitstreamScript;
#endif
    } else {
      m_openFPGAScript = basicOpenFPGABitstreamScript;
    }
  }
  return m_openFPGAScript;
}

std::string CompilerOpenFPGA::FinishOpenFPGAScript(const std::string& script) {
  std::string result = script;

  std::string netlistFilePrefix = ProjManager()->projectName() + "_post_synth";

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
        break;
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
  result = ReplaceAll(
      result, "${NET_FILE}",
      FilePath(Action::Pack, "fabric_" + netlistFilePrefix + ".net").string());
  result = ReplaceAll(
      result, "${PLACE_FILE}",
      FilePath(Action::Placement, "fabric_" + netlistFilePrefix + ".place")
          .string());
  result = ReplaceAll(
      result, "${ROUTE_FILE}",
      FilePath(Action::Routing, "fabric_" + netlistFilePrefix + ".route")
          .string());
  result = ReplaceAll(
      result, "${SDC_FILE}",
      FilePath(Action::Pack,
               "fabric_" + ProjManager()->projectName() + "_openfpga.sdc")
          .string());
  if (!ProjManager()->DesignTopModule().empty())
    result = ReplaceAll(result, "${TOP_MODULE_INFO}",
                        "--top " + ProjManager()->DesignTopModule());
  else
    result = ReplaceAll(result, "${TOP_MODULE_INFO}", "");
  std::string pnrOptions;
  if (ClbPackingOption() == ClbPacking::Timing_driven) {
    pnrOptions += " --allow_unrelated_clustering off";
  } else {
    pnrOptions += " --allow_unrelated_clustering on";
  }
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
  result = ReplaceAll(result, "${PNR_OPTIONS}", pnrOptions);
  const std::string netlistFile = GetNetlistPath();
  result = ReplaceAll(result, "${VPR_TESTBENCH_BLIF}", netlistFile);

  std::string netlistFormat;
  switch (GetNetlistType()) {
    case NetlistType::Verilog:
      netlistFormat = "eblif";
      break;
    case NetlistType::VHDL:
      netlistFormat = "eblif";
      break;
    case NetlistType::Edif:
      netlistFormat = "edif";
      break;
    case NetlistType::Blif:
      netlistFormat = "blif";
      break;
    case NetlistType::EBlif:
      netlistFormat = "eblif";
      break;
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
  // VPR an OpenFPGA pb_pin_fixup settings are mutually exclusive
  if (m_bitstreamMoreOpt.find("pb_pin_fixup") != std::string::npos)
    m_pb_pin_fixup = "pb_pin_fixup";
  result = ReplaceAll(result, "${PB_PIN_FIXUP}", m_pb_pin_fixup);
  if (m_pb_pin_fixup == "pb_pin_fixup") {
    // Skip
    result = ReplaceAll(result, "${VPR_PB_PIN_FIXUP}", "on");
  } else {
    // Don't skip
    result = ReplaceAll(result, "${VPR_PB_PIN_FIXUP}", "off");
  }
  if (m_runtime_OpenFpgaBitstreamSettingFile.string().empty()) {
    result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}", "");
  } else {
    result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                        "read_openfpga_bitstream_setting -f " +
                            m_runtime_OpenFpgaBitstreamSettingFile.string());
  }
  result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                      m_runtime_OpenFpgaBitstreamSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_PIN_CONSTRAINTS}",
                      m_OpenFpgaPinConstraintXml.string());

  if (m_bitstreamMoreOpt.find("wl_decremental_order") != std::string::npos) {
    result = ReplaceAll(result, "${WL_ORDER_OPTION}", "--wl_decremental_order");
  } else {
    result = ReplaceAll(result, "${WL_ORDER_OPTION}", "");
  }
  if (m_bitstreamMoreOpt.find("ignore_dont_care_bits") != std::string::npos) {
    result = ReplaceAll(result, "${DONT_CARE_BITS_OPTION}", "");
  } else {
    result =
        ReplaceAll(result, "${DONT_CARE_BITS_OPTION}", "--keep_dont_care_bits");
  }

  std::string repack_constraints =
      FilePath(Action::Placement,
               ProjManager()->projectName() + "_repack_constraints.xml")
          .string();
  const bool fpga_repack = FileUtils::FileExists(repack_constraints);
  if (!fpga_repack) {
    result = ReplaceAll(result, "${OPENFPGA_REPACK_CONSTRAINTS}",
                        m_OpenFpgaRepackConstraintsFile.string());
  } else {
    result = ReplaceAll(result, "${OPENFPGA_REPACK_CONSTRAINTS}",
                        repack_constraints);
  }
  if (m_OpenFpgaFabricKeyFile == "") {
    result = ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}", "");
  } else {
    result =
        ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}",
                   "--load_fabric_key " + m_OpenFpgaFabricKeyFile.string());
  }
  if (m_bitstreamMoreOpt.find("write_fabric_independent") != std::string::npos)
    result = ReplaceAll(result, "${BUILD_ARCHITECTURE_BITSTREAM_OPTIONS}",
                        "--write_file fabric_independent_bitstream.xml");
  else
    result = ReplaceAll(result, "${BUILD_ARCHITECTURE_BITSTREAM_OPTIONS}", "");
  if (m_bitstreamMoreOpt.find("write_xml") != std::string::npos)
    result = ReplaceAll(
        result, "${WRITE_FABRIC_BITSTREAM_XML}",
        "write_fabric_bitstream --format xml --file fabric_bitstream.xml");
  else
    result = ReplaceAll(result, "${WRITE_FABRIC_BITSTREAM_XML}", "");

  return result;
}

bool CompilerOpenFPGA::GenerateBitstream() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", BITSTREAM_LOG);
    m_compile2bits = false;
  });

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
    BitsOpt(BitstreamOpt::None);
    CleanFiles(Action::Bitstream);
    return true;
  }
  if (!ProjManager()->getTargetDevice().empty()) {
    if (!LicenseDevice(BaseDeviceName())) {
      ErrorMessage(
          "Device is not licensed: " + ProjManager()->getTargetDevice() + "\n");
      return false;
    }
  }
  PERF_LOG("GenerateBitstream has started");
  Message("##################################################");
  Message("Bitstream generation for design \"" + ProjManager()->projectName() +
          "\" on device \"" + ProjManager()->getTargetDevice() + "\"");
  Message("##################################################");
  if ((m_state != State::Routed) && (m_state != State::BistreamGenerated) &&
      !m_compile2bits) {
    ErrorMessage("Design needs to be in routed state");
    return false;
  }

  if (BitsFlags() == BitstreamFlags::EnableSimulation) {
    std::filesystem::path bit_path = "BIT_SIM";
    std::filesystem::create_directory(bit_path);
  }

  fs::path netlistPath = GetNetlistPath();
  auto netlistFileName = netlistPath.filename().stem().string();
  if (FileUtils::IsUptoDate(
          FilePath(Action::Routing, netlistFileName + ".route").string(),
          FilePath(Action::Bitstream, "fabric_bitstream.bit").string())) {
    Message("Design " + ProjManager()->projectName() +
            " bitstream didn't change");
    m_state = State::BistreamGenerated;
    return true;
  }

  if (BitsFlags() == BitstreamFlags::DefaultBitsOpt) {
#ifdef PRODUCTION_BUILD
    if (BitstreamEnabled() == false) {
      Message("Device " + ProjManager()->getTargetDevice() +
              " bitstream is not enabled, skipping");
      return true;
    }
#endif
  } else if (BitsFlags() == BitstreamFlags::Force) {
    // Force bitstream generation
  }

  // Before generating fabric bitstream script, determine the runtime bitstream
  // setting file which might include the runtime design IO tile clock out
  std::pair<bool, std::string> io_status = IsDeviceSizeCorrect(m_deviceSize);
  if (io_status.first) {
    std::filesystem::path design_edit_sdc =
        FilePath(Action::Synthesis, "design_edit.sdc");
    if (std::filesystem::exists(design_edit_sdc)) {
      std::string command = CFG_print(
          "model_config gen_bitstream_setting_xml -device_size %s -design %s "
          "-pin %s "
          "%s bitstream_setting.xml",
          io_status.second.c_str(), design_edit_sdc.c_str(),
          m_PinMapCSV.c_str(), m_OpenFpgaBitstreamSettingFile.c_str());
      auto file =
          ProjManager()->projectName() + "_gen_bitstream_setting_xml_cmd.tcl";
      FileUtils::WriteToFile(file, command);
      command = CFG_print("source %s", file.c_str());
      int status = TCL_OK;
      m_interp->evalCmd(command, &status);
      if (status != TCL_OK) {
        ErrorMessage("Design " + ProjManager()->projectName() +
                     " Bitstream Setting XML generation failed");
        return false;
      } else {
        m_runtime_OpenFpgaBitstreamSettingFile = "bitstream_setting.xml";
      }
    } else {
      m_runtime_OpenFpgaBitstreamSettingFile = m_OpenFpgaBitstreamSettingFile;
    }
  } else {
    m_runtime_OpenFpgaBitstreamSettingFile = m_OpenFpgaBitstreamSettingFile;
  }

  std::string command = m_openFpgaExecutablePath.string() + " -batch -f " +
                        ProjManager()->projectName() + ".openfpga";

  std::string script = InitOpenFPGAScript();

  if (m_compile2bits) {
    script = StringUtils::replaceAll(script, "--net_file ${NET_FILE}", {});
    script = StringUtils::replaceAll(script, "--place_file ${PLACE_FILE}", {});
    script = StringUtils::replaceAll(script, "--route_file ${ROUTE_FILE}", {});
    script = StringUtils::replaceAll(script, "--sdc_file ${SDC_FILE}", {});
  }

  script = FinishOpenFPGAScript(script);

  if (m_compile2bits) {
    script = StringUtils::replaceAll(script, "--analysis", {});
  }

  std::string script_path = ProjManager()->projectName() + ".openfpga";

  std::filesystem::remove("fabric_bitstream.bit");
  std::filesystem::remove("fabric_independent_bitstream.xml");
  // Create OpenFpga command and execute
  FileUtils::WriteToFile(script_path, script, false);
  if (!FileUtils::FileExists(m_openFpgaExecutablePath)) {
    ErrorMessage("Cannot find executable: " +
                 m_openFpgaExecutablePath.string());
    return false;
  }

  auto file = ProjManager()->projectName() + "_bitstream.cmd";
  FileUtils::WriteToFile(file, command);
  auto workingDir = FilePath(Action::Bitstream);
  int status = ExecuteAndMonitorSystemCommand(command, {}, false, workingDir);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " bitstream generation failed");
    return false;
  }

  // Generate PrimaryPinMapping.xml from PinMapping.xml (With real primary i/o
  // name, post netlist editing)
  std::filesystem::path pinmapFile =
      FilePath(Action::Bitstream, "PinMapping.xml").string();
  if (FileUtils::FileExists(pinmapFile)) {
    std::ifstream ifs(pinmapFile);
    if (ifs.good()) {
      std::stringstream buffer;
      buffer << ifs.rdbuf();
      ifs.close();
      std::string contents = buffer.str();
      for (auto pair : getNetlistEditData()->getReversePrimaryInputMap()) {
        contents = StringUtils::replaceAll(contents, pair.first, pair.second);
      }
      for (auto pair : getNetlistEditData()->getReversePrimaryOutputMap()) {
        contents = StringUtils::replaceAll(contents, pair.first, pair.second);
      }
      std::filesystem::path primaryPinmapFile =
          FilePath(Action::Bitstream, "PrimaryPinMapping.xml").string();
      std::ofstream ofs(primaryPinmapFile.string());
      ofs << contents;
      ofs.close();
    }
  }

  // IO bitstream
  auto device_data = deviceData();
  std::string device_name = std::string(ProjManager()->getTargetDevice());
  CFG_string_tolower(device_name);
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
  std::filesystem::path ric_folder =
      datapath / "etc" / "devices" / device_name / "ric";
  // By default use the one defined in device XML
  if (!m_OpenFpgaRICModelDir.string().empty()) {
    ric_folder = datapath / "etc" / m_OpenFpgaRICModelDir;
  }
  std::filesystem::path ric_model = ric_folder / "periphery.tcl";
  std::filesystem::path config_mapping =
      datapath / "configuration" / device_data.series /
      (device_name + "_config_attributes.mapping.json");
  std::filesystem::path backdoor_script =
      datapath / "configuration" / device_data.series / "icb_backdoor.py";
  if (!std::filesystem::exists(config_mapping)) {
    config_mapping = datapath / "configuration" / device_data.series /
                     "config_attributes.mapping.json";
  }
  std::filesystem::path netlist_ppdb =
      FilePath(Action::Synthesis, "io_config.json");
  std::vector<std::filesystem::path> api_files =
      FOEDAG::FileUtils::FindFilesByExtension(ric_folder.string(), ".json");
  if (std::filesystem::exists(ric_model) &&
      std::filesystem::exists(config_mapping) &&
      std::filesystem::exists(netlist_ppdb)) {
    // Read the INI before the m_constraints is reset
    nlohmann::json properties = m_constraints->get_simplified_property_json();
    std::string io_model_config_file = "";
    if (properties.contains("INI")) {
      if (properties["INI"].contains("SOURCE_IO_MODEL_CONFIG_FILE")) {
        io_model_config_file = properties["INI"]["SOURCE_IO_MODEL_CONFIG_FILE"];
        if (!std::filesystem::exists(io_model_config_file)) {
          std::filesystem::path pp = ProjManager()->projectPath();
          std::filesystem::path mp =
              std::filesystem::absolute(pp / ".." / io_model_config_file);
          if (std::filesystem::exists(mp)) {
            io_model_config_file = mp.string();
          } else {
            mp = std::filesystem::absolute(pp / io_model_config_file);
            if (std::filesystem::exists(mp)) {
              io_model_config_file = mp.string();
            } else {
              io_model_config_file = "";
            }
          }
        }
      }
    }
    // update constraints
    command = CFG_print("cd %s", workingDir.c_str());
    command = CFG_print("%s\nclear_property", command.c_str());
    m_constraints->reset();
    for (const auto& file : ProjManager()->getConstrFiles()) {
      command = CFG_print("%s\nread_sdc {%s}", command.c_str(), file.c_str());
    }
    command = CFG_print("%s\nwrite_property model_config.property.json",
                        command.c_str());
    command = CFG_print(
        "%s\nwrite_simplified_property model_config.simplified.property.json",
        command.c_str());
    command = CFG_print("%s\nundefine_device PERIPHERY", command.c_str());
    command = CFG_print("%s\nsource %s", command.c_str(), ric_model.c_str());
    command = CFG_print("%s\nmodel_config set_model -feature IO PERIPHERY",
                        command.c_str());
    for (auto file : api_files) {
      if (file.string().size() > 9 &&
          file.string().rfind(".api.json") == file.string().size() - 9) {
        std::string filepath =
            CFG_change_directory_to_linux_format(file.string());
        command = CFG_print("%s\nmodel_config set_api -feature IO {%s}",
                            command.c_str(), filepath.c_str());
      }
    }
    command = CFG_print("%s\nmodel_config dump_ric PERIPHERY io_ric.txt",
                        command.c_str());
    uint32_t gen_bitstream_count = 1;
    if (CFG_find_string_in_vector({"Gemini", "Virgo"}, device_data.series) >=
        0) {
      command = CFG_print(
          "%s\nmodel_config gen_ppdb -netlist_ppdb %s -config_mapping %s "
          "-property_json model_config.property.json -pll_workaround 0 "
          "model_config.ppdb.json",
          command.c_str(), netlist_ppdb.c_str(), config_mapping.c_str());
      command = CFG_print(
          "%s\nmodel_config gen_ppdb -netlist_ppdb %s -config_mapping %s "
          "-property_json model_config.property.json -pll_workaround 1 "
          "model_config.post.ppdb.json",
          command.c_str(), netlist_ppdb.c_str(), config_mapping.c_str());
      gen_bitstream_count = 2;
    } else {
      command = CFG_print(
          "%s\nmodel_config gen_ppdb -netlist_ppdb %s -config_mapping %s "
          "-property_json model_config.property.json model_config.ppdb.json",
          command.c_str(), netlist_ppdb.c_str(), config_mapping.c_str());
    }
    std::string design = "model_config.ppdb.json";
    std::string bit_file = "io_bitstream.bit";
    std::string detail_file = "io_bitstream.detail.bit";
    std::string backdoor_file = "io_bitstream.backdoor.txt";
    for (uint32_t i = 0; i < gen_bitstream_count; i++) {
      command = CFG_print("%s\nmodel_config set_design -feature IO %s",
                          command.c_str(), design.c_str());
      if (io_model_config_file.size()) {
        command = CFG_print("%s\nsource %s", command.c_str(),
                            io_model_config_file.c_str());
      }
      command = CFG_print("%s\nmodel_config write -feature IO -format BIT %s",
                          command.c_str(), bit_file.c_str());
      command =
          CFG_print("%s\nmodel_config write -feature IO -format DETAIL %s",
                    command.c_str(), detail_file.c_str());
      if (std::filesystem::exists(backdoor_script)) {
        command = CFG_print("%s\nmodel_config backdoor -script %s -input %s %s",
                            command.c_str(), backdoor_script.c_str(),
                            detail_file.c_str(), backdoor_file.c_str());
      }
      if (i == 0 && gen_bitstream_count == 2) {
        command =
            CFG_print("%s\nmodel_config reset -feature IO", command.c_str());
        design = "model_config.post.ppdb.json";
        bit_file = "io_bitstream.post.bit";
        detail_file = "io_bitstream.post.detail.bit";
        backdoor_file = "io_bitstream.post.backdoor.txt";
      }
    }
    file = ProjManager()->projectName() + "_io_bitstream_cmd.tcl";
    FileUtils::WriteToFile(file, command);
    command = CFG_print("source %s", file.c_str());
    m_interp->evalCmd(command, &status);
    if (status != TCL_OK) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " IO bitstream generation failed");
      return false;
    }
  }

  m_state = State::BistreamGenerated;

  Message("Design " + ProjManager()->projectName() + " bitstream is generated");
  return true;
}

bool CompilerOpenFPGA::LoadDeviceData(const std::string& deviceName) {
  bool status = true;
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
  std::filesystem::path devicefile =
      datapath / std::string("etc") / std::string("device.xml");
  if (!DeviceFile().empty()) devicefile = DeviceFile();
  bool deviceFound = false;
  const std::filesystem::path devicesBase = devicefile.parent_path();
  status = LoadDeviceData(deviceName, devicefile, devicesBase, deviceFound);
  if (status) {
    // Local (Usually temporary) device settings per device directory
    // The HW team might want to try some options or settings before making them
    // official
    std::filesystem::path device_data_dir = m_architectureFile.parent_path();
    std::filesystem::path local_device_settings =
        device_data_dir / "device.xml";
    if (std::filesystem::exists(local_device_settings)) {
      status = LoadDeviceData(deviceName, local_device_settings, devicesBase,
                              deviceFound);
    }
  }
  m_deviceFileLocal = false;
  if (!deviceFound) {
    // load local devices
    auto local = Config::Instance()->customDeviceXml();
    if (std::filesystem::exists(local)) {
      status = LoadDeviceData(deviceName, local, devicesBase, deviceFound);
      m_deviceFileLocal = true;
    }
  }
  if (!deviceFound) {
    ErrorMessage("Incorrect device: " + deviceName + "\n");
  }
  if (status) reloadSettings();
  SetDeviceResources();
  if (!LicenseDevice(BaseDeviceName())) {
    ErrorMessage("Device is not licensed: " + deviceName + "\n");
    status = false;
  }

  return status;
}

bool CompilerOpenFPGA::LoadDeviceData(
    const std::string& deviceName, const std::filesystem::path& deviceListFile,
    const std::filesystem::path& devicesBase, bool& deviceFound) {
  bool status = true;
  QFile file(deviceListFile.string().c_str());
  if (!file.open(QFile::ReadOnly)) {
    ErrorMessage("Cannot open device file: " + deviceListFile.string());
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    ErrorMessage("Incorrect device file: " + deviceListFile.string());
    return false;
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      std::string name = e.attribute("name").toStdString();
      std::string family = e.attribute("family").toStdString();
      std::string series = e.attribute("series").toStdString();
      std::string package = e.attribute("package").toStdString();
      if (name == deviceName) {
        setDeviceData({family, series, package});
        deviceFound = true;
        BaseDeviceName(deviceName);
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
              if (!file.empty()) {
                if (FileUtils::FileExists(file)) {
                  fullPath = file;  // Absolute path
                } else {
                  fullPath = devicesBase / file;
                }
                if (!FileUtils::FileExists(fullPath.string())) {
                  ErrorMessage("Invalid device config file: " +
                               fullPath.string() + "\n");
                  status = false;
                }
              }
              if (file_type == "vpr_arch") {
                ArchitectureFile(fullPath.string());
              } else if (file_type == "openfpga_arch") {
                OpenFpgaArchitectureFile(fullPath.string());
              } else if (file_type == "bitstream_settings") {
                OpenFpgaBitstreamSettingFile(fullPath.string());
              } else if (file_type == "routing_graph") {
                RoutingGraphFile(fullPath.string());
              } else if (file_type == "sim_settings") {
                OpenFpgaSimSettingFile(fullPath.string());
              } else if (file_type == "repack_settings") {
                OpenFpgaRepackConstraintsFile(fullPath.string());
              } else if (file_type == "fabric_key") {
                OpenFpgaFabricKeyFile(fullPath.string());
              } else if (file_type == "pinmap_xml") {
                OpenFpgaPinmapXMLFile(fullPath.string());
              } else if (file_type == "pcf_xml") {
                OpenFpgaPinConstraintFile(fullPath.string());
              } else if (file_type == "ric_model_dir") {
                OpenFpgaRICModelDir(fullPath.string());
              } else if (file_type == "pb_pin_fixup") {
                PbPinFixup(name);
              } else if (file_type == "pinmap_csv") {
                PinmapCSVFile(fullPath);
              } else if (file_type == "plugin_lib") {
                YosysPluginLibName(name);
              } else if (file_type == "plugin_func") {
                YosysPluginName(name);
              } else if (file_type == "technology") {
                YosysMapTechnology(name);
              } else if (file_type == "tag_version") {
                DeviceTagVersion(name);
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
              } else if (file_type == "flat_routing") {
                if (num == "true") {
                  FlatRouting(true);
                } else if (num == "false") {
                  FlatRouting(false);
                } else {
                  ErrorMessage(
                      "Invalid flat_routing num (true, false): " + num + "\n");
                  status = false;
                }
              } else if (file_type == "base_device") {
                BaseDeviceName(name);
                // field is used for identify base for custom device
                // no action so far
              } else {
                ErrorMessage("Invalid device config type: " + file_type + "\n");
                status = false;
              }
            } else if (n.nodeName() == "resource") {
              std::string file_type =
                  n.toElement().attribute("type").toStdString();
              std::string num = n.toElement().attribute("num").toStdString();
              if (file_type == "dsp") {
                MaxDeviceDSPCount(std::strtoul(num.c_str(), nullptr, 10));
                MaxUserDSPCount(MaxDeviceDSPCount());
              } else if (file_type == "bram") {
                MaxDeviceBRAMCount(std::strtoul(num.c_str(), nullptr, 10));
                MaxUserBRAMCount(MaxDeviceBRAMCount());
              } else if (file_type == "carry_length") {
                MaxDeviceCarryLength(std::strtoul(num.c_str(), nullptr, 10));
                MaxUserCarryLength(MaxDeviceCarryLength());
              } else if (file_type == "lut") {
                MaxDeviceLUTCount(std::strtoul(num.c_str(), nullptr, 10));
              } else if (file_type == "ff") {
                MaxDeviceFFCount(std::strtoul(num.c_str(), nullptr, 10));
              } else if (file_type == "io") {
                MaxDeviceIOCount(std::strtoul(num.c_str(), nullptr, 10));
              }
            }
          }
        }
      }
    }

    node = node.nextSibling();
  }
  if (!deviceFound) {
    status = false;
  }
  return status;
}

bool CompilerOpenFPGA::LicenseDevice(const std::string& deviceName) {
  // No need for licenses
  return true;
}
