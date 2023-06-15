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
#include "Main/Settings.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
#include "scope_guard/scope_guard.hpp"

using json = nlohmann::ordered_json;

using namespace FOEDAG;

void CompilerOpenFPGA::Version(std::ostream* out) {
  (*out) << "Foedag OpenFPGA Compiler"
         << "\n";
  LogUtils::PrintVersion(out);
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

std::filesystem::path CompilerOpenFPGA::copyLog(
    FOEDAG::ProjectManager* projManager, const std::string& srcFileName,
    const std::string& destFileName) {
  std::filesystem::path dest{};

  if (projManager) {
    std::filesystem::path projectPath(projManager->projectPath());
    std::filesystem::path src = projectPath / srcFileName;
    if (FileUtils::FileExists(src)) {
      dest = projectPath / destFileName;
      std::filesystem::remove(dest);
      std::filesystem::copy_file(src, dest);
    }
  }

  return dest;
}

bool CompilerOpenFPGA::IPGenerate() {
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
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
    StringUtils::tokenize(FileUtils::AdjustPath(path), " ", tokens);
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
    StringUtils::tokenize(FileUtils::AdjustPath(path), " ", tokens);
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
}

std::vector<std::string> CompilerOpenFPGA::GetCleanFiles(
    Action action, const std::string& projectName,
    const std::string& topModule) const {
  namespace fs = std::filesystem;
  std::vector<std::string> files;
  switch (action) {
    case Compiler::Action::Analyze:
      files = {ANALYSIS_LOG, "port_info.json", "hier_info.json",
               std::string{projectName + "_analyzer.cmd"}};
      break;
    case Compiler::Action::Synthesis:
      files = {
          std::string{projectName + "_post_synth.blif"},
          std::string{projectName + "_post_synth.eblif"},
          std::string{projectName + "_post_synth.edif"},
          std::string{projectName + "_post_synth.v"},
          std::string{projectName + "_post_synth.vhd"},
          std::string{projectName + ".ys"},
          std::string{projectName + "_synth.log"},
          SYNTHESIS_LOG,
          fs::path{fs::path{"reports"} / "synth_utilization.json"}.string(),
          fs::path{fs::path{"reports"} / "synth_design_stat.json"}.string()};
      break;
    case Compiler::Action::Pack:
      files = {
          std::string{projectName + "_post_synth.net"},
          std::string{projectName + "_pack.cmd"},
          "check_rr_node_warnings.log",
          "packing_pin_util.rpt",
          "pre_pack.report_timing.setup.rpt",
          std::string{projectName + "_openfpga.sdc"},
          std::string{projectName + "_post_synth_ports.json"},
          "vpr_stdout.log",
          PACKING_LOG,
          fs::path{fs::path{"reports"} / "packing_utilization.json"}.string(),
          fs::path{fs::path{"reports"} / "packing_design_stat.json"}.string()};
      break;
    case Compiler::Action::Detailed:
      files = {
          "packing_pin_util.rpt",
          std::string{projectName + "_post_place_timing.rpt"},
          std::string{projectName + "_post_synth_ports.json"},
          std::string{projectName + "_place.cmd"},
          std::string{projectName + "_openfpga.pcf"},
          "check_rr_node_warnings.log",
          std::string{projectName + "_post_synth.place"},
          std::string{projectName + "_pin_loc.cmd"},
          std::string{projectName + "_pin_loc.place"},
          "vpr_stdout.log",
          "post_place_timing.rpt",
          PLACEMENT_LOG,
          fs::path{fs::path{"reports"} / "place_utilization.json"}.string(),
          fs::path{fs::path{"reports"} / "place_design_stat.json"}.string()};
      break;
    case Compiler::Action::Routing:
      files = {
          "check_rr_node_warnings.log",
          std::string{topModule + "_post_synthesis.blif"},
          std::string{topModule + "_post_synthesis.eblif"},
          std::string{topModule + "_post_synthesis.sdf"},
          std::string{topModule + "_post_synthesis.v"},
          std::string{projectName + "_post_synth_ports.json"},
          std::string{projectName + "_route.cmd"},
          std::string{projectName + "_post_synth.route"},
          "packing_pin_util.rpt",
          "post_place_timing.rpt",
          "post_route_timing.rpt",
          "report_timing.hold.rpt",
          "report_timing.setup.rpt",
          "report_unconstrained_timing.hold.rpt",
          "report_unconstrained_timing.setup.rpt",
          ROUTING_LOG,
          "vpr_stdout.log",
          fs::path{fs::path{"reports"} / "route_utilization.json"}.string(),
          fs::path{fs::path{"reports"} / "route_design_stat.json"}.string()};
      break;
    case Compiler::Action::STA:
      files = {"check_rr_node_warnings.log",
               std::string{topModule + "_post_synthesis.blif"},
               std::string{topModule + "_post_synthesis.eblif"},
               std::string{topModule + "_post_synthesis.sdf"},
               std::string{topModule + "_post_synthesis.v"},
               std::string{projectName + "_sta.cmd"},
               std::string{projectName + "_post_synth_ports.json"},
               "packing_pin_util.rpt",
               "post_place_timing.rpt",
               "post_route_timing.rpt",
               "post_ta_timing.rpt",
               "report_timing.hold.rpt",
               "report_timing.setup.rpt",
               "report_unconstrained_timing.hold.rpt",
               "report_unconstrained_timing.setup.rpt",
               TIMING_ANALYSIS_LOG,
               "vpr_stdout.log",
               fs::path{fs::path{"reports"} / "sta_utilization.json"}.string(),
               fs::path{fs::path{"reports"} / "sta_design_stat.json"}.string()};
      break;
    case Compiler::Action::Power:
      files = {"post_place_timing.rpt", "post_route_timing.rpt",
               "post_ta_timing.rpt", "vpr_stdout.log", POWER_ANALYSIS_LOG};
      break;
    case Compiler::Action::Bitstream:
      files = {std::string{projectName + ".openfpga"},
               std::string{projectName + "_bitstream.cmd"},
               std::string{projectName + "_post_synth_ports.json"},
               "fabric_bitstream.bit",
               "fabric_independent_bitstream.xml",
               "packing_pin_util.rpt",
               "PinMapping.xml",
               "post_place_timing.rpt",
               "post_route_timing.rpt",
               "post_ta_timing.rpt",
               "report_timing.hold.rpt",
               "report_timing.setup.rpt",
               "report_unconstrained_timing.hold.rpt",
               "report_unconstrained_timing.setup.rpt",
               "vpr_stdout.log",
               BITSTREAM_LOG};
      break;
    default:
      break;
  }
  return files;
}

std::string CompilerOpenFPGA::InitAnalyzeScript() {
  std::string analysisScript;
  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    fileList += "-set-warning VERI-1063\n";
    std::string includes;
    for (auto path : ProjManager()->includePathList()) {
      includes += FileUtils::AdjustPath(path) + " ";
    }

    // Add Tcl project directory as an include dir
    if (!GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script = GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      includes += FileUtils::AdjustPath(scriptPath.string()) + " ";
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
          includes += FileUtils::AdjustPath(path) + " ";
          designFileDirs.insert(path);
        }
      }
    }

    fileList += "-vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : ProjManager()->libraryPathList()) {
      libraries += FileUtils::AdjustPath(path) + " ";
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
          lang = "";
          break;
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          lang = "BLIF";
          ErrorMessage("Unsupported file format:" + lang);
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
       case Design::Language::BLIF:
       case Design::Language::EBLIF:
       ErrorMessage("Unsupported language (Yosys default parser)");
       break;
       }
       analysisScript = fileList;
       */
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
    std::filesystem::path projectPath(ProjManager()->projectPath());
    std::filesystem::path logPath = projectPath / ANALYSIS_LOG;
    LogUtils::AddHeaderToLog(logPath);
  });

  auto printTopModules = [](const std::filesystem::path& filePath,
                            std::ostream* out) {
    // Check for "topModule" in a given json filePath
    // Assumed json format is [ { "topModule" : "some_value"} ]
    if (out) {
      if (FileUtils::FileExists(filePath)) {
        std::ifstream file(filePath);
        json data = json::parse(file);
        if (data.is_array()) {
          std::vector<std::string> topModules;
          std::transform(data.begin(), data.end(),
                         std::back_inserter(topModules),
                         [](json val) -> std::string {
                           return val.value("topModule", "");
                         });

          (*out) << "Top Modules: " << StringUtils::join(topModules, ", ")
                 << std::endl;
        }
      }
    }
  };

  if (AnalyzeOpt() == DesignAnalysisOpt::Clean) {
    Message("Cleaning analysis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    AnalyzeOpt(DesignAnalysisOpt::None);
    CleanFiles(Action::Analyze);
    return true;
  }
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  if (!HasTargetDevice()) return false;

  PERF_LOG("Analysis has started");
  Message("##################################################");
  Message("Analysis for design: " + ProjManager()->projectName());
  Message("##################################################");

  std::string analysisScript = InitAnalyzeScript();
  analysisScript = FinishAnalyzeScript(analysisScript);

  std::string script_path = ProjManager()->projectName() + "_analyzer.cmd";
  script_path =
      (std::filesystem::path(ProjManager()->projectPath()) / script_path)
          .string();
  std::filesystem::path output_path =
      std::filesystem::path(ProjManager()->projectPath()) / "port_info.json";
  if (!DesignChanged(analysisScript, script_path, output_path)) {
    Message("Design didn't change: " + ProjManager()->projectName() +
            ", skipping analysis.");
    std::stringstream tempOut{};
    printTopModules(output_path, &tempOut);
    Message(tempOut.str());
    return true;
  }
  // Create Analyser command and execute
  std::ofstream ofs(script_path);
  ofs << analysisScript;
  ofs.close();
  std::string command;
  int status = 0;
  std::filesystem::path analyse_path =
      std::filesystem::path(ProjManager()->projectPath()) / ANALYSIS_LOG;
  if (m_useVerific) {
    if (!FileUtils::FileExists(m_analyzeExecutablePath)) {
      ErrorMessage("Cannot find executable: " +
                   m_analyzeExecutablePath.string());
      return false;
    }
    command = m_analyzeExecutablePath.string() + " -f " + script_path;
    Message("Analyze command: " + command);
    status = ExecuteAndMonitorSystemCommand(command, analyse_path.string());
  }
  std::ifstream raptor_log(analyse_path.string());
  if (raptor_log.good()) {
    std::stringstream buffer;
    buffer << raptor_log.rdbuf();
    const std::string& buf = buffer.str();
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
    raptor_log.close();
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

bool CompilerOpenFPGA::Synthesize() {
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), ProjManager()->projectName() + "_synth.log",
            SYNTHESIS_LOG);
  });

  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + ProjManager()->projectName());
    m_state = State::IPGenerated;
    SynthOpt(SYNTH_OPT_DEFAULT);
    CleanFiles(Action::Synthesis);
    return true;
  }
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
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
      includes += FileUtils::AdjustPath(path) + " ";
    }

    // Add Tcl project directory as an include dir
    if (!GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script = GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      includes += FileUtils::AdjustPath(scriptPath.string()) + " ";
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
          includes += FileUtils::AdjustPath(path) + " ";
          designFileDirs.insert(path);
        }
      }
    }

    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : ProjManager()->libraryPathList()) {
      libraries += FileUtils::AdjustPath(path) + " ";
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
    if (ProjManager()->DesignTopModule().empty()) {
      fileList += "verific -import -all\n";
    } else {
      fileList += "verific " + topModuleLibImport + importLibs + "-import " +
                  ProjManager()->DesignTopModule() + "\n";
    }
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
  } else {
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
      includes += "-I" + FileUtils::AdjustPath(path) + " ";
    }

    // Add Tcl project directory as an include dir
    if (!GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script = GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      includes += "-I" + FileUtils::AdjustPath(scriptPath.string()) + " ";
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
          includes += "-I" + FileUtils::AdjustPath(path) + " ";
          designFileDirs.insert(path);
        }
      }
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
    yosysScript =
        ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", macros + designFiles);
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
    Message("Design didn't change: " + ProjManager()->projectName() +
            ", skipping synthesis.");
    return true;
  }
  std::filesystem::remove(
      std::filesystem::path(ProjManager()->projectPath()) /
      std::string(ProjManager()->projectName() + "_post_synth.blif"));
  std::filesystem::remove(
      std::filesystem::path(ProjManager()->projectPath()) /
      std::string(ProjManager()->projectName() + "_post_synth.eblif"));
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
  Message("Synthesis command: " + command);
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " synthesis failed");
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
  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile + std::string(" --sdc_file ") +
                  std::string(ProjManager()->projectName() + "_openfpga.sdc") +
                  std::string(" --clock_modeling ideal --route_chan_width ") +
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
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", PACKING_LOG);
  });

  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    CleanFiles(Action::Pack);
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
  Message("##################################################");
  Message("Packing for design: " + ProjManager()->projectName());
  Message("##################################################");
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
    Message("Constraint: " + constraint);
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
    if (constraint.find("set_property") != std::string::npos) {
      continue;
    }
    if (constraint.find("set_clock_pin") != std::string::npos) {
      continue;
    }
    ofssdc << constraint << "\n";
  }
  ofssdc.close();

  auto prevOpt = PackOpt();
  PackOpt(PackingOpt::None);

  std::string command = BaseVprCommand() + " --pack";
  std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
                     std::string(ProjManager()->projectName() + "_pack.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();

  if (FileUtils::IsUptoDate(
          GetNetlistPath(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.net"))
              .string()) &&
      (prevOpt != PackingOpt::Debug)) {
    m_state = State::Packed;
    Message("Design " + ProjManager()->projectName() + " packing reused");
    return true;
  }

  PackOpt(prevOpt);
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " packing failed");
    if (PackOpt() == PackingOpt::Debug) {
      std::string command = BaseVprCommand() + " --pack";
      std::ofstream ofs(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_pack.cmd"))
              .string());
      ofs << command << std::endl;
      ofs.close();

      ExecuteAndMonitorSystemCommand(command);
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
    CleanFiles(Action::Detailed);
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

  const std::string pcfOut =
      (std::filesystem::path(ProjManager()->projectPath()) /
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
  bool repackConstraint = false;
  std::vector<std::string> constraints;
  std::vector<std::string> set_clks;
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
    Message("Design " + ProjManager()->projectName() + " placement reused");
    return true;
  }

  std::string netlistFile = ProjManager()->projectName() + "_post_synth.blif";
  if (GetNetlistType() == NetlistType::EBlif) {
    netlistFile = ProjManager()->projectName() + "_post_synth.eblif";
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

  std::string command = BaseVprCommand() + " --place";
  std::string pincommand = m_pinConvExecutablePath.string();
  if ((PinAssignOpts() != PinAssignOpt::Free) &&
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

    if (GetNetlistType() == NetlistType::Verilog ||
        GetNetlistType() == NetlistType::VHDL ||
        GetNetlistType() == NetlistType::Edif) {
      std::filesystem::path p(netlistFile);
      p.replace_extension();
      pincommand += " --port_info ";
      pincommand += p.string() + "_ports.json";
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
    } else if (PinAssignOpts() == PinAssignOpt::Free) {
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
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + ".temp_file_clkmap"))
              .string();
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

    std::string pin_loc_constraint_file;

    std::ofstream ofsp(
        (std::filesystem::path(ProjManager()->projectPath()) /
         std::string(ProjManager()->projectName() + "_pin_loc.cmd"))
            .string());
    ofsp << pincommand << std::endl;
    ofsp.close();

    int status = ExecuteAndMonitorSystemCommand(pincommand);

    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " pin conversion failed");
      return false;
    } else {
      pin_loc_constraint_file = pin_locFile;
    }

    if ((PinAssignOpts() != PinAssignOpt::Free) && PinConstraintEnabled() &&
        (!pin_loc_constraint_file.empty())) {
      command += " --fix_clusters " + pin_loc_constraint_file;
    }
  }

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
                     std::string(ProjManager()->projectName() + "_place.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " placement failed");
    return false;
  }
  m_state = State::Placed;
  Message("Design " + ProjManager()->projectName() + " is placed");
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
      if ((tokens.size() != 3) && (tokens.size() != 4)) {
        ErrorMessage("Invalid set_pin_loc command: <" + constraints[i] + ">");
        return false;
      }
      std::string constraint_with_mode = tokens[0] + std::string(" ") +
                                         tokens[1] + std::string(" ") +
                                         tokens[2];
      if (pin_mode_map.find(tokens[2]) != pin_mode_map.end()) {
        constraint_with_mode +=
            std::string(" -mode ") + pin_mode_map[tokens[2]];
      } else {
        constraint_with_mode += std::string(" -mode Mode_GPIO");
      }
      if (tokens.size() == 4) {
        constraint_with_mode += std::string(" -internal_pin ") + tokens[3];
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
  // Using a Scope Guard so this will fire even if we exit mid function
  // This will fire when the containing function goes out of scope
  auto guard = sg::make_scope_guard([this] {
    // Rename log file
    copyLog(ProjManager(), "vpr_stdout.log", ROUTING_LOG);
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

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.place"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string())) {
    m_state = State::Routed;
    Message("Design " + ProjManager()->projectName() + " routing reused");
    return true;
  }

  std::string command = BaseVprCommand() + " --route";
  std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
                     std::string(ProjManager()->projectName() + "_route.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " routing failed");
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

  if (TimingAnalysisOpt() == STAOpt::View) {
    TimingAnalysisOpt(STAOpt::None);
    const std::string command = BaseVprCommand() + " --analysis --disp on";
    const int status = ExecuteAndMonitorSystemCommand(command);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " place and route view failed");
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
    Message("Design " + ProjManager()->projectName() + " timing didn't change");
    return true;
  }
  int status = 0;
  std::string taCommand;
  // use OpenSTA to do the job
  if (TimingAnalysisEngineOpt() == STAEngineOpt::Opensta) {
    // allows SDF to be generated for OpenSTA
    std::string command = BaseVprCommand() + " --gen_post_synthesis_netlist on";
    std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
                       std::string(ProjManager()->projectName() + "_sta.cmd"))
                          .string());
    ofs.close();
    int status = ExecuteAndMonitorSystemCommand(command);
    if (status) {
      ErrorMessage("Design " + ProjManager()->projectName() +
                   " timing analysis failed");
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
      std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
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
    std::ofstream ofs((std::filesystem::path(ProjManager()->projectPath()) /
                       std::string(ProjManager()->projectName() + "_sta.cmd"))
                          .string());
    ofs << taCommand << " --disp on" << std::endl;
    ofs.close();
  }

  status = ExecuteAndMonitorSystemCommand(taCommand);
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
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_sta.cmd"))
              .string())) {
    Message("Design " + ProjManager()->projectName() + " power didn't change");
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
                 " power analysis failed");
    return false;
  }

  Message("Design " + ProjManager()->projectName() + " is power analysed");
  return true;
}

const std::string basicOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --skip_sync_clustering_and_routing_results ${VPR_PB_PIN_FIXUP} --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT} --analysis ${PNR_OPTIONS}

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
build_fabric --frame_view --compress_routing --duplicate_grid_pin ${OPENFPGA_BUILD_FABRIC_OPTION}

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream ${BUILD_ARCHITECTURE_BITSTREAM_OPTIONS}

build_fabric_bitstream
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit
${WRITE_FABRIC_BITSTREAM_XML}
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

const std::string simulationOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --skip_sync_clustering_and_routing_results ${VPR_PB_PIN_FIXUP} --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT} --analysis ${PNR_OPTIONS}

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
build_fabric --frame_view --compress_routing --duplicate_grid_pin ${OPENFPGA_BUILD_FABRIC_OPTION}

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

build_architecture_bitstream --verbose \
                             --write_file fabric_independent_bitstream.xml
 
build_fabric_bitstream --verbose 

write_fabric_verilog --file BIT_SIM \
                     --explicit_port_mapping \
                     --include_timing \
                     --print_user_defined_template \
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
    if (BitsOpt() == BitstreamOpt::EnableSimulation) {
      m_openFPGAScript = simulationOpenFPGABitstreamScript;
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
  if (!ProjManager()->DesignTopModule().empty())
    result += " --top " + ProjManager()->DesignTopModule();

  std::string pnrOptions;
  if (ClbPackingOption() == ClbPacking::Timing_driven) {
    pnrOptions += " --allow_unrelated_clustering off";
  } else {
    pnrOptions += " --allow_unrelated_clustering on";
  }
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
  result = ReplaceAll(result, "${PNR_OPTIONS}", pnrOptions);
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

  std::string netlistFormat;
  switch (GetNetlistType()) {
    case NetlistType::Verilog:
      netlistFormat = "verilog";
      break;
    case NetlistType::VHDL:
      // Until we have a VHDL netlist reader in VPR
      netlistFormat = "verilog";
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
  if (m_OpenFpgaBitstreamSettingFile.string().empty()) {
    result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}", "");
  } else {
    result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                        "read_openfpga_bitstream_setting -f " +
                            m_OpenFpgaBitstreamSettingFile.string());
  }
  result = ReplaceAll(result, "${OPENFPGA_BITSTREAM_SETTING_FILE}",
                      m_OpenFpgaBitstreamSettingFile.string());
  result = ReplaceAll(result, "${OPENFPGA_PIN_CONSTRAINTS}",
                      m_OpenFpgaPinConstraintXml.string());
  std::string repack_constraints =
      ProjManager()->projectName() + "_repack_constraints.xml";
  const bool fpga_repack = FileUtils::FileExists(
      std::filesystem::path(ProjManager()->projectPath()) / repack_constraints);
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
    BitsOpt(BitstreamOpt::DefaultBitsOpt);
    CleanFiles(Action::Bitstream);
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
  Message("##################################################");
  Message("Bitstream generation for design \"" + ProjManager()->projectName() +
          "\" on device \"" + ProjManager()->getTargetDevice() + "\"");
  Message("##################################################");
  if ((m_state != State::Routed) && (m_state != State::BistreamGenerated)) {
    ErrorMessage("Design needs to be in routed state");
    return false;
  }

  if (BitsOpt() == BitstreamOpt::EnableSimulation) {
    std::filesystem::path bit_path =
        std::filesystem::path(ProjManager()->projectPath()) / "BIT_SIM";
    std::filesystem::create_directory(bit_path);
  }

  if (FileUtils::IsUptoDate(
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string(ProjManager()->projectName() + "_post_synth.route"))
              .string(),
          (std::filesystem::path(ProjManager()->projectPath()) /
           std::string("fabric_bitstream.bit"))
              .string())) {
    Message("Design " + ProjManager()->projectName() +
            " bitstream didn't change");
    m_state = State::BistreamGenerated;
    return true;
  }

  if (BitsOpt() == BitstreamOpt::DefaultBitsOpt) {
#ifdef PRODUCTION_BUILD
    if (BitstreamEnabled() == false) {
      Message("Device " + ProjManager()->getTargetDevice() +
              " bitstream is not enabled, skipping");
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

  std::filesystem::remove(std::filesystem::path(ProjManager()->projectPath()) /
                          std::string("fabric_bitstream.bit"));
  std::filesystem::remove(std::filesystem::path(ProjManager()->projectPath()) /
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
                 " bitstream generation failed");
    return false;
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
  status = LoadDeviceData(deviceName, devicefile);
  if (status) {
    // Local (Usually temporary) device settings per device directory
    // The HW team might want to try some options or settings before making them
    // official
    std::filesystem::path device_data_dir = m_architectureFile.parent_path();
    std::filesystem::path local_device_settings =
        device_data_dir / "device.xml";
    if (std::filesystem::exists(local_device_settings)) {
      status = LoadDeviceData(deviceName, local_device_settings);
    }
  }
  if (status) reloadSettings();
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
  return status;
}

bool CompilerOpenFPGA::LoadDeviceData(
    const std::string& deviceName,
    const std::filesystem::path& deviceListFile) {
  bool status = true;
  std::filesystem::path datapath = GetSession()->Context()->DataPath();
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
  bool foundDevice = false;
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      std::string name = e.attribute("name").toStdString();
      std::string family = e.attribute("family").toStdString();
      std::string series = e.attribute("series").toStdString();
      std::string package = e.attribute("package").toStdString();
      if (name == deviceName) {
        setDeviceData({family, series, package});
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
              if (!file.empty()) {
                if (FileUtils::FileExists(file)) {
                  fullPath = file;  // Absolute path
                } else {
                  fullPath = datapath / std::string("etc") /
                             std::string("devices") / file;
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
