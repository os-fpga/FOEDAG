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

// clang-format off

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
#include <QJsonArray>
#include <QDirIterator>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>
#include <regex>
#include <vector>
#include <string>
#include <locale>

#include "Compiler/CompilerOpenFPGA_ql.h"
#include "Compiler/Constraints.h"
#include "Log.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"
#include "MainWindow/main_window.h"
#include "Main/WidgetFactory.h"
#include "Main/Settings.h"
#include <CRFileCryptProc.hpp>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QListWidget>
#include <QTabWidget>

using namespace FOEDAG;

void CompilerOpenFPGA_ql::Version(std::ostream* out) {
  (*out) << "QuickLogic Aurora"
         << "\n";
  PrintVersion(out);
}

CompilerOpenFPGA_ql::~CompilerOpenFPGA_ql() {
  CleanTempFiles();
}

void CompilerOpenFPGA_ql::Help(std::ostream* out) {
  (*out) << "------------------------------------" << std::endl;
  (*out) << "-----  QuickLogic Aurora HELP  -----" << std::endl;
  (*out) << "------------------------------------" << std::endl;
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

// internal fallback yosys template script, if default template script is not found!
const std::string qlYosysScript = R"( 

# yosys (internal) template script for Aurora

# refer:
# 1. https://yosyshq.readthedocs.io/projects/yosys/en/latest/cmd_ref.html#command-line-reference
# 2. https://github.com/chipsalliance/yosys-f4pga-plugins/blob/main/ql-qlf-plugin/synth_quicklogic.cc (help() function describes the commands)

# load the ql-qlf plugin (don't change this):
${PLUGIN_LOAD}

# read design files:
${READ_DESIGN_FILES}

# synthesize:
${QL_SYNTH_PASS_NAME} -top ${TOP_MODULE} -family ${FAMILY} -blif ${OUTPUT_BLIF} ${YOSYS_OPTIONS}

)";

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

bool CompilerOpenFPGA_ql::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  Compiler::RegisterCommands(interp, batchMode);
  auto select_architecture_file = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify type: Yosys/RS/QL");
      return TCL_ERROR;
    }
    // std::string arg = argv[1];
    // if (arg == "Yosys") {
    //   compiler->SynthType(SynthesisType::Yosys);
    // } else if (arg == "RS") {
    //   compiler->SynthType(SynthesisType::RS);
    // } else if (arg == "QL") {
    //   compiler->SynthType(SynthesisType::QL);
    // } else {
    //   compiler->ErrorMessage("Illegal synthesis type: " + arg);
    //   return TCL_ERROR;
    // }
    return TCL_OK;
  };
  interp->registerCmd("synthesis_type", synthesis_type, this, 0);
  
  auto show_settings = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
                            
  CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;

  std::string settings_json_filename = compiler->m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = compiler->GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path));

  // create a temp dialog to show the widgets
  QDialog* dlg = new QDialog();
  dlg->setWindowTitle("Settings");
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  QVBoxLayout* dlg_toplevellayout = new QVBoxLayout();
  QHBoxLayout* dlg_widgetslayout = new QHBoxLayout();
  QHBoxLayout* dlg_buttonslayout = new QHBoxLayout();
  dlg_toplevellayout->addLayout(dlg_widgetslayout); // first the settings stuff
  dlg_toplevellayout->addLayout(dlg_buttonslayout); // second a row of buttons for actions
  dlg->setLayout(dlg_toplevellayout);

  // json structure:
  //  category0
  //    subcategory0
  //      param0 {}
  //      param1 {}
  //    subcategory1
  //      param2 {}
  //      param3 {}
  // category1
  //    ... and so on.

  // 1. settings GUI representation -> Dialog
  // 2. split layout into 
  //      (a) QListWidget (list of categories)
  //      (b) QStackedWidget (content of the categories)
  // 3. on selecting a category (QListWidget) -> corresponding widget which has the content of the category is visible
  // 4. each widget in the category widget (QStackedWidget) will be a QTabWidget
  //    each of the tabs in it represents a subcategory
  //    so, all the params in a subcategory will be displayed inside a 'tab' which is the 'subcategory widget'

  QStackedWidget *stackedWidget = new QStackedWidget();
  QListWidget *listWidget = new QListWidget();

  json& rootJson = currentSettings->getJson();

  for (auto [categoryId, categoryJson] : rootJson.items()) {

    if(categoryId == "Tasks") {
      // we don't use this section in our use-cases.
      continue;
    }

    // container widget for each 'category' -> this will be each 'page' of the QStackedWidget
    // this container widget will contain all the 'subcategories' of the category -> hence a QTabWidget for each category
    QTabWidget* categoryWidget = new QTabWidget();

    for (auto [subcategoryId, subcategoryJson] : categoryJson.items()) {

      // container widget for each 'subcategory'  -> 'page' widget inside the category QTabWidget
      // the container widget will contain all the 'parameters' of this subcategory -> hence a simple QWidget will do.
      // this should become a ScrollArea.
      QWidget* subcategoryWidget = new QWidget();
      QVBoxLayout* subcategoryWidgetlayout = new QVBoxLayout();
      subcategoryWidgetlayout->setAlignment(Qt::AlignTop);
      subcategoryWidget->setLayout(subcategoryWidgetlayout);

      for (auto [widgetId, widgetJson] : subcategoryJson.items()) {

        // finally, each parameter becomes a widget according the type and properties.
        QWidget* subWidget =
            FOEDAG::createWidget(widgetJson, QString::fromStdString(widgetId));
        
        // the parameter widget is added into the 'subcategory widget' layout.
        subcategoryWidgetlayout->addWidget(subWidget);
      }

      // subcategory widget ready -> this is a 'page' or 'tab' in QTabWidget, so add to the QTabWidget directly (no layout)
      categoryWidget->addTab(subcategoryWidget, QString::fromStdString(subcategoryId));
    }

    // category widget is ready -> this is a 'page' in the 'container' QStackedWidget
    stackedWidget->addWidget(categoryWidget);
    
    // correspondingly, add the category 'name' into the QListWidget
    new QListWidgetItem(QString::fromStdString(categoryId), listWidget);
  }

  // when a 'category' in the QListView is selected, corresponding 'page' widget in the QStackedWidget should be shown.
  QObject::connect(listWidget, QOverload<int>::of(&QListWidget::currentRowChanged),
            stackedWidget, &QStackedWidget::setCurrentIndex);

  // container widget for all settings, add the QListView(left side), and then QStackedWidget(right side)
  dlg_widgetslayout->addWidget(listWidget);
  dlg_widgetslayout->addWidget(stackedWidget);

  // show vpr by default first, test only.
  listWidget->setCurrentRow(2);

  // make the buttons for the actions in the settings dialog
  QPushButton *button_loadfromjson = new QPushButton("Reload");
  button_loadfromjson->setToolTip("Reload everything from the JSON file");
  QPushButton *button_savetojson = new QPushButton("Save");
  button_savetojson->setToolTip("Save everything to the JSON file");
  QPushButton *button_cancel = new QPushButton("Cancel");
  button_cancel->setToolTip("Discard any modifications to the current session");
  QPushButton *button_save = new QPushButton("Apply");
  button_save->setToolTip("Keep any modifications for the current session");
  
  dlg_buttonslayout->addWidget(button_loadfromjson);
  dlg_buttonslayout->addWidget(button_savetojson);
  dlg_buttonslayout->addStretch();
  dlg_buttonslayout->addWidget(button_cancel);
  dlg_buttonslayout->addWidget(button_save);

  dlg->show();

  return TCL_OK;
  };
  interp->registerCmd("show_settings", show_settings, this, 0);

  auto add_device = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {

    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;

    // add_device <family> <foundry> <node> <source_device_data_dir_path> [force]
    // this will perform the steps:
    // 1. check if the 'device' already exists in the installation
    //      check if the '<INSTALLATION> / device_data / <family> / <foundry> / <node>' dir path
    //        already exists in installation
    //      if it already exists, we will display an error, and stop.
    //      if 'force' has been specified, we will push out a warning, but proceed further.
    // 2. ensure that the structure in the <source_device_data_dir_path> reflects 
    //      required structure, as specified in the document: <TODO>
    //    basically, all the required files should exist, in the right hierarchy,
    //      and missing optional files would output a warning.
    // 3. encrypt all the files in the <source_device_data_dir_path> in place
    // 4. copy over all the encrypted files & cryption db
    //      from: <source_device_data_dir_path>
    //      to: <INSTALLATION> / device_data / <family> / <foundry> / <node>
    //      and clean up all the encrypted files & cryption db from the <source_device_data_dir_path>

    // check args: 5 or 6(if force is specified)
    if (argc != 5 && argc != 6) {
      compiler->ErrorMessage("Please enter command in the format:\n"
                             "    encrypt <family> <foundry> <node> <source_device_data_dir_path> [force]");
      return TCL_ERROR;
    }

    // parse args
    std::string family = compiler->ToUpper(std::string(argv[1]));
    std::string foundry = compiler->ToUpper(std::string(argv[2]));
    std::string node = compiler->ToLower(std::string(argv[3]));
    std::filesystem::path source_device_data_dir_path = argv[4];
    bool force = false;
    if(argc == 6) {
      if( compiler->ToLower(std::string(argv[5])).compare("force") == 0 ) {
        force = true;
      }
    }

    std::string device = compiler->DeviceString(family,
                                                foundry,
                                                node,
                                                "",
                                                "");

    // convert to canonical path, which will also check that the path exists.
    std::error_code ec;
    std::filesystem::path source_device_data_dir_path_c = 
            std::filesystem::canonical(source_device_data_dir_path, ec);
    if(ec) {
      // error
      compiler->ErrorMessage("Please check if the path specified exists!");
      compiler->ErrorMessage("path: " + source_device_data_dir_path.string());
      return TCL_ERROR;
    }

    // debug prints
    // std::cout << std::endl;
    // std::cout << "family: " << family << std::endl;
    // std::cout << "foundry: " << foundry << std::endl;
    // std::cout << "node: " << node << std::endl;
    // std::cout << "source_device_data_dir_path: " << source_device_data_dir_path_c << std::endl;
    // std::cout << "force: " << std::string(force?"true":"false") << std::endl;
    // std::cout << std::endl;

    // [1] check if installation already has the device added and inform the user accordingly.
    //     (device data dir for this device already exists)
    std::filesystem::path target_device_data_dir_path = 
        std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                              family /
                              foundry /
                              node);

    if (std::filesystem::exists(target_device_data_dir_path, ec)) {
      if(force) {
        compiler->Message("\nWARNING: The device you are trying to add already exists in the installation.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("'force' has been specified, this will overwrite the target device dir with new files.");
        compiler->Message("\n");
      }
      else {
        compiler->Message("\n");
        compiler->ErrorMessage("The device you are trying to add already exists in the installation.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("Please specify 'force' to overwrite the target device dir with new files.");
        compiler->Message("Please enter command in the format:\n"
                          "    encrypt <family> <foundry> <node> <source_device_data_dir_path> [force]");
        compiler->Message("\n");
        return TCL_ERROR;
      }
    }
    else {
        compiler->Message("\nNew Device files will be added to the installation.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("\n");
    }


    // [2] check dir structure of the source_device_data_dir_path of the device to be added
    // and return the list of device_variants if everything is ok.
    std::vector<std::string> device_variants;
    device_variants = compiler->list_device_variants(family,
                                                     foundry,
                                                     node,
                                                     source_device_data_dir_path_c);

    if(device_variants.empty()) {
      compiler->ErrorMessage(std::string("error parsing device_data in: ") +
                               source_device_data_dir_path_c.string());
        return TCL_ERROR;
    }
    else {
      // save std::ios settings.
      std::ios ios_default_state(nullptr);
      ios_default_state.copyfmt(std::cout);

      std::cout << std::endl;
      std::cout << "device variants parsed:" << std::endl;
      std::cout << "<family>,<foundry>,<node>,[voltage_threshold],[p_v_t_corner]" << std::endl;
      int index = 1;
      for (auto device_variant: device_variants) {
        std::cout << std::setw(4)
                  << std::setfill(' ')
                  << index;
        // restore cout state
        std::cout.copyfmt(ios_default_state);
        std::cout << ". " 
                  << device_variant 
                  << std::endl;
        index++;
      }
      std::cout << std::endl;
    }

    // collect the list of every filepath in the source_device_data_dir that we want to encrypt.
    std::vector<std::filesystem::path> source_device_data_file_list_to_encrypt;
    std::vector<std::filesystem::path> source_device_data_file_list_to_copy;
    for (const std::filesystem::directory_entry& dir_entry :
        std::filesystem::recursive_directory_iterator(source_device_data_dir_path_c,
                                                      std::filesystem::directory_options::skip_permission_denied,
                                                      ec))
    {
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed listing contents of ") +  source_device_data_dir_path.string());
        return TCL_ERROR;
      }

      if(dir_entry.is_regular_file(ec)) {
          // we want xml files for encryption
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.xml",
                                std::regex::icase))) {
            // exclude fpga_io_map xml files from encryption
            // include them for copy
            if (std::regex_match(dir_entry.path().filename().string(),
                                  std::regex(".+_fpga_io_map\\.xml",
                                  std::regex::icase))) {
              source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
              continue;
            }
            source_device_data_file_list_to_encrypt.push_back(dir_entry.path().string());
          }

          // include pin_table csv files for copy
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+_pin_table\\.csv",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }
      }

      if(ec) {
        compiler->ErrorMessage(std::string("error while checking: ") +  dir_entry.path().string());
        return TCL_ERROR;
      }
    }

    // debug prints
    // std::sort(source_device_data_file_list_to_encrypt.begin(),source_device_data_file_list_to_encrypt.end());
    // std::cout << "source_device_data_file_list_to_encrypt" << std::endl;
    // for(auto path : source_device_data_file_list_to_encrypt) std::cout << path << std::endl;
    // std::cout << std::endl;

    // encrypt the list of files
    if (!CRFileCryptProc::getInstance()->encryptFiles(source_device_data_file_list_to_encrypt)) {
        compiler->ErrorMessage("encrypt files failed!");
        return TCL_ERROR;
    } else {
        compiler->Message("files encrypted ok.");
    }

    // save cryptdb
    string cryptdb_path_str;
    if (!CRFileCryptProc::getInstance()->saveCryptKeyDB(source_device_data_dir_path_c.string(), 
                                                        family + "_" + foundry + "_" + node,
                                                        cryptdb_path_str)) {
        compiler->ErrorMessage("cryptdb save failed!");
        return TCL_ERROR;
    }
    else {
        compiler->Message("cryptdb saved ok.");
    }

    // [4] copy all encrypted files and cryptdb into the installation target_device_data_dir_path
    //     also, cleanup these files in the source_device_data_dir_path

    // delete the target_device_data_dir_path directory in the installation
    //   so that, we don't have a mix of old remnants and new files.
    std::filesystem::remove_all(target_device_data_dir_path,
                                ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to delete target dir: ") + target_device_data_dir_path.string());
      return TCL_ERROR;
    }

    // create the target_device_data_dir_path directory in the installation
    std::filesystem::create_directories(target_device_data_dir_path,
                                        ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to create target dir: ") + target_device_data_dir_path.string());
      return TCL_ERROR;
    }

    // pass through the list of files we prepared earlier for encryption and process each one
    for(std::filesystem::path source_file_path : source_device_data_file_list_to_encrypt) {

      // corresponding encrypted file path
      std::filesystem::path source_en_file_path = 
          std::filesystem::path(source_file_path.string() + ".en");

      // get the encrypted file path, relative to the source_device_data_dir_path
      std::filesystem::path relative_en_file_path = 
          std::filesystem::relative(source_en_file_path,
                                    source_device_data_dir_path_c,
                                    ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create relative path: ") + source_en_file_path.string());
        return TCL_ERROR;
      }

      // add the relative encrypted file path to the target_device_data_dir_path
      std::filesystem::path target_en_file_path = 
          target_device_data_dir_path / relative_en_file_path;

      // ensure that the target encrypted file's parent dir is created if not existing:
      std::filesystem::create_directories(target_en_file_path.parent_path(),
                                          ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create directory: ") + target_en_file_path.parent_path().string());
        return TCL_ERROR;
      }

      // copy the source encrypted file to the target encrypted file path:
      std::cout << "copying:" << relative_en_file_path << std::endl;
      std::filesystem::copy_file(source_en_file_path,
                                 target_en_file_path,
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to copy: ") + source_en_file_path.string());
        return TCL_ERROR;
      }

      // delete the source encrypted file, as it not needed anymore.
      std::filesystem::remove(source_en_file_path,
                              ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to delete: ") + source_en_file_path.string());
        return TCL_ERROR;
      }
    }

    // now copy the cryptdb file into the installation and delete from the source
    std::filesystem::path source_cryptdb_path = cryptdb_path_str;

    // get the cryptdb file path, relative to the source_device_data_dir_path
    std::filesystem::path relative_cryptdb_path =
        std::filesystem::relative(source_cryptdb_path,
                                  source_device_data_dir_path_c,
                                  ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to create relative path: ") + source_cryptdb_path.string());
        return TCL_ERROR;
    }

    // add the relative encrypted file path to the target_device_data_dir_path
    std::filesystem::path target_cryptdb_path =
        target_device_data_dir_path / relative_cryptdb_path;

    // copy the source cryptdb file to the target cryptdb file path:
    std::cout << "copying:" << relative_cryptdb_path << std::endl;
    std::filesystem::copy_file(source_cryptdb_path,
                               target_cryptdb_path,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to copy: ") + source_cryptdb_path.string());
      return TCL_ERROR;
    }

    // delete the source encrypted file, as it not needed anymore.
    std::filesystem::remove(source_cryptdb_path,
                            ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to delete: ") + source_cryptdb_path.string());
      return TCL_ERROR;
    }

    // pass through the list of files we prepared earlier for copying without encryption and process each one
    for(std::filesystem::path source_file_path : source_device_data_file_list_to_copy) {

      // get the file path, relative to the source_device_data_dir_path
      std::filesystem::path relative_file_path = 
          std::filesystem::relative(source_file_path,
                                    source_device_data_dir_path_c,
                                    ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create relative path: ") + source_file_path.string());
        return TCL_ERROR;
      }

      // add the relative file path to the target_device_data_dir_path
      std::filesystem::path target_file_path = 
          target_device_data_dir_path / relative_file_path;

      // ensure that the target file's parent dir is created if not existing:
      std::filesystem::create_directories(target_file_path.parent_path(),
                                          ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create directory: ") + target_file_path.parent_path().string());
        return TCL_ERROR;
      }

      // copy the source file to the target file path:
      std::cout << "copying:" << relative_file_path << std::endl;
      std::filesystem::copy_file(source_file_path,
                                 target_file_path,
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to copy: ") + source_file_path.string());
        return TCL_ERROR;
      }
    }

    compiler->Message("\ndevice added ok: " + device);

    return TCL_OK;
  };
  interp->registerCmd("add_device", add_device, this, 0);

  auto generate_fpga_io_map = [](void* clientData, Tcl_Interp* interp, int argc,
                                    const char* argv[]) -> int {

    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;

    // theory: 
    // we want to generate the FPGA IO MAP XML file for every layout available
    //    in the VPR architecture file(s)
    // to do this, we need to use OpenFPGA which provides the option to do so, but
    //    it requires that we run the full flow (yosys+vpr+openfpga) to generate this.
    // so, we use a very simple and2 design, and for every layout that exists in the
    //    vpr.xml (of each device variant) run the aurora flow for that layout.
    // then, we take the generated fpga io map xml and copy it into the device data.
    // so, we would be running the aurora flow for and2 for NxL times,
    //    where N=number of device_variants, L=number of fixed_layouts
    //
    // implementation notes:
    // iterate through all the 'vpr.xml' files in the device data dir:
    // we know family, foundry, node
    // vpr.xml -> dir = p_v_t_corner (best/worst etc.)
    // vpr.xml -> dir -> parent = voltage_threshold (ulvt/lvt etc.)
    // then, parse vpr.xml for the list of the 'fixed_layout' elements avaialble.
    // for each fixed_layout:
    //    generate the and2.json from the template with device settings(family, foundry...)
    //    run aurora --batch --script and2.tcl to run regular flow to generate fpga_io_map xml
    //    grab the _fpga_io_map.xml file generated, and copy into the device_data dir
    //
    // now, this has a redundancy:
    // multiple vpr files, may have the same layout, so we would be repeating it unnecessarily!!
    // current "worst case" assumption: each device variant may have different layout!
    //  so, we need the io map for every variant + layout.
    // if we know that ALL VPR XML variant files have the SAME LAYOUTs, we can just process
    // the first VPR XML that we find, and be done with it.
    // This is a reasonable ask, as we control the layouts in the XML files.
    // how about a customer? different pvtcorner/thresholds should not need to have different
    // fixed_layouts?
    // <TODO>, if needed we will simplify as above.


    // generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force] [size]
    // this will perform the steps:
    // 1. check the expected fpga_io_map.xml files, and if any are missing
    //    by default, it will only generate the files if they are missing.
    //    to force regeneration of all xml files, use "force"
    // 2. the generated xml files are placed into device_data/family/foundry/node/fpga_io_map directory
    // 3. if [source_device_data_dir_path] is specified, the files are also placed into [source_device_data_dir_path]/fpga_io_map directory

    // check args:4 or 5 or 6(if source_device_data_dir_path/force is specified)
    if (argc != 4 && argc != 5 && argc != 6) {
      compiler->ErrorMessage("Please enter command in the format:\n"
                             "    generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force]");
      return TCL_ERROR;
    }

    // parse args
    std::string family = compiler->ToUpper(std::string(argv[1]));
    std::string foundry = compiler->ToUpper(std::string(argv[2]));
    std::string node = compiler->ToLower(std::string(argv[3]));
    std::filesystem::path source_device_data_dir_path;
    bool force = false;
    if(argc == 4) {
      // no force
      // no source_device_data_dir_path
      // ok.
    }
    else if(argc == 5) {
      // argv[4] can be either force, or source_device_data_dir_path:
      if( compiler->ToLower(std::string(argv[4])).compare("force") == 0 ) {
        force = true;
      }
      else {
        source_device_data_dir_path = argv[4];
      }
    }
    else if(argc == 6) {
      // then argv[4] MUST be source_device_data_dir_path
      // and argv[5] MUST be 'force'
      if( compiler->ToLower(std::string(argv[4])).compare("force") == 0 ) {
        compiler->ErrorMessage("\"force\" must be the last option!");
        compiler->ErrorMessage("Please enter command in the format:\n"
                             "    generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force]");
        return TCL_ERROR;
      }
      else {
        source_device_data_dir_path = argv[4];
      }
      
      if( compiler->ToLower(std::string(argv[5])).compare("force") == 0 ) {
        force = true;
      }
      else {
        compiler->ErrorMessage("\"force\" must be the last option!");
        compiler->ErrorMessage("Please enter command in the format:\n"
                             "    generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force]");
        return TCL_ERROR;
      }
    }
    else {
      compiler->ErrorMessage("Please enter command in the format:\n"
                             "    generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force]");
      return TCL_ERROR;
    }

    std::error_code ec;
    std::filesystem::path source_device_data_dir_path_c;
    if(!source_device_data_dir_path.empty()) {
      // convert to canonical path, which will also check that the path exists.
      source_device_data_dir_path_c = 
              std::filesystem::canonical(source_device_data_dir_path, ec);
      if(ec) {
        // error
        compiler->ErrorMessage("Please check if the path specified exists!");
        compiler->ErrorMessage("path: " + source_device_data_dir_path.string());
        return TCL_ERROR;
      }
    }

    // debug prints
    // std::cout << std::endl;
    // std::cout << "family: " << family << std::endl;
    // std::cout << "foundry: " << foundry << std::endl;
    // std::cout << "node: " << node << std::endl;
    // std::cout << "source_device_data_dir_path: " << source_device_data_dir_path_c << std::endl;
    // std::cout << "force: " << std::string(force?"true":"false") << std::endl;
    // std::cout << std::endl;

    // collect the list of every filepath in the device_data directory that we want to use further
    // we can work with vpr.xml or vpr.xml.en files
    std::string vpr_xml_pattern = "vpr\\.xml.*";
    std::filesystem::path device_data_dir_path = 
        std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                              family /
                              foundry /
                              node);

    std::vector<std::filesystem::path> device_data_file_list;
    for (const std::filesystem::directory_entry& dir_entry :
        std::filesystem::recursive_directory_iterator(device_data_dir_path,
                                                      std::filesystem::directory_options::skip_permission_denied,
                                                      ec))
    {
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed listing contents of ") +  device_data_dir_path.string());
        return TCL_ERROR;
      }

      if(dir_entry.is_regular_file(ec)) {
          // we want vpr.xml files:
          if (std::regex_match(dir_entry.path().filename().string(), 
                                std::regex(vpr_xml_pattern, 
                                std::regex::icase))) {
            device_data_file_list.push_back(dir_entry.path().string());
          }
      }

      if(ec) {
        compiler->ErrorMessage(std::string("error while checking: ") +  dir_entry.path().string());
        return TCL_ERROR;
      }
    }

    // for each vpr.xml, get all the "fixed_layout" elements
    // for each "fixed_layout" element, generate the fpga_io_map xml file.
    for(std::filesystem::path source_file_path : device_data_file_list) {
      // we already have family, foundry, node from inputs.
      // get the p_v_t_corner and voltage_threshold
      std::string p_v_t_corner;
      std::string voltage_threshold;
      if(std::filesystem::equivalent(source_file_path.parent_path(), device_data_dir_path)) {
        // this means that the vpr.xml is inside the source_device_data_path itself, so this is the
        // "default" variant and does not have any p_v_t_corner or voltage_threshold defined.
      }
      else {
        // get the dir-name component of the path, this is the p_v_t_corner
        p_v_t_corner = source_file_path.parent_path().filename().string();
      
        // get the dir-name component of the parent of the parent of the path, this is the voltage_threshold
        voltage_threshold = source_file_path.parent_path().parent_path().filename().string();
      }

      std::filesystem::path vpr_xml_filepath;
      // if the file is encrypted, we need to decrypt it first
      if(source_file_path.filename() == "vpr.xml.en") {
        vpr_xml_filepath = compiler->GenerateTempFilePath();

        std::filesystem::path m_cryptdbPath = 
            CRFileCryptProc::getInstance()->getCryptDBFileName(device_data_dir_path.string(),
                                                              family + "_" + foundry + "_" + node);

        if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath.string())) {
          compiler->Message("load cryptdb failed!");
          compiler->CleanTempFiles();
          return TCL_ERROR;
        }

        if (!CRFileCryptProc::getInstance()->decryptFile(source_file_path, vpr_xml_filepath)) {
          compiler->ErrorMessage("decryption failed!");
          compiler->CleanTempFiles();
          return TCL_ERROR;
        }
      }
      else if(source_file_path.filename() == "vpr.xml") {
        vpr_xml_filepath = source_file_path;
      }
      else {
        // should never get here.
        compiler->CleanTempFiles();
        return TCL_ERROR;
      }

      // open file with Qt
      // qDebug() << "vpr xml" << QString::fromStdString(vpr_xml_filepath.string());
      QFile file(vpr_xml_filepath.string().c_str());
      if (!file.open(QFile::ReadOnly)) {
        compiler->ErrorMessage("Cannot open file: " + vpr_xml_filepath.string());
        compiler->CleanTempFiles();
        return TCL_ERROR;
      }

      // parse as XML with Qt
      QDomDocument doc;
      if (!doc.setContent(&file)) {
        file.close();
        compiler->ErrorMessage("Incorrect file: " + vpr_xml_filepath.string());
        compiler->CleanTempFiles();
        return TCL_ERROR;
      }
      file.close();
      compiler->CleanTempFiles(); // the file is not needed anymore.

      // get all "fixed_layout" tag elements
      QStringList listOfFixedLayout;
      QDomNodeList nodes = doc.elementsByTagName("fixed_layout");
      for(int i = 0; i < nodes.count(); i++) {
          QDomNode node = nodes.at(i);
          if(node.isElement()) {
              // get the "name" attribute for the "fixed_layout" tag element
              QString fixed_layout_value = node.toElement().attribute("name", "notfound");
              listOfFixedLayout.append(fixed_layout_value);
          }
      }

      // dir path to store the fpga_io_map xml files:
      std::filesystem::path device_data_fpga_io_map_dir_path = 
          device_data_dir_path / "fpga_io_map";

      // ensure that the dir path to store the fpga_io_map xml is created if not existing:
      std::filesystem::create_directories(device_data_fpga_io_map_dir_path,
                                          ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create directory: ") + device_data_fpga_io_map_dir_path.string());
        return TCL_ERROR;
      }

      // same for dir path to store the fpga_io_map xml files in source device data dir if specified:
      std::filesystem::path source_device_data_fpga_io_map_dir_path;
      if(!source_device_data_dir_path_c.empty()) {
        source_device_data_fpga_io_map_dir_path = 
            source_device_data_dir_path_c / "fpga_io_map";

        // ensure that the dir path to store the fpga_io_map xml is created if not existing:
        std::filesystem::create_directories(source_device_data_fpga_io_map_dir_path,
                                            ec);
        if(ec) {
          // error
          compiler->ErrorMessage(std::string("failed to create directory: ") + source_device_data_fpga_io_map_dir_path.string());
          return TCL_ERROR;
        }
      }

      // we don't need this yet.
      // std::filesystem::path current_design_dir = std::filesystem::current_path();
      // qDebug() << "current_design_dir: " << QString::fromStdString(current_design_dir.string());

      std::filesystem::path current_working_dir = compiler->m_projManager->projectPath();
      // qDebug() << "current_working_dir: " << QString::fromStdString(current_working_dir.string());

      // reference design to use for the fpga_io_map xml generation
      // this is guarnateed to be present in the scripts/and2/ path (refer CMakeLists.txt)
      std::filesystem::path source_and2_design_dir = compiler->GetSession()->Context()->DataPath() /
                                                      std::filesystem::path("..") /
                                                      std::filesystem::path("scripts") /
                                                      std::filesystem::path("and2");
      // qDebug() << "source_and2_design_dir: " << QString::fromStdString(source_and2_design_dir.string());

      // we need to copy the "and2/" dir **content** as is into the current working directory
      std::filesystem::path target_and2_design_dir = current_working_dir;
      // ensure that the dir path to store the and2/ files is created if not existing:
      std::filesystem::create_directories(target_and2_design_dir,
                                          ec);

      // copy the "and2" dir **content** from source(scripts/) into the current project path,
      // because, when foedag actually executes a command, it will first change dir
      // to the current project path (working directory), and then execute it.
      // the command that we want to execute would be "aurora --batch --script and2.tcl"
      // so "and2" content should be present in the project path itself.
      for (const std::filesystem::directory_entry& dir_entry :
          std::filesystem::directory_iterator(source_and2_design_dir,
                                              std::filesystem::directory_options::skip_permission_denied,
                                              ec))
      {
        if(ec) {
          // error
          compiler->ErrorMessage(std::string("failed listing contents of ") +  source_and2_design_dir.string());
          return TCL_ERROR;
        }
        if(dir_entry.is_regular_file(ec)) {

          // MinGW g++ bug? overwrite_existing, still throws error if it exists? hence the check below.
          if(FileUtils::FileExists(std::filesystem::path(target_and2_design_dir / dir_entry.path().filename()))) {
            std::filesystem::remove(target_and2_design_dir / dir_entry.path().filename());
          }
          std::filesystem::copy_file(dir_entry.path().string(),
                                      target_and2_design_dir / dir_entry.path().filename(),
                                      std::filesystem::copy_options::overwrite_existing,
                                      ec);
          if(ec) {
            std::cout << "error number : " << ec.value() << "\n"
                      << "     message : " << ec.message() << "\n"
                      << "    category : " << ec.category().name() << "\n"
                      << std::endl;
            // error
            compiler->ErrorMessage(std::string("failed to copy: ") + dir_entry.path().string() + "\n" +
                                    std::string("to: ") + target_and2_design_dir.string());
            return TCL_ERROR;
          }
        }
      }

      // qDebug() << QString::fromStdString(family);
      // qDebug() << QString::fromStdString(foundry);
      // qDebug() << QString::fromStdString(node);
      // qDebug() << QString::fromStdString(voltage_threshold);
      // qDebug() << QString::fromStdString(p_v_t_corner);
      for(QString fixed_layout_value: listOfFixedLayout) {

        // qDebug() << fixed_layout_value;
        compiler->Message(std::string("\n>>>> processing: ") +
                          compiler->DeviceString(family, foundry, node, voltage_threshold, p_v_t_corner) +
                          "," +
                          fixed_layout_value.toStdString() +
                          std::string("\n"));

        // testing
        //if(fixed_layout_value == QString("base")) continue; // exclude filter
        // if( (fixed_layout_value != QString("4x4")) &&
        //     (fixed_layout_value != QString("8x8"))) continue; // include filter
        // if(fixed_layout_value != QString("76x76")) continue; // include filter
        // if(!voltage_threshold.empty() && !p_v_t_corner.empty()) continue; // include filter

        // form the expected device_data_fpga_io_map file name
        std::string device_data_fpga_io_map_filename = family +
                                                        std::string("_") +
                                                        foundry +
                                                        std::string("_") +
                                                        node;
        if(!voltage_threshold.empty()) {
          device_data_fpga_io_map_filename += std::string("_") +
                                              voltage_threshold;
        }
        if(!p_v_t_corner.empty()) {
          device_data_fpga_io_map_filename += std::string("_") +
                                              p_v_t_corner;
        }
        device_data_fpga_io_map_filename += std::string("_") +
                                        fixed_layout_value.toStdString() +
                                        std::string("_fpga_io_map.xml");
        
        // fpga_io_map xml should finally be here in the device data dir
        std::filesystem::path device_data_fpga_io_map_filepath = device_data_fpga_io_map_dir_path / device_data_fpga_io_map_filename;
        // qDebug() << "device_data_fpga_io_map_filepath: " << QString::fromStdString(device_data_fpga_io_map_filepath.string());

        // fpga_io_map xml can also be output to the 'source' device data for future use if specified:
        std::filesystem::path source_device_data_fpga_io_map_filepath;
        if(!source_device_data_fpga_io_map_dir_path.empty()) {
          source_device_data_fpga_io_map_filepath = 
              source_device_data_fpga_io_map_dir_path / device_data_fpga_io_map_filename;
        }

        if(FileUtils::FileExists(device_data_fpga_io_map_filepath)) {
          // the fpga_io_map xml seems to be already generated and present, so skip this fixed_layout combo.
          // if we really need to regenerate the xml files, add the 'force' option!
          if(force) {
            compiler->Message("\nWARNING: The fpga_io_map xml already exists.");
            compiler->Message(std::string("device_data_fpga_io_map_filename:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filename);
            compiler->Message(std::string("target device_data_fpga_io_map_filepath:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filepath.string());
            compiler->Message("'force' has been specified, this will overwrite the fpga_io_map xml.");
            compiler->Message("\n");
          }
          else {
            compiler->Message("\n");
            compiler->Message("\nWARNING: The fpga_io_map xml already exists, skip this layout!");
            compiler->Message(std::string("device_data_fpga_io_map_filename:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filename);
            compiler->Message(std::string("target device_data_fpga_io_map_filepath:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filepath.string());
            compiler->Message("Please specify 'force' to overwrite the fpga_io_map xml.");
            compiler->Message("Please enter command in the format:\n"
                              "    generate_fpga_io_map <family> <foundry> <node> [source_device_data_dir_path] [force]");
            compiler->Message("\n");
            continue; // with the next 'fixed_layout'
          }
        }
        else {
            compiler->Message("\nNew fpga_io_map xml will be generated.");
            compiler->Message(std::string("device_data_fpga_io_map_filename:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filename);
            compiler->Message(std::string("target device_data_fpga_io_map_filepath:") +
                              std::string("\n    ") +
                              device_data_fpga_io_map_filepath.string());
            compiler->Message("\n");
        }

        // fpga_io_map xml will be generated using and2 design after aurora flow here
        // the extra "and2" in the path below is because, again, foedag will change to 
        // the corresponding "working_directory" when actually executing the command.
        std::filesystem::path and2_fpga_io_map_filepath = target_and2_design_dir / "and2" / device_data_fpga_io_map_filename;
        // qDebug() << "and2_fpga_io_map_filepath: " << QString::fromStdString(and2_fpga_io_map_filepath.string());

        // read "and2" project json template
        std::filesystem::path and2_json_template = target_and2_design_dir / "and2.json.in";
        // qDebug() << "and2_json_template: " << QString::fromStdString(and2_json_template.string());
        std::ifstream stream(and2_json_template.string());
        if (!stream.good()) {
          compiler->ErrorMessage("Cannot find and2_json_template: " +
                                std::string(and2_json_template.string()));
          return TCL_ERROR;
        }
        std::string and2_json_content((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
        stream.close();

        // qDebug() << "\n\n\n";
        // qDebug() << QString::fromStdString(and2_json_content);
        // qDebug() << "\n\n\n";

        // update json content with actual variable values of current device and layout (size)
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_FAMILY}", family);
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_FOUNDRY}", foundry);
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_NODE}", node);
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_VOLTAGE_THRESHOLD}", voltage_threshold);
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_P_V_T_CORNER}", p_v_t_corner);
        and2_json_content = compiler->ReplaceAll(and2_json_content, "${DEVICE_SIZE}", fixed_layout_value.toStdString());
        
        // now save updated json file
        std::filesystem::path and2_json = target_and2_design_dir / "and2.json";
        std::ofstream ofs(and2_json.string());
        ofs << and2_json_content;
        ofs.close();

        // invoke aurora flow for the and2 design, using the updated json
        std::string command = std::string("aurora") +
                                std::string(" --batch") +
                                std::string(" --mute") +
                                std::string(" --script") +
                                std::string(" ") +
                                std::string("and2.tcl");
        int status = compiler->ExecuteAndMonitorSystemCommand(command);
        compiler->CleanTempFiles();
        if (status) {
          compiler->Message("oops! aurora2 flow to generate fpga_io_map failed!");
          std::filesystem::remove(and2_json);
          return TCL_ERROR;
        }

        // copy generated fpga io map from the and2 dir into the device data dir.
        // MinGW g++ bug? overwrite_existing, still throws error if it exists? hence the check below.
        if(FileUtils::FileExists(device_data_fpga_io_map_filepath)) {
          std::filesystem::remove(device_data_fpga_io_map_filepath);
        }
        std::filesystem::copy_file(and2_fpga_io_map_filepath,
                                    device_data_fpga_io_map_filepath,
                                    std::filesystem::copy_options::overwrite_existing,
                                    ec);
        if(ec) {
          std::cout << "error number : " << ec.value() << "\n"
                    << "     message : " << ec.message() << "\n"
                    << "    category : " << ec.category().name() << "\n"
                    << std::endl;
          // error
          compiler->ErrorMessage(std::string("failed to copy: ") + and2_fpga_io_map_filepath.string() + "\n" +
                                  std::string("to: ") + device_data_fpga_io_map_filepath.string());
          return TCL_ERROR;
        }

        // also copy the fpga io map xml to the source device data dir if specified:
        if(!source_device_data_fpga_io_map_filepath.empty()) {
          // copy generated fpga io map from the and2 dir into the 'source' device data dir.
          // MinGW g++ bug? overwrite_existing, still throws error if it exists? hence the check below.
          if(FileUtils::FileExists(source_device_data_fpga_io_map_filepath)) {
            std::filesystem::remove(source_device_data_fpga_io_map_filepath);
          }
          std::filesystem::copy_file(and2_fpga_io_map_filepath,
                                      source_device_data_fpga_io_map_filepath,
                                      std::filesystem::copy_options::overwrite_existing,
                                      ec);
          if(ec) {
            std::cout << "error number : " << ec.value() << "\n"
                      << "     message : " << ec.message() << "\n"
                      << "    category : " << ec.category().name() << "\n"
                      << std::endl;
            // error
            compiler->ErrorMessage(std::string("failed to copy: ") + and2_fpga_io_map_filepath.string() + "\n" +
                                    std::string("to: ") + source_device_data_fpga_io_map_filepath.string());
            return TCL_ERROR;
          }
        }

        // finally delete the generated json for this combo.
        std::filesystem::remove(and2_json);
      }
      // qDebug() << "\n\n";
    }

    return TCL_OK;
  };
  interp->registerCmd("generate_fpga_io_map", generate_fpga_io_map, this, 0);

  auto list_devices = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {

    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;

    std::vector<std::string> device_list = compiler->ListDevices();

    // save std::ios settings.
    std::ios ios_default_state(nullptr);
    ios_default_state.copyfmt(std::cout);

    std::cout << std::endl;
    std::cout << "devices available:" << std::endl;
    std::cout << "<family>,<foundry>,<node>,[voltage_threshold],[p_v_t_corner]" << std::endl;
    int index = 1;
    for (auto device_variant: device_list) {
      std::cout << std::setw(4)
                << std::setfill(' ')
                << index;
      // restore cout state
      std::cout.copyfmt(ios_default_state);
      std::cout << ". " 
                << device_variant 
                << std::endl;
      index++;
    }
    std::cout << std::endl;

    return TCL_OK;
  };
  interp->registerCmd("list_devices", list_devices, this, 0);

  // note: we invoke these steps using the base class compiler.
  //       this is so that, the base class status is reflected correctly as well.
  auto route_and_sta = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "clean") {
          compiler->RouteOpt(Compiler::RoutingOpt::Clean);
          compiler->TimingAnalysisOpt(Compiler::STAOpt::Clean);
      } else {
          compiler->ErrorMessage("Unknown option: " + arg);
      }
    }
    // route
    bool status = compiler->Compile(Action::Routing);
    // if route was ok, do STA
    if(status == true) {
      status = compiler->Compile(Action::STA);
    }

    if(status == false) {
      return TCL_ERROR;
    }
    
    return TCL_OK;
  };
  interp->registerCmd("route_and_sta", route_and_sta, this, 0);

  auto listdir = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {

    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
    
    if (argc != 2) {
      compiler->ErrorMessage("provide the dirpath!");
      return TCL_ERROR;
    }

    std::string in_filepath = argv[1];

    compiler->Message("");
    compiler->Message("listing files in: " + in_filepath);

    // using Qt
    compiler->Message("");
    compiler->Message("");
    compiler->Message(" >>> Qt");
    QDirIterator allFilesIterator_xml(QString::fromStdString(in_filepath),
                                      QStringList() << "*.xml",
                                      QDir::Files,
                                      QDirIterator::Subdirectories);
    while (allFilesIterator_xml.hasNext()) {
      compiler->Message(allFilesIterator_xml.next().toStdString());
    }
    compiler->Message("");
    
    QDirIterator allFilesIterator_xmlen(QString::fromStdString(in_filepath),
                                        QStringList() << "*.xml.en",
                                        QDir::Files,
                                        QDirIterator::Subdirectories);
    while (allFilesIterator_xmlen.hasNext()) {
      compiler->Message(allFilesIterator_xmlen.next().toStdString());
    }
    compiler->Message("");

    QDirIterator allFilesIterator_db(QString::fromStdString(in_filepath),
                                     QStringList() << "*.db",
                                     QDir::Files,
                                     QDirIterator::Subdirectories);
    while (allFilesIterator_db.hasNext()) {
      compiler->Message(allFilesIterator_db.next().toStdString());
    }


    // using std:: C++17
    compiler->Message("");
    compiler->Message("");
    compiler->Message(" >>> c++17");
    std::error_code ec;
    std::vector<std::filesystem::path> xml_files;
    std::vector<std::filesystem::path> xml_en_files;
    std::vector<std::filesystem::path> db_files;
    for (const std::filesystem::directory_entry& dir_entry : 
        std::filesystem::recursive_directory_iterator(in_filepath, 
                                                      std::filesystem::directory_options::skip_permission_denied,
                                                      ec))
    {
        if(!ec) {
            // no error, proceed
            if(dir_entry.is_regular_file(ec)) {
              if(!ec) {
                // no error, proceed
                if (std::regex_match(dir_entry.path().filename().string(), std::regex(".+\\.xml", std::regex::icase))) {
                  xml_files.push_back(dir_entry.path().string());
                }
                if (std::regex_match(dir_entry.path().filename().string(), std::regex(".+\\.xml.en", std::regex::icase))) {
                  xml_en_files.push_back(dir_entry.path().string());
                }
                if (std::regex_match(dir_entry.path().filename().string(), std::regex(".+\\.db", std::regex::icase))) {
                  db_files.push_back(dir_entry.path().string());
                }
              }
            }
        }
        else {
          compiler->ErrorMessage(std::string("failed listing contents of ") + in_filepath );
        }
    }

    for (auto file_path:  xml_files) {
      compiler->Message(file_path.string());
    }
    compiler->Message("");

    for (auto file_path:  xml_en_files) {
      compiler->Message(file_path.string());
    }
    compiler->Message("");

    for (auto file_path:  db_files) {
      compiler->Message(file_path.string());
    }

    return TCL_OK;
  };
  interp->registerCmd("listdir", listdir, this, 0);

  return true;
}

std::pair<bool, std::string> CompilerOpenFPGA_ql::IsDeviceSizeCorrect(
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

bool CompilerOpenFPGA_ql::VerifyTargetDevice() const {
  const bool target = Compiler::VerifyTargetDevice();
  const bool archFile = FileUtils::FileExists(m_architectureFile);
  return target || archFile;
}

bool CompilerOpenFPGA_ql::IPGenerate() {
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
  if (!HasIPInstances()) {
    // No instances configured, no-op w/o error
    return true;
  }
#endif // #if UPSTREAM_UNUSED
  PERF_LOG("IPGenerate has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "IP generation for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
//  bool status = GetIPGenerator()->Generate();
  bool status = true;
// this is only an example of using python process - as this is not needed currently, it
// is disabled.
//   // placeholder for ipgenerate process ++
//   std::string settings_json_filename = m_projManager->projectName() + ".json";
//   std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
//   GetSession()->GetSettings()->loadJsonFile(QString::fromStdString(settings_json_path));
//   json settings_general_device_obj = GetSession()->GetSettings()->getJson()["general"]["device"];
  

//   std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
//   std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
//   std::string node = settings_general_device_obj["node"]["default"].get<std::string>();

//   // use script from project dir:
//   //std::filesystem::path python_script_path = std::filesystem::path(std::filesystem::current_path() / std::string("example.py"));
//   // use script from scripts dir: try getting from environment variable
//   const char* const path_scripts = std::getenv("AURORA2_SCRIPTS_DIR"); // this is from setup.sh
//   std::filesystem::path scriptsDir;
//   std::error_code ec;
//   if (path_scripts != nullptr) {
//     std::filesystem::path dirpath = std::string(path_scripts);
//     if (std::filesystem::exists(dirpath, ec)) {
//       scriptsDir = dirpath;
//     }
//   }

//   // proceed if we have a valid scripts directory
//   if (!scriptsDir.empty()) {
//     std::filesystem::path python_script_path =
//         std::filesystem::path(scriptsDir / std::string("example.py"));
//     std::string command = std::string("python3") + std::string(" ") +
//                           python_script_path.string() + std::string(" ") +
//                           std::string("IPGenerate") + std::string(" ") +
//                           m_projManager->projectName();

//     int status = ExecuteAndMonitorSystemCommand(command);
//     CleanTempFiles();
//     if (status) {
//       ErrorMessage("Design " + m_projManager->projectName() +
//                    " IP generation failed!");
//       return false;
//     }
//   }
//   // placeholder for ipgenerate process --

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

bool CompilerOpenFPGA_ql::DesignChanged(
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

  // check if there are changes to the settings json
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::filesystem::path settings_json_path = 
      std::filesystem::path(ProjManager()->projectPath()) /
      ".." /
      settings_json_filename;
  time_t tf = FileUtils::Mtime(settings_json_path);
  if ((tf > time_netlist) || (tf == -1)) {
      result = true;
  }
  
  std::filesystem::current_path(path);
  return result;
}

bool CompilerOpenFPGA_ql::Analyze() {
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
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED

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

bool CompilerOpenFPGA_ql::Synthesize() {
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
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED
  PERF_LOG("Synthesize has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Synthesis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
  std::string yosysScript = InitSynthesisScript();

  // read settings -> SynthArray of Objects
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path));

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first) {
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
#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

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

    std::string macros = "";
	std::string includes = "";
#if UPSTREAM_UNUSED
    std::string macros = "verilog_defines ";
    for (auto& macro_value : ProjManager()->macroList()) {
      macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    }
    macros += "\n";
    std::string includes;
    for (auto path : ProjManager()->includePathList()) {
      includes += "-I" + AdjustPath(path) + " ";
    }
#endif // #if UPSTREAM_UNUSED

       std::string designFiles;
    for (const auto& lang_file : ProjManager()->DesignFiles()) {
      std::string filesScript =
          "read_verilog ${READ_VERILOG_OPTIONS} ${INCLUDE_PATHS} "
          "${VERILOG_FILES}";
      std::string lang;

      auto files = lang_file.second + " ";
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
	  std::string options = lang;
      options += " -nolatches";
      filesScript = ReplaceAll(filesScript, "${READ_VERILOG_OPTIONS}", options);
      filesScript = ReplaceAll(filesScript, "${INCLUDE_PATHS}", includes);
      filesScript = ReplaceAll(filesScript, "${VERILOG_FILES}", files);

      designFiles += filesScript + "\n";
    }
	yosysScript =
        ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", macros + designFiles);
  }

  yosysScript = ReplaceAll(yosysScript, "${PLUGIN_LOAD}", std::string("plugin -i ql-qlf"));

  yosysScript = ReplaceAll(yosysScript, "${QL_SYNTH_PASS_NAME}", std::string("synth_quicklogic"));

#if(AURORA_USE_TABBYCAD == 1)
// check if the env variable: YOSYSHQ_LICENSE is defined
  bool use_tabbycad_for_synthesis = false;
  const char* const yosyshq_license = std::getenv("YOSYSHQ_LICENSE");
  if (yosyshq_license != nullptr) {
    std::filesystem::path path_yosyshq_license = std::string(yosyshq_license);
    if(FileUtils::FileExists(path_yosyshq_license)) {
      // if the env variable is defined, and license file path is ok, use Tabby CAD:
      use_tabbycad_for_synthesis = true;
    }
    else {
      // if license file path is not ok, fallback to open source YosysHQ yosys.
      Message("YOSYSHQ_LICENSE is set, but license file path is invalid: " + path_yosyshq_license.string());
      Message("Using open source YosysHQ yosys instead!");
    }
  }
#endif // #if(AURORA_USE_TABBYCAD == 1)

  yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}",
                           ProjManager()->DesignTopModule());
  json settings_general_device_obj = currentSettings->getJson()["general"]["device"];
  std::string family = settings_general_device_obj["family"]["default"].get<std::string>();

  if(family == "QLF_K6N10") {
    yosysScript = ReplaceAll(yosysScript, "${FAMILY}", std::string("qlf_k6n10f"));
  }
  else if(family == "QLF_K4N8") {
    yosysScript = ReplaceAll(yosysScript, "${FAMILY}", std::string("qlf_k4n8"));
  }
  else {
    ErrorMessage("Unknown Family Specified: " + family);
    return false;
  }
  
  yosysScript = ReplaceAll(
      yosysScript, "${OUTPUT_BLIF}",
      std::string(ProjManager()->projectName() + "_post_synth.blif"));

  

  // use settings to populate yosys_options
  std::string yosys_options;
  json settings_yosys_general_obj = currentSettings->getJson()["yosys"]["general"];


  if( (settings_yosys_general_obj.contains("verilog")) && 
      (settings_yosys_general_obj["verilog"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -verilog " + std::string(m_projManager->projectName() + "_post_synth.v");
  }

  if( (settings_yosys_general_obj.contains("no_abc_opt")) && 
      (settings_yosys_general_obj["no_abc_opt"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -no_abc_opt";
  }

  if( (settings_yosys_general_obj.contains("no_adder")) && 
      (settings_yosys_general_obj["no_adder"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -no_adder";
  }

  if( (settings_yosys_general_obj.contains("no_ff_map")) && 
      (settings_yosys_general_obj["no_ff_map"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -no_ff_map";
  }

  if( (settings_yosys_general_obj.contains("no_dsp")) && 
      (settings_yosys_general_obj["no_dsp"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -no_dsp";
  }

  if( (settings_yosys_general_obj.contains("no_bram")) && 
      (settings_yosys_general_obj["no_bram"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -no_bram";
  }

  if( (settings_yosys_general_obj.contains("no_sdff")) && 
      (settings_yosys_general_obj["no_sdff"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -nosdff";
  }

  if( (settings_yosys_general_obj.contains("edif")) && 
      (settings_yosys_general_obj["edif"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -edif " + std::string(m_projManager->projectName() + ".edif");
  }

  yosysScript = ReplaceAll(yosysScript, "${YOSYS_OPTIONS}", yosys_options);

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
#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_yosysExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
    return false;
  }
#endif // #if UPSTREAM_UNUSED


  std::filesystem::path yosys_executable_path = m_yosysExecutablePath;
#if(AURORA_USE_TABBYCAD == 1)
  if(use_tabbycad_for_synthesis) {
    yosys_executable_path = GetSession()->Context()->BinaryPath() /
                            ".." /
                            "tabby" /
                            "bin" /
                            "yosys_verific";
  }
#endif // #if(AURORA_USE_TABBYCAD == 1)

  std::string command =
      yosys_executable_path.string() + " -s " +
      std::string(ProjManager()->projectName() + ".ys -l " +
                  ProjManager()->projectName() + "_synth.log");
  (*m_out) << "Synthesis command: " << command << std::endl;
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " synthesis failed!");
    return false;
  } else {
    m_state = State::Synthesized;
    (*m_out) << "Design " << ProjManager()->projectName() << " is synthesized!"
             << std::endl;
    return true;
  }
}

std::string CompilerOpenFPGA_ql::InitSynthesisScript() {
  // Default or custom Yosys script
  if (m_yosysScript.empty()) {
#if UPSTREAM_UNUSED
    m_yosysScript = basicYosysScript;
#endif // #if UPSTREAM_UNUSED

    bool use_external_template_yosys = false;
    std::string aurora_template_script_yosys;

    // check if we have the default aurora template script available:
    // at 'scripts/aurora_template_script.ys', if so, we use that.
    std::filesystem::path aurora_template_script_yosys_path =
        GetSession()->Context()->DataPath() /
        std::filesystem::path("..") /
        std::filesystem::path("scripts") /
        std::filesystem::path("aurora_template_script.ys");

    if(FileUtils::FileExists(aurora_template_script_yosys_path)) {
        
      // get it into a ifstream
      std::ifstream stream(aurora_template_script_yosys_path.string());
        
      if (stream.good()) {
        aurora_template_script_yosys = 
          std::string((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
          stream.close();
          use_external_template_yosys = true;
          
        }
    }

    if(use_external_template_yosys) {
      Message("Using External Yosys Template Script: " +
                                std::string(aurora_template_script_yosys_path.string()));
      m_yosysScript = aurora_template_script_yosys;
    }
    else {
      Message("Cannot load Yosys Template Script: " +
                                std::string(aurora_template_script_yosys_path.string()));
      Message("Using Internal Yosys Template Script.");
      m_yosysScript = qlYosysScript;
    }
  }
  return m_yosysScript;
}

std::string CompilerOpenFPGA_ql::FinishSynthesisScript(const std::string& script) {
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

std::string CompilerOpenFPGA_ql::BaseVprCommand() {

  // note: at this point, the current_path() is the project 'source' directory.

  // read settings -> SynthArray of Objects
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path));

  json settings_vpr_general_obj = currentSettings->getJson()["vpr"]["general"];
  json settings_vpr_filename_obj = currentSettings->getJson()["vpr"]["filename"];
  json settings_vpr_netlist_obj = currentSettings->getJson()["vpr"]["netlist"];
  json settings_vpr_pack_obj = currentSettings->getJson()["vpr"]["pack"];
  json settings_vpr_place_obj = currentSettings->getJson()["vpr"]["place"];
  json settings_vpr_route_obj = currentSettings->getJson()["vpr"]["route"];
  json settings_vpr_analysis_obj = currentSettings->getJson()["vpr"]["analysis"];
  json settings_vpr_custom_obj = currentSettings->getJson()["vpr"]["custom"];

  std::string vpr_options;

  // parse vpr general options
#if UPSTREAM_UNUSED
  std::string device_size = "";
  if (!m_deviceSize.empty()) {
    device_size = " --device " + m_deviceSize;
  }
#endif // #if UPSTREAM_UNUSED
  if (!m_deviceSize.empty()) {
    vpr_options += std::string(" --device") + 
                    std::string(" ") + 
                    m_deviceSize;
  }
  else if(settings_vpr_general_obj.contains("device")) {
    vpr_options += std::string(" --device") + 
                    std::string(" ") + 
                    settings_vpr_general_obj["device"]["default"].get<std::string>();
  }

  if(settings_vpr_general_obj.contains("timing_analysis")) {
    vpr_options += std::string(" --timing_analysis");
    if(settings_vpr_general_obj["timing_analysis"]["default"].get<std::string>() == "checked") {
      vpr_options += std::string(" on");
    }
    else {
      vpr_options += std::string(" off");
    }
  }

  if(settings_vpr_general_obj.contains("constant_net_method")) {
    vpr_options += std::string(" --constant_net_method") + 
                   std::string(" ") + 
                   settings_vpr_general_obj["constant_net_method"]["default"].get<std::string>();
  }

  if(settings_vpr_general_obj.contains("clock_modeling")) {
    vpr_options += std::string(" --clock_modeling") + 
                   std::string(" ") + 
                   settings_vpr_general_obj["clock_modeling"]["default"].get<std::string>();
  }

  if(settings_vpr_general_obj.contains("exit_before_pack")) {
    vpr_options += std::string(" --exit_before_pack");
    if(settings_vpr_general_obj["exit_before_pack"]["default"].get<std::string>() == "checked") {
      vpr_options += std::string(" on");
    }
    else {
      vpr_options += std::string(" off");
    }
  }

  // parse vpr filename options
  if(settings_vpr_filename_obj.contains("circuit_format")) {
    vpr_options += std::string(" --circuit_format") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["circuit_format"]["default"].get<std::string>();
  }

  if( (settings_vpr_filename_obj.contains("net_file")) && 
      !settings_vpr_filename_obj["net_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --net_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["net_file"]["default"].get<std::string>();
  }

  if( (settings_vpr_filename_obj.contains("place_file")) && 
      !settings_vpr_filename_obj["place_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --place_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["place_file"]["default"].get<std::string>();
  }

  if( (settings_vpr_filename_obj.contains("route_file")) && 
      !settings_vpr_filename_obj["route_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --route_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["route_file"]["default"].get<std::string>();
  }


  // ---------------------------------------------------------------- sdc_file ++
  //Message(std::string("currentpath:") + std::filesystem::current_path().string());
  std::filesystem::path sdc_file_path;
  bool sdc_file_path_from_json = false;
  // check if an sdc file is specified in the json:
  if( (settings_vpr_filename_obj.contains("sdc_file")) && 
      !settings_vpr_filename_obj["sdc_file"]["default"].get<std::string>().empty() ) {

    sdc_file_path = 
        std::filesystem::path(settings_vpr_filename_obj["sdc_file"]["default"].get<std::string>());

    sdc_file_path_from_json = true;

    //Message(std::string("[1]") + sdc_file_path.string());
  }
  // check if an sdc file exists with the project name (projectName.sdc) in the project source dir:
  else {

    sdc_file_path = 
      std::filesystem::path(m_projManager->projectName() + std::string(".sdc"));

    //Message(std::string("[2]") + sdc_file_path.string());
  }

  // convert to canonical path, which will also check that the path exists.
  std::error_code ec;
  std::filesystem::path sdc_file_path_c = std::filesystem::canonical(sdc_file_path, ec);
  if(!ec) {
    // path exists, and can be used
    vpr_options += std::string(" --sdc_file") + 
                   std::string(" ") + 
                   sdc_file_path_c.string();
  }
  else {
    // path does not exist, we got a filesystem error while making the canonical path.

    if(sdc_file_path_from_json) {
      // if the sdc_file comes from the json, and it is not found, that is an error.

      ErrorMessage(std::string("sdc file from json: ") + sdc_file_path.string() + std::string(" does not exist!!"));

      // empty string returned on error.
      return std::string("");
    }

    // otherwise, we just have a warning for the user, and proceed.
    Message(std::string("no sdc file found, skipping this vpr option!"));
  }
  // ---------------------------------------------------------------- sdc_file --


  if( (settings_vpr_filename_obj.contains("write_rr_graph")) && 
      !settings_vpr_filename_obj["write_rr_graph"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --write_rr_graph") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["write_rr_graph"]["default"].get<std::string>();
  }

  // parse vpr netlist options
  if(settings_vpr_netlist_obj.contains("absorb_buffer_luts")) {
    vpr_options += std::string(" --absorb_buffer_luts");
    if(settings_vpr_netlist_obj["absorb_buffer_luts"]["default"].get<std::string>() == "checked") {
      vpr_options += std::string(" on");
    }
    else {
      vpr_options += std::string(" off");
    }
  }

  // parse vpr pack options: nothing here

  // parse vpr place options: nothing here

  // parse vpr route options
  if( (settings_vpr_route_obj.contains("route_chan_width")) && 
      !settings_vpr_route_obj["route_chan_width"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --route_chan_width") + 
                   std::string(" ") + 
                   settings_vpr_route_obj["route_chan_width"]["default"].get<std::string>();

    // fallback values - should this be enforced if empty in the settings JSON?
    // if(family == "QLF_K6N10") {
    //   vpr_options += std::string(" --route_chan_width") + std::string(" ") + std::string("180")
    // }
    // else if(family == "QLF_K4N8") {
    //   vpr_options += std::string(" --route_chan_width") + std::string(" ") + std::string("60")
    // }
  }

  // parse vpr analysis options
  if(settings_vpr_analysis_obj.contains("gen_post_synthesis_netlist")) {
    vpr_options += std::string(" --gen_post_synthesis_netlist");
    if(settings_vpr_analysis_obj["gen_post_synthesis_netlist"]["default"].get<std::string>() == "checked") {
      vpr_options += std::string(" on");
    }
    else {
      vpr_options += std::string(" off");
    }
  }

  if( (settings_vpr_analysis_obj.contains("post_synth_netlist_unconn_inputs")) && 
      !settings_vpr_analysis_obj["post_synth_netlist_unconn_inputs"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --post_synth_netlist_unconn_inputs") + 
                   std::string(" ") + 
                   settings_vpr_analysis_obj["post_synth_netlist_unconn_inputs"]["default"].get<std::string>();
  }

  if( (settings_vpr_analysis_obj.contains("post_synth_netlist_unconn_outputs")) && 
      !settings_vpr_analysis_obj["post_synth_netlist_unconn_outputs"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --post_synth_netlist_unconn_outputs") + 
                   std::string(" ") + 
                   settings_vpr_analysis_obj["post_synth_netlist_unconn_outputs"]["default"].get<std::string>();
  }

  if( (settings_vpr_analysis_obj.contains("timing_report_npaths")) && 
      !settings_vpr_analysis_obj["timing_report_npaths"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --timing_report_npaths") + 
                   std::string(" ") + 
                   settings_vpr_analysis_obj["timing_report_npaths"]["default"].get<std::string>();
  }

  // custom vpr command-line options, it is upto the user to ensure that the options are passed in correctly.
  if( (settings_vpr_custom_obj.contains("custom_vpr_options_str")) && 
      !settings_vpr_custom_obj["custom_vpr_options_str"]["default"].get<std::string>().empty() ) {
    // first, trim the entire string to eliminate any extra whitespace in the front and the back
    std::string vpr_custom_options_string = settings_vpr_custom_obj["custom_vpr_options_str"]["default"].get<std::string>();
    vpr_custom_options_string = StringUtils::trim(vpr_custom_options_string);
    // add the options string to the end of the vpr options with one whitespace separator
    vpr_options += std::string(" ") + vpr_custom_options_string;
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
    switch (lang_file.first) {
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
#if UPSTREAM_UNUSED
  std::string pnrOptions;
  if (!PnROpt().empty()) pnrOptions += " " + PnROpt();
  if (!PerDevicePnROptions().empty()) pnrOptions += " " + PerDevicePnROptions();
#endif // #if UPSTREAM_UNUSED

  json settings_general_device_obj = currentSettings->getJson()["general"]["device"];

  std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
  std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
  std::string node = settings_general_device_obj["node"]["default"].get<std::string>();
  std::string voltage_threshold = "";
  std::string p_v_t_corner = "";

  std::filesystem::path device_base_dir_path = 
      std::filesystem::path(GetSession()->Context()->DataPath() /
                            family /
                            foundry /
                            node);
  
  std::filesystem::path device_variant_base_dir_path = device_base_dir_path;

  // optional: if voltage_threshold and p_v_t_corner are specified in the JSON, 
  //           take the XML file specific to that combination:
  if( settings_general_device_obj.contains("voltage_threshold") &&
      settings_general_device_obj.contains("p_v_t_corner") ) {
    voltage_threshold = 
        settings_general_device_obj["voltage_threshold"]["default"].get<std::string>();
    p_v_t_corner = 
            settings_general_device_obj["p_v_t_corner"]["default"].get<std::string>();
    if(!voltage_threshold.empty() &&
       !p_v_t_corner.empty()) {
        device_variant_base_dir_path = 
            std::filesystem::path(GetSession()->Context()->DataPath() /
                                  family /
                                  foundry /
                                  node /
                                  voltage_threshold /
                                  p_v_t_corner);
    }
  }

  std::string device = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner);

  // check if the target device exists
  if(!DeviceExists(device)) {
    ErrorMessage("Device does not exist in the installation: " + device);
      // empty string returned on error.
      return std::string("");
  }

  // prefer to use the unencrypted file, if available.
  m_architectureFile = 
      std::filesystem::path(device_variant_base_dir_path / std::string("vpr.xml"));

  // if not, use the encrypted file after decryption.
  if (!std::filesystem::exists(m_architectureFile, ec)) {

    std::filesystem::path vpr_xml_en_path = 
          std::filesystem::path(device_variant_base_dir_path / std::string("vpr.xml.en"));
    m_architectureFile = GenerateTempFilePath();

    m_cryptdbPath = 
        CRFileCryptProc::getInstance()->getCryptDBFileName(device_base_dir_path.string(),
                                                           family + "_" + foundry + "_" + node);

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath.string())) {
      Message("load cryptdb failed!");
      // empty string returned on error.
      return std::string("");
    }

    if (!CRFileCryptProc::getInstance()->decryptFile(vpr_xml_en_path, m_architectureFile)) {
      ErrorMessage("decryption failed!");
      // empty string returned on error.
      return std::string("");
    }
  }

  Message(std::string("Using vpr.xml for: ") + device );

  // add the *internal* option to allow dangling nodes in the logic.
  // ref: https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/a7f573b7a5432711042ddeb9f2958cd035097a10/vpr/src/timing/timing_graph_builder.cpp#L277
  // this is a workaround, to avoid putting timing arcs for static input ports.
  vpr_options += " --allow_dangling_combinational_nodes on";

  // construct the base vpr command with all the options here.
#if UPSTREAM_UNUSED
  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile + std::string(" --sdc_file ") +
                  std::string(ProjManager()->projectName() + "_openfpga.sdc") +
                  std::string(" --route_chan_width ") +
                  std::to_string(m_channel_width) + device_size + pnrOptions);
  return command;
#endif // #if UPSTREAM_UNUSED
  std::string base_vpr_command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile) + // NOTE: don't add a " " here as vpr options start with a " "
      vpr_options;
  
  return base_vpr_command;
}

std::string CompilerOpenFPGA_ql::BaseStaCommand() {
  std::string command =
      m_staExecutablePath.string() +
      std::string(
          " -exit ");  // allow open sta exit its tcl shell even there is error
  return command;
}

std::string CompilerOpenFPGA_ql::BaseStaScript(std::string libFileName,
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
bool CompilerOpenFPGA_ql::Packing() {
  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.net"));
    return true;
  }
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
#endif // #if UPSTREAM_UNUSED
  PERF_LOG("Packing has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Packing for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

#if UPSTREAM_UNUSED
  std::string command = BaseVprCommand() + " --pack";
#endif // #if UPSTREAM_UNUSED
  std::string command = BaseVprCommand();
  if(command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    return false;
  }
  command += std::string(" ") + 
             std::string("--pack");

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_pack.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " packing failed!");
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is packed!"
           << std::endl;

// this is only an example of using python process - as this is not needed currently, it
// is disabled.
//   // placeholder for pin_placement process ++
//   // we are already loaded, so, no need to read the json again at this point.
//   //std::string settings_json_filename = m_projManager->projectName() + ".json";
//   //std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
//   //GetSession()->GetSettings()->loadJsonFile(QString::fromStdString(settings_json_path));
//   json settings_general_device_obj = GetSession()->GetSettings()->getJson()["general"]["device"];
  

//   std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
//   std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
//   std::string node = settings_general_device_obj["node"]["default"].get<std::string>();
//   m_OpenFpgaPinMapXml = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("pinmap.xml"));
//   m_OpenFpgaPinMapCSV = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("pinmap.csv"));

//   // use script from project dir:
//   //std::filesystem::path python_script_path = std::filesystem::path(std::filesystem::current_path() / std::string("example.py"));
//   // use script from scripts dir:
//   const char* const path_scripts =
//       std::getenv("AURORA2_SCRIPTS_DIR");  // this is from setup.sh
//   std::filesystem::path scriptsDir;
//   std::error_code ec;
//   if (path_scripts != nullptr) {
//     std::filesystem::path dirpath = std::string(path_scripts);
//     if (std::filesystem::exists(dirpath, ec)) {
//       scriptsDir = dirpath;
//     }
//   }

//   // proceed if we have a valid scripts directory
//   if (!scriptsDir.empty()) {
//     std::filesystem::path python_script_path =
//         std::filesystem::path(scriptsDir / std::string("example.py"));
//     command = std::string("python3") + std::string(" ") +
//               python_script_path.string() + std::string(" ") +
//               m_OpenFpgaPinMapXml.string() + std::string(" ") +
//               m_OpenFpgaPinMapCSV.string();

//     status = ExecuteAndMonitorSystemCommand(command);
//     CleanTempFiles();
//     if (status) {
//       ErrorMessage("Design " + m_projManager->projectName() +
//                    " PinPlacement failed!");
//       return false;
//     }
//     (*m_out) << "Design " << m_projManager->projectName()
//              << " PinPlacement Done!" << std::endl;
//   }
//   // placeholder for pin_placement process --

  return true;
}

bool CompilerOpenFPGA_ql::GlobalPlacement() {
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
#if UPSTREAM_UNUSED
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED
  if (m_state != State::Packed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in packed state"));
    return false;
  }

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

bool CompilerOpenFPGA_ql::Placement() {
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
#if UPSTREAM_UNUSED
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED

  if (m_state != State::Packed && m_state != State::GloballyPlaced) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in packed/globally_placed state"));
    return false;
  }

  PERF_LOG("Placement has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Placement for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
#endif // #if UPSTREAM_UNUSED
#if UPSTREAM_UNUSED
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
    switch (lang_file.first) {
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
#endif // #if UPSTREAM_UNUSED

  // generate pin contraints file or use pre-generated .place file, if required.
  // this string should contain the path of the PinConstraints file, if generated correctly.
  // the "filepath_fpga_fix_pins_place_str" variable will be empty if:
  // - there is no pre-generated .place file AND
  // - there is no pcf file in the project.
  std::string filepath_fpga_fix_pins_place_str;
  if (!GeneratePinConstraints(filepath_fpga_fix_pins_place_str)) return false;

  std::string command = BaseVprCommand();
  if(command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    return false;
  }
  command += std::string(" ") + 
             std::string("--place");

  if (!filepath_fpga_fix_pins_place_str.empty()) {
    command += std::string(" --fix_pins") + 
               std::string(" ") + 
               filepath_fpga_fix_pins_place_str;
  }
  else
  {
    Message("no pcf file found, skipping PinConstraints usage!");
  }

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_place.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " placement failed!");
    return false;
  }
  m_state = State::Placed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is placed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::ConvertSdcPinConstrainToPcf(
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
bool CompilerOpenFPGA_ql::Route() {
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
#if UPSTREAM_UNUSED
  if (m_state != State::Placed) {
    ErrorMessage("Design needs to be in placed state");
    return false;
  }
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED

  if (m_state != State::Placed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in placed state"));
    return false;
  }
  
  PERF_LOG("Route has started");
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Routing for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;
#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
     return false;
   }
#endif // #if UPSTREAM_UNUSED

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

#if UPSTREAM_UNUSED
  std::string command = BaseVprCommand() + " --route";
#endif // #if UPSTREAM_UNUSED
  std::string command = BaseVprCommand();
  if(command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    return false;
  }
  command += std::string(" ") + 
             std::string("--route");

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_route.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " routing failed!");
    return false;
  }
  m_state = State::Routed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is routed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::TimingAnalysis() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED
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

#ifdef _WIN32

// under WIN32, running the analysis stage alone causes issues, hence we call the
// route and analysis stages together
// hence, we can also be at Placed state here.
  if ( (m_state != State::Placed) && (m_state != State::Routed) ) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in placed/routed state"));
    return false;
  }

#else // #ifdef _WIN32

  if (m_state != State::Routed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in routed state"));
    return false;
  }

#endif // #ifdef _WIN32

#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
    return false;
  }
#endif // #if UPSTREAM_UNUSED

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

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED
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
    json settings_vpr_filename_obj = GetSession()->GetSettings()->getJson()["vpr"]["filename"];
    std::string vpr_options;
    std::string netlistFilePrefix = m_projManager->projectName() + "_post_synth";

    if( (settings_vpr_filename_obj.contains("net_file")) && 
        !settings_vpr_filename_obj["net_file"]["default"].get<std::string>().empty() ) {
        vpr_options += std::string(" --net_file") + 
                    std::string(" ") + 
                    settings_vpr_filename_obj["net_file"]["default"].get<std::string>();
    }
    else {
        vpr_options += std::string(" --net_file") + 
                    std::string(" ") + 
                    netlistFilePrefix + std::string(".net");
    }

    if( (settings_vpr_filename_obj.contains("place_file")) && 
        !settings_vpr_filename_obj["place_file"]["default"].get<std::string>().empty() ) {
        vpr_options += std::string(" --place_file") + 
                    std::string(" ") + 
                    settings_vpr_filename_obj["place_file"]["default"].get<std::string>();
    }
    else {
        vpr_options += std::string(" --place_file") + 
                    std::string(" ") + 
                    netlistFilePrefix + std::string(".place");
    }

    if( (settings_vpr_filename_obj.contains("route_file")) && 
        !settings_vpr_filename_obj["route_file"]["default"].get<std::string>().empty() ) {
        vpr_options += std::string(" --route_file") + 
                    std::string(" ") + 
                    settings_vpr_filename_obj["route_file"]["default"].get<std::string>();
    }
    else {
        vpr_options += std::string(" --route_file") + 
                    std::string(" ") + 
                    netlistFilePrefix + std::string(".route");
    }

    taCommand = BaseVprCommand();
    if(taCommand.empty()) {
        ErrorMessage("Base VPR Command is empty!");
        return false;
    }
    taCommand += vpr_options +
    #ifdef _WIN32
    // under WIN32, running the analysis stage along causes issues, hence we call the
    // route and analysis stages together
                std::string(" ") + 
                std::string("--route") +
    #endif // #ifdef _WIN32
                std::string(" ") + 
                std::string("--analysis");

    std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                        std::string(ProjManager()->projectName() + "_sta.cmd"))
                            .string());
    ofs << taCommand << std::endl;
    ofs.close();
  }
  int status = ExecuteAndMonitorSystemCommand(taCommand);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " timing analysis failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName()
           << " is timing analysed!" << std::endl;

#ifdef _WIN32
// under WIN32, running the analysis stage along causes issues, hence we call the
// route and analysis stages together
// hence, we set the state here, so that just sta can be called instead of route and sta as well.
  m_state = State::Routed;
#endif // #ifdef _WIN32

  return true;
}

bool CompilerOpenFPGA_ql::PowerAnalysis() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
#endif // #if UPSTREAM_UNUSED
  if (PowerAnalysisOpt() == PowerOpt::Clean) {
    Message("Cleaning PoweAnalysis results for " +
            ProjManager()->projectName());
    PowerAnalysisOpt(PowerOpt::None);
    m_state = State::Routed;
    return true;
  }
  PERF_LOG("PowerAnalysis has started");
#ifdef _WIN32
// under WIN32, running the analysis stage along causes issues, hence we call the
// route and analysis stages together
// hence, we can also be at Placed state here.
  if ( (m_state != State::Placed) && (m_state != State::Routed) ) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in placed/routed state"));
    return false;
  }
#else // #ifdef _WIN32
  if (m_state != State::Routed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in routed state"));
    return false;
  }
#endif // #ifdef _WIN32
  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "Power Analysis for design: " << ProjManager()->projectName()
           << std::endl;
  (*m_out) << "##################################################" << std::endl;

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

#if UPSTREAM_UNUSED
  std::string command = BaseVprCommand() + " --analysis";
  if (!FileUtils::FileExists(m_vprExecutablePath)) {
    ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
       return false;
     }
#endif // #if UPSTREAM_UNUSED
  
  json settings_vpr_filename_obj = GetSession()->GetSettings()->getJson()["vpr"]["filename"];
  std::string vpr_options;
  std::string netlistFilePrefix = m_projManager->projectName() + "_post_synth";

  if( (settings_vpr_filename_obj.contains("net_file")) && 
      !settings_vpr_filename_obj["net_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --net_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["net_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --net_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".net");
  }

  if( (settings_vpr_filename_obj.contains("place_file")) && 
      !settings_vpr_filename_obj["place_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --place_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["place_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --place_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".place");
  }

  if( (settings_vpr_filename_obj.contains("route_file")) && 
      !settings_vpr_filename_obj["route_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --route_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["route_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --route_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".route");
  }

  std::string command = BaseVprCommand();
  if(command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    return false;
  }
  command += vpr_options +
#ifdef _WIN32
// under WIN32, running the analysis stage alone causes issues, hence we call the
// route and analysis stages together
             std::string(" ") + 
             std::string("--route") +
#endif // #ifdef _WIN32
             std::string(" ") + 
             std::string("--analysis");

  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " power analysis failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName() << " is power analysed!"
           << std::endl;
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
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit --verbose
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

const std::string qlOpenFPGABitstreamScript = R"(
# openfpga (internal) template script for Aurora

# refer:
# 1. https://openfpga.readthedocs.io/en/latest/manual/openfpga_shell/openfpga_script/
# 2. https://openfpga.readthedocs.io/en/latest/manual/openfpga_shell/openfpga_commands/

# we need to run the vpr analysis command before openfpga process can start.
# don't edit this:
${VPR_ANALYSIS_COMMAND}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation setting
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

# Read OpenFPGA bitstream setting
${READ_OPENFPGA_BITSTREAM_SETTING_COMMAND}

# Annotate the OpenFPGA architecture to VPR data base
# to debug add '--verbose'
# to specify activity file, add '--activity_file ${ACTIVITY_FILE}'
link_openfpga_arch --sort_gsb_chan_node_in_edges

# Apply fix-up to clustering nets based on routing results
# to debug add '--verbose'
pb_pin_fixup

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
# - Enabled compression on routing architecture modules with '--compress_routing'
# - Enable pin duplication on grid modules with '--duplicate_grid_pin'
# - Create only frame views of the module graph to make it run faster with '--frame_view'
build_fabric --compress_routing --duplicate_grid_pin --frame_view

# Dump GSB data
# Necessary for creation of rr graph for SymbiFlow
write_gsb_to_xml --file gsb

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
# to use repack design contraints, add '--design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}'
repack

# Build bitstream database and save to file
build_architecture_bitstream --write_file fabric_independent_bitstream.xml

# Build fabric bitstream
build_fabric_bitstream

# Write fabric bitstream
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit

# Write fabric bitstream xml format
write_fabric_bitstream --format xml --file fabric_bitstream.xml

# Write io mapping 
write_io_mapping -f PinMapping.xml

${OPENFPGA_WRITE_FABRIC_IO_INFO_COMMAND}

# Write the SDC files for PnR backend
#write_pnr_sdc --time_unit ns --flatten_names --file ./SDC
#write_pnr_sdc --time_unit ns --flatten_names --hierarchical --file ./SDC_leaf

# Finish and exit OpenFPGA
exit
)";

const std::string qlOpenFPGApcf2placeScript = R"(
${OPENFPGA_PCF2PLACE_COMMAND}
# Finish and exit OpenFPGA
exit
)";

std::string CompilerOpenFPGA_ql::InitOpenFPGAScript() {
  // Default or custom OpenFPGA script
  if (m_openFPGAScript.empty()) {
#if UPSTREAM_UNUSED
    m_openFPGAScript = basicOpenFPGABitstreamScript;
#endif // #if UPSTREAM_UNUSED

    bool use_external_template_openfpga = false;
    std::string aurora_template_script_openfpga;

    // check if we have the default aurora template script available:
    // at 'scripts/aurora_template_script.openfpga', if so, we use that.
    std::filesystem::path aurora_template_script_openfpga_path =
        GetSession()->Context()->DataPath() /
        std::filesystem::path("..") /
        std::filesystem::path("scripts") /
        std::filesystem::path("aurora_template_script.openfpga");

    if(FileUtils::FileExists(aurora_template_script_openfpga_path)) {
        
      // get it into a ifstream
      std::ifstream stream(aurora_template_script_openfpga_path.string());
        
      if (stream.good()) {
        aurora_template_script_openfpga = 
          std::string((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
          stream.close();
          use_external_template_openfpga = true;
        }
    }

    if(use_external_template_openfpga) {
      Message("Using External OpenFPGA Template Script: " +
                                std::string(aurora_template_script_openfpga_path.string()));
      m_openFPGAScript = aurora_template_script_openfpga;
    }
    else {
      Message("Cannot load OpenFPGA Template Script: " +
                                std::string(aurora_template_script_openfpga_path.string()));
      Message("Using Internal OpenFPGA Template Script.");
      m_openFPGAScript = qlOpenFPGABitstreamScript;
    }
  }
  return m_openFPGAScript;
}

std::string CompilerOpenFPGA_ql::FinishOpenFPGAScript(const std::string& script) {
  std::string result = script;

  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path));

  json settings_general_device_obj = currentSettings->getJson()["general"]["device"];

  std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
  std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
  std::string node = settings_general_device_obj["node"]["default"].get<std::string>();
  std::string voltage_threshold = "";
  std::string p_v_t_corner = "";

  std::filesystem::path device_base_dir_path = 
      std::filesystem::path(GetSession()->Context()->DataPath() /
                            family /
                            foundry /
                            node);

  std::filesystem::path device_variant_base_dir_path = device_base_dir_path;

  // optional: if voltage_threshold and p_v_t_corner are specified in the JSON, 
  //           take the XML file specific to that combination:
  if( settings_general_device_obj.contains("voltage_threshold") &&
      settings_general_device_obj.contains("p_v_t_corner") ) {
    voltage_threshold = 
        settings_general_device_obj["voltage_threshold"]["default"].get<std::string>();
    p_v_t_corner = 
            settings_general_device_obj["p_v_t_corner"]["default"].get<std::string>();
    if(!voltage_threshold.empty() &&
       !p_v_t_corner.empty()) {
        device_variant_base_dir_path = 
            std::filesystem::path(GetSession()->Context()->DataPath() /
                                  family /
                                  foundry /
                                  node /
                                  voltage_threshold /
                                  p_v_t_corner);
    }
  }

  std::string device = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner);

  // check if the target device exists
  if(!DeviceExists(device)) {
    ErrorMessage("Device does not exist in the installation: " + device);
      // empty string returned on error.
      return std::string("");
  }

  std::error_code ec;
  // prefer to use the unencrypted file, if available.
  m_OpenFpgaArchitectureFile = 
      std::filesystem::path(device_variant_base_dir_path / std::string("openfpga.xml"));

  // this is optional:
  m_OpenFpgaBitstreamSettingFile = 
      std::filesystem::path(device_base_dir_path / std::string("bitstream_annotation.xml"));
  if(!std::filesystem::exists(m_OpenFpgaBitstreamSettingFile, ec)) {
    m_OpenFpgaBitstreamSettingFile.clear();
  }
  
  m_OpenFpgaSimSettingFile = 
      std::filesystem::path(device_base_dir_path / std::string("fixed_sim_openfpga.xml"));

  // if not, use the encrypted file after decryption.
  if (!std::filesystem::exists(m_OpenFpgaArchitectureFile, ec)) {

    // all of the xml files will be the encrypted versions.

    std::filesystem::path openfpga_xml_en_path = 
          std::filesystem::path(device_variant_base_dir_path / std::string("openfpga.xml.en"));
    m_OpenFpgaArchitectureFile = GenerateTempFilePath();

    std::filesystem::path bitstream_annotation_en_path = 
          std::filesystem::path(device_base_dir_path / std::string("bitstream_annotation.xml.en"));
    m_OpenFpgaBitstreamSettingFile = GenerateTempFilePath();

    std::filesystem::path fixed_sim_openfpga_en_path = 
          std::filesystem::path(device_base_dir_path / std::string("fixed_sim_openfpga.xml.en"));
    m_OpenFpgaSimSettingFile = GenerateTempFilePath();

    m_cryptdbPath = 
        CRFileCryptProc::getInstance()->getCryptDBFileName(device_base_dir_path.string(),
                                                           family + "_" + foundry + "_" + node);

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath.string())) {
      Message("load cryptdb failed!");
      // empty string returned on error.
      return std::string("");
    }

    if (!CRFileCryptProc::getInstance()->decryptFile(openfpga_xml_en_path, m_OpenFpgaArchitectureFile)) {
      ErrorMessage("decryption failed!");
      // empty string returned on error.
      return std::string("");
    }

    // this is optional:
    if(std::filesystem::exists(bitstream_annotation_en_path, ec)) {
      if (!CRFileCryptProc::getInstance()->decryptFile(bitstream_annotation_en_path, m_OpenFpgaBitstreamSettingFile)) {
        ErrorMessage("decryption failed!");
        // empty string returned on error.
        return std::string("");
      }
    }
    else {
      m_OpenFpgaBitstreamSettingFile.clear();
    }

    if (!CRFileCryptProc::getInstance()->decryptFile(fixed_sim_openfpga_en_path, m_OpenFpgaSimSettingFile)) {
      ErrorMessage("decryption failed!");
      // empty string returned on error.
      return std::string("");
    }
  }

  Message(std::string("Using openfpga.xml for: ") + device );



  // call vpr to execute analysis
  json settings_vpr_filename_obj = GetSession()->GetSettings()->getJson()["vpr"]["filename"];
  std::string vpr_options;
  std::string netlistFilePrefix = ProjManager()->projectName() + "_post_synth";

  if( (settings_vpr_filename_obj.contains("net_file")) && 
      !settings_vpr_filename_obj["net_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --net_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["net_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --net_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".net");
  }

  if( (settings_vpr_filename_obj.contains("place_file")) && 
      !settings_vpr_filename_obj["place_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --place_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["place_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --place_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".place");
  }

  if( (settings_vpr_filename_obj.contains("route_file")) && 
      !settings_vpr_filename_obj["route_file"]["default"].get<std::string>().empty() ) {
    vpr_options += std::string(" --route_file") + 
                   std::string(" ") + 
                   settings_vpr_filename_obj["route_file"]["default"].get<std::string>();
  }
  else {
    vpr_options += std::string(" --route_file") + 
                   std::string(" ") + 
                   netlistFilePrefix + std::string(".route");
  }

  std::string vpr_analysis_command = BaseVprCommand();
  if(vpr_analysis_command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    // empty string returned on error.
    return std::string("");
  }
  vpr_analysis_command += vpr_options +
#ifdef _WIN32
// under WIN32, running the analysis stage along causes issues, hence we call the
// route and analysis stages together
                          std::string(" ") + 
                          std::string("--route") +
#endif // #ifdef _WIN32
                          std::string(" ") + 
                          std::string("--analysis");

  result = ReplaceAll(result, "${VPR_ANALYSIS_COMMAND}", vpr_analysis_command);

  //std::string netlistFilePrefix = m_projManager->projectName() + "_post_synth";

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first) {
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
    switch (lang_file.first) {
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

  // optional, so only if this file is available, else blank command.
  std::string read_openfpga_bitstream_setting_command = "#skipped";
  if(!m_OpenFpgaBitstreamSettingFile.empty()) {
    // read_openfpga_bitstream_setting -f ${OPENFPGA_BITSTREAM_SETTING_FILE}
    read_openfpga_bitstream_setting_command = 
        std::string("read_openfpga_bitstream_setting -f ") + 
        m_OpenFpgaBitstreamSettingFile.string();
  }
  result = ReplaceAll(result, "${READ_OPENFPGA_BITSTREAM_SETTING_COMMAND}",
                      read_openfpga_bitstream_setting_command);

  result = ReplaceAll(result, "${OPENFPGA_REPACK_CONSTRAINTS}",
                      m_OpenFpgaRepackConstraintsFile.string());
  if (m_OpenFpgaFabricKeyFile == "") {
    result = ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}", "");
  } else {
    result =
        ReplaceAll(result, "${OPENFPGA_BUILD_FABRIC_OPTION}",
                   "--load_fabric_key " + m_OpenFpgaFabricKeyFile.string());
  }

  // call openfpga to output the fpga_io_map XML file *always*
  // write_fabric_io_info --file ${OPENFPGA_IO_MAP_FILE} --verbose
  std::string openfpga_write_fabric_io_info_command;
  json settings_obj = GetSession()->GetSettings()->getJson();
  // fpga_io_map
  std::filesystem::path filepath_fpga_io_map_xml;
  // form the file name using the current device: family_foundy_node
  std::string device_size = settings_obj["vpr"]["general"]["device"]["default"].get<std::string>();
  filepath_fpga_io_map_xml = family +
                             std::string("_") +
                             foundry +
                             std::string("_") +
                             node;
  if(!voltage_threshold.empty()) {
    filepath_fpga_io_map_xml += std::string("_") +
                                voltage_threshold;
  }
  if(!p_v_t_corner.empty()) {
    filepath_fpga_io_map_xml += std::string("_") +
                                p_v_t_corner;
  }
  filepath_fpga_io_map_xml += std::string("_") +
                              device_size +
                              std::string("_fpga_io_map") + std::string(".xml");
  // generate the fpga_io_map file in the generated 'working_directory', not in the 'design_directory'
  // so the below part of code is commented out.
  // if (!filepath_fpga_io_map_xml.is_absolute()) {
  //   filepath_fpga_io_map_xml = std::filesystem::path(std::filesystem::path("..") / filepath_fpga_io_map_xml);
  // }
  openfpga_write_fabric_io_info_command = std::string("write_fabric_io_info") +
                                          std::string(" --file") +
                                          std::string(" ") + filepath_fpga_io_map_xml.string();
  openfpga_write_fabric_io_info_command += std::string(" --verbose");
  result = ReplaceAll(result, "${OPENFPGA_WRITE_FABRIC_IO_INFO_COMMAND}", openfpga_write_fabric_io_info_command);

  return result;
}

bool CompilerOpenFPGA_ql::GenerateBitstream() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
#if UPSTREAM_UNUSED
  if (!HasTargetDevice()) return false;
  const bool openFpgaArch = FileUtils::FileExists(m_OpenFpgaArchitectureFile);
  if (!openFpgaArch) {
    ErrorMessage("Please specify OpenFPGA architecture file");
    return false;
  }
#endif // #if UPSTREAM_UNUSED
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
#if UPSTREAM_UNUSED
  if (!ProjManager()->getTargetDevice().empty()) {
    if (!LicenseDevice(ProjManager()->getTargetDevice())) {
      ErrorMessage(
          "Device is not licensed: " + ProjManager()->getTargetDevice() + "\n");
      return false;
    }
  }
#endif // #if UPSTREAM_UNUSED
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

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED

  std::string command = m_openFpgaExecutablePath.string() + " -batch -f " +
                        ProjManager()->projectName() + ".openfpga";

  std::string script = InitOpenFPGAScript();

  script = FinishOpenFPGAScript(script);
  if(script.empty()) {
    ErrorMessage("OpenFPGA Script is empty!");
    return false;
  }

  std::string script_path = ProjManager()->projectName() + ".openfpga";

  std::filesystem::remove(std::filesystem::path(ProjManager()->projectName()) /
                          std::string("fabric_bitstream.bit"));
  std::filesystem::remove(std::filesystem::path(ProjManager()->projectName()) /
                          std::string("fabric_independent_bitstream.xml"));
  // Create OpenFpga command and execute
  script_path =
      (std::filesystem::path(ProjManager()->projectName()) / script_path)
          .string();
  std::ofstream sofs(script_path);
  sofs << script;
  sofs.close();
#if UPSTREAM_UNUSED
  if (!FileUtils::FileExists(m_openFpgaExecutablePath)) {
    ErrorMessage("Cannot find executable: " +
                 m_openFpgaExecutablePath.string());
    return false;
  }
#endif // #if UPSTREAM_UNUSED

  std::ofstream ofs(
      (std::filesystem::path(ProjManager()->projectName()) /
       std::string(ProjManager()->projectName() + "_bitstream.cmd"))
          .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
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

bool CompilerOpenFPGA_ql::GeneratePinConstraints(std::string& filepath_fpga_fix_pins_place_str) {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  
  // PinConstraints (.place): if a pre-generated pin constraints file is available, prefer to use that, and
  //   ignore any pcf file for the project, so pcf2place flow is not invoked.
  // if there is a .place file path specified in "openfpga" > "general" > "place" > "default" : use this, else:
  // if there is a .place file in the design directory with the name: <project_name>_fix_pins.place -> use this, else:
  // use the pcf file and pcf2place flow as below:

  // PinConstraints (.pcf): if there is a PCF file available, we need to generate PinConstraints (.place) file
  //  and use it in the VPR placement stage.
  // if there is a pcf file path specified in "openfpga" > "general" > "pcf" > "default" : use this, else:
  // if there is a pcf file in the design directory with the name <project_name>.pcf  : use this, else:
  // no pcf file is found, continue without PinConstraints.

  // either way, if the .place file is available, set the path to it in the 'filepath_fpga_fix_pins_place_str' variable ref passed in.

  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path));

  json settings_obj = GetSession()->GetSettings()->getJson();

  ///////////////////////////////////////////////////////////////// PLACE ++
  std::filesystem::path filepath_place;
  if ( (settings_obj.contains("openfpga")) &&
       (settings_obj["openfpga"].contains("general")) &&
       (settings_obj["openfpga"]["general"].contains("place")) &&
       (!settings_obj["openfpga"]["general"]["place"]["default"].get<std::string>().empty()) ) {
    filepath_place = settings_obj["openfpga"]["general"]["place"]["default"].get<std::string>();
  }
  else {
    filepath_place = ProjManager()->projectName() + std::string("_fix_pins") + std::string(".place");
  }
  // we are currently in the 'design_directory' now...
  if (FileUtils::FileExists(filepath_place)) {
    if (!filepath_place.is_absolute()) {
      // if it exists, make path relative to the working directory (used when openfpga is actually run)
      filepath_place = std::filesystem::path(std::filesystem::path("..") / filepath_place);
    }
    // set the PinConstraints file path to be used by the caller.
    filepath_fpga_fix_pins_place_str = filepath_place.string();

    (*m_out) << "Design " << ProjManager()->projectName()
             << " use available PinConstraints file: "
             << filepath_fpga_fix_pins_place_str
             << std::endl;

    return true;
  }
  // else
  // no place file found, so we continue with the PCF flow for PinConstraints below.
  ///////////////////////////////////////////////////////////////// PLACE --

  ///////////////////////////////////////////////////////////////// PCF ++
  std::filesystem::path filepath_pcf;
  if ( (settings_obj.contains("openfpga")) &&
       (settings_obj["openfpga"].contains("general")) &&
       (settings_obj["openfpga"]["general"].contains("pcf")) &&
       (!settings_obj["openfpga"]["general"]["pcf"]["default"].get<std::string>().empty()) ) {
    filepath_pcf = settings_obj["openfpga"]["general"]["pcf"]["default"].get<std::string>();
  }
  else {
    filepath_pcf = ProjManager()->projectName() + std::string(".pcf");
  }
  // we are currently in the 'design_directory' now...
  if (FileUtils::FileExists(filepath_pcf)) {
    if (!filepath_pcf.is_absolute()) {
      // if it exists, make path relative to the working directory (used when openfpga is actually run)
      filepath_pcf = std::filesystem::path(std::filesystem::path("..") / filepath_pcf);
    }
  }
  else {
    // no pcf file found, so we continue without PinConstraints defined.
    // This is not an error, so we return true.
    return true;
  }
  ///////////////////////////////////////////////////////////////// PCF --

  ///////////////////////////////////////////////////////////////// NETLIST ++
  std::string netlistFile = ProjManager()->projectName() + "_post_synth.blif";
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first) {
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
  ///////////////////////////////////////////////////////////////// NETLIST --

  // get the required values for the current device:
  std::string family = settings_obj["general"]["device"]["family"]["default"].get<std::string>();
  std::string foundry = settings_obj["general"]["device"]["foundry"]["default"].get<std::string>();
  std::string node = settings_obj["general"]["device"]["node"]["default"].get<std::string>();
  std::string device_size = settings_obj["vpr"]["general"]["device"]["default"].get<std::string>();
  std::string voltage_threshold;
  std::string p_v_t_corner;
  if( settings_obj["general"]["device"].contains("voltage_threshold") &&
      settings_obj["general"]["device"].contains("p_v_t_corner") ) {
    voltage_threshold = 
        settings_obj["general"]["device"]["voltage_threshold"]["default"].get<std::string>();
    p_v_t_corner = 
            settings_obj["general"]["device"]["p_v_t_corner"]["default"].get<std::string>();
  }

  ///////////////////////////////////////////////////////////////// PIN TABLE CSV ++
  // we expect the fpga io map xml to be named: family_foundry_node_voltagethreshold_pvtcorner_size_pin_table.csv
  // this would be in the device_data/family/foundry/node/pin_table directory
  // optionally, it can also be placed the design_directory
  std::filesystem::path filename_pin_table_csv;
  std::filesystem::path filepath_pin_table_csv;
  filename_pin_table_csv = family +
                             std::string("_") +
                             foundry +
                             std::string("_") +
                             node;
  if(!voltage_threshold.empty()) {
    filename_pin_table_csv += std::string("_") +
                                voltage_threshold;
  }
  if(!p_v_t_corner.empty()) {
    filename_pin_table_csv += std::string("_") +
                                p_v_t_corner;
  }
  filename_pin_table_csv += std::string("_") +
                              device_size +
                              std::string("_pin_table") + std::string(".csv");

  filepath_pin_table_csv = GetSession()->Context()->DataPath() /
                            family /
                            foundry /
                            node /
                            std::string("pin_table") /
                            filename_pin_table_csv;
  // if the file does not exist in the device data dir
  if (!FileUtils::FileExists(filepath_pin_table_csv)) {
    // check if the file exists in the design_directory instead?
    if (FileUtils::FileExists(filename_pin_table_csv)) {
      filepath_pin_table_csv = std::filesystem::path(std::filesystem::path("..") / filename_pin_table_csv);
    }
    else {
      // no fpga io map xml available, we cannot proceed with the pcf flow!
      ErrorMessage(std::string(__func__) + ": PIN TABLE CSV File: " + filename_pin_table_csv.string() + " not found!");
    return false;
    }
  }
  ///////////////////////////////////////////////////////////////// PIN TABLE CSV --

  ///////////////////////////////////////////////////////////////// FPGA IO MAP XML ++
  // we expect the fpga io map xml to be named: family_foundry_node_voltagethreshold_pvtcorner_size_fpga_io_map.xml
  // this would be in the device_data/family/foundry/node/fpga_io_map directory
  // optionally, it can also be placed the design_directory
  std::filesystem::path filename_fpga_io_map_xml;
  std::filesystem::path filepath_fpga_io_map_xml;
  filename_fpga_io_map_xml = family +
                             std::string("_") +
                             foundry +
                             std::string("_") +
                             node;
  if(!voltage_threshold.empty()) {
    filename_fpga_io_map_xml += std::string("_") +
                                voltage_threshold;
  }
  if(!p_v_t_corner.empty()) {
    filename_fpga_io_map_xml += std::string("_") +
                                p_v_t_corner;
  }
  filename_fpga_io_map_xml += std::string("_") +
                              device_size +
                              std::string("_fpga_io_map") + std::string(".xml");

  filepath_fpga_io_map_xml = GetSession()->Context()->DataPath() /
                              family /
                              foundry /
                              node /
                              std::string("fpga_io_map") /
                              filename_fpga_io_map_xml;
  // if the file does not exist in the device data dir
  if (!FileUtils::FileExists(filepath_fpga_io_map_xml)) {
    // check if the file exists in the design_directory instead?
    if (FileUtils::FileExists(filename_fpga_io_map_xml)) {
      filepath_fpga_io_map_xml = std::filesystem::path(std::filesystem::path("..") / filename_fpga_io_map_xml);
    }
    else {
      // no fpga io map xml available, we cannot proceed with the pcf flow!
      ErrorMessage(std::string(__func__) + ": FPGA IO MAP XML File: " + filename_fpga_io_map_xml.string() + " not found!");
    return false;
    }
  }
  ///////////////////////////////////////////////////////////////// FPGA IO MAP XML --

  ///////////////////////////////////////////////////////////////// VPR FIX PINS PLACE ++
  // we want OpenFPGA to generate the vpr fix pins place file in the working_directory itself
  // and it will be used from there, so we don't adjust the path below.
  std::filesystem::path filepath_fpga_fix_pins_place;
  filepath_fpga_fix_pins_place = ProjManager()->projectName() + std::string("_fix_pins") + std::string(".place");
  ///////////////////////////////////////////////////////////////// VPR FIX PINS PLACE --

  (*m_out) << "##################################################" << std::endl;
  (*m_out) << "PinConstraints generation for design \""
          << ProjManager()->projectName() << "\" on device \""
          << ProjManager()->getTargetDevice() << "\"" << std::endl;
  (*m_out) << "##################################################" << std::endl;

  // call openfpga to generate pin_constraints 'fix_pins.place' if 'pcf2place' is enabled
  // pcf2place --pcf ${OPENFPGA_PCF} --blif ${VPR_TESTBENCH_BLIF} --pin_table ${OPENFPGA_PIN_TABLE} --fpga_io_map ${OPENFPGA_IO_MAP_FILE} --fpga_fix_pins ${OPENFPGA_VPR_FIX_PINS_FILE}
  // replace command: ${OPENFPGA_PCF2PLACE_COMMAND} in the template script

  std::string openfpga_pcf2place_command;
  openfpga_pcf2place_command = std::string("pcf2place") +
                               std::string(" --blif") +
                               std::string(" ") + netlistFile +
                               std::string(" --pcf") +
                               std::string(" ") + filepath_pcf.string() +
                               std::string(" --pin_table") +
                               std::string(" ") + filepath_pin_table_csv.string() +
                               std::string(" --fpga_io_map") +
                               std::string(" ") + filepath_fpga_io_map_xml.string() +
                               std::string(" --fpga_fix_pins") +
                               std::string(" ") + filepath_fpga_fix_pins_place.string();
  // this does not seem to be supported in OpenFPGA
  // openfpga_pcf2place_command += std::string(" --assign_unconstrained_pins") + 
  //                               std::string(" ") + 
  //                               std::string("in_define_order"); // or "random"

  std::string script = qlOpenFPGApcf2placeScript;
  script = ReplaceAll(script, "${OPENFPGA_PCF2PLACE_COMMAND}", openfpga_pcf2place_command);
  
  std::string pin_constaints_openfpga_script_name = ProjManager()->projectName() + 
                                                    std::string("_pinconstraints") + 
                                                    std::string(".openfpga");
  std::string command = m_openFpgaExecutablePath.string() + 
                        std::string(" -f") +
                        std::string(" ") +
                        pin_constaints_openfpga_script_name;

  // Create OpenFpga command and execute
  std::filesystem::path script_path =
      (std::filesystem::path(ProjManager()->projectName()) / pin_constaints_openfpga_script_name)
          .string();
  std::ofstream sofs(script_path);
  sofs << script;
  sofs.close();

  std::ofstream ofs(
      (std::filesystem::path(ProjManager()->projectName()) /
      std::string(ProjManager()->projectName() + "_pinconstraints.cmd"))
          .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                " PinConstraints generation failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName()
          << " PinConstraints generated!" << std::endl;
  
  // set the PinConstraints file path to be used by the caller.
  filepath_fpga_fix_pins_place_str = filepath_fpga_fix_pins_place.string();
  return true;
}

bool CompilerOpenFPGA_ql::LoadDeviceData(const std::string& deviceName) {
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
#if UPSTREAM_UNUSED
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
#endif // #if UPSTREAM_UNUSED
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
#if UPSTREAM_UNUSED
  if (!LicenseDevice(deviceName)) {
    ErrorMessage("Device is not licensed: " + deviceName + "\n");
    status = false;
  }
#endif // #if UPSTREAM_UNUSED
  return status;
}

bool CompilerOpenFPGA_ql::LicenseDevice(const std::string& deviceName) {
  // No need for licenses
  return true;
}

std::string CompilerOpenFPGA_ql::ToUpper(std::string str) {
        std::string upper;
        // for (size_t i = 0; i < str.size(); ++i) {
        //     char C = ::toupper(str[i]);
        //     upper.push_back(C);
        // }
        // https://stackoverflow.com/a/39927248
        upper = str;
        auto& facet = 
            std::use_facet<std::ctype<char>>(std::locale());
        facet.toupper(upper.data(), upper.data() + upper.size());
        return upper;
    }

std::string CompilerOpenFPGA_ql::ToLower(std::string str) {
    std::string lower;
    // for (size_t i = 0; i < str.size(); ++i) {
    //     char C = ::tolower(str[i]);
    //     lower.push_back(C);
    // }
    // https://stackoverflow.com/a/39927248
    lower = str;
    auto& facet = 
        std::use_facet<std::ctype<char>>(std::locale());
    facet.tolower(lower.data(), lower.data() + lower.size());
    return lower;
}


std::filesystem::path CompilerOpenFPGA_ql::GenerateTempFilePath() {

    // remember where we are
    std::filesystem::path current_path = std::filesystem::current_path();

    // get a guaranteed temp directory
    std::filesystem::path temp_dir_path = std::filesystem::temp_directory_path();

    // change to the temp directory before generating a temp file name
    std::filesystem::current_path(temp_dir_path);

    // generate a temp file path in the system temp directory
    std::filesystem::path temp_file_path;
#if defined(_WIN32)
    // in windows, the tmpnam only generates the file name (with a '\' in the front), and we need to append this
    // to the temp_directory_path() to make a complete path.
    std::string temp_file_path_str = std::tmpnam(nullptr);
    // convert the string into a std::filesystem::path
    temp_file_path = temp_file_path_str;
    // tmpnam() returns a filepath that starts with a '\' and is hence an absolute path
    // using the '/' operator with 2 absolute paths, results in replacement, than append!
    // https://stackoverflow.com/questions/55214156/why-does-stdfilesystempathappend-replace-the-current-path-if-p-starts-with
    // hence, we should convert the temp_file_path into a relative path first, and then
    // append it to the temp_dir_path to get the absolute path we need on Windows.
    temp_file_path = temp_file_path.relative_path();
    temp_file_path = temp_dir_path / temp_file_path;
#else // #if defined(_WIN32)
    // in linux, the tmpnam generates the file path including the current dir path
    // so, we can use this as the final path as is.
    std::string temp_file_path_str = std::tmpnam(nullptr);
    temp_file_path = temp_file_path_str;
#endif // #if defined(_WIN32)

    // change back to the original path we came from
    std::filesystem::current_path(current_path);

    // add to our cleanup list
    m_TempFileList.push_back(temp_file_path);
    
    // return the temp file path we obtained
    return temp_file_path;
}


int CompilerOpenFPGA_ql::CleanTempFiles() {

  int count = 0;
  std::error_code ec;
  for(std::filesystem::path tempFile: m_TempFileList) {
    // delete the source encrypted file, as it not needed anymore.
    std::filesystem::remove(tempFile,
                            ec);

    //std::cout << "removing: " << tempFile << std::endl;
    if(ec) {
      // error : ignore it.
      //std::cout << "failed removing: " << tempFile << std::endl;
    }
    count++;
  }

  m_TempFileList.clear();

  return count;
}


std::string CompilerOpenFPGA_ql::DeviceString(std::string family,
                                              std::string foundry,
                                              std::string node,
                                              std::string voltage_threshold,
                                              std::string p_v_t_corner) {

  // form the string representation of the device
  std::string device = family + "," + foundry + "," + node;

  if(!voltage_threshold.empty() && !p_v_t_corner.empty()) {
    device += "," + voltage_threshold + "," + p_v_t_corner;
  }

  return device;
}

bool CompilerOpenFPGA_ql::DeviceExists(std::string family,
                                       std::string foundry,
                                       std::string node,
                                       std::string voltage_threshold,
                                       std::string p_v_t_corner) {

  // form the string representation of the device
  std::string device = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner);

  return DeviceExists(device);
}


bool CompilerOpenFPGA_ql::DeviceExists(std::string device) {

  // get the list of available devices
  std::vector<std::string> device_list = ListDevices();

  // check if we have the device in our list
  if(std::find(device_list.begin(), device_list.end(), device) != device_list.end()) {
    return true;
  }

  return false;
}


std::vector<std::string> CompilerOpenFPGA_ql::ListDevices() {

  std::vector<std::string> empty_list_of_devices = {};
  std::vector<std::string> list_of_devices = {};

  std::string family;
  std::string foundry;
  std::string node;

  std::error_code ec;

  // get to the device_data dir path of the installation
  std::filesystem::path root_device_data_dir_path = 
      GetSession()->Context()->DataPath();

  // each dir in the device_data is a family
  //    for each family, check for foundry dirs
  //        for each foundry, check for node 
  //            for each family-foundry-node dir, check the device_variants
  
  // look at the directories inside the device_data_dir_path for 'family' entries
  for (const std::filesystem::directory_entry& dir_entry_family : 
                    std::filesystem::directory_iterator(root_device_data_dir_path)) {
    
    if(dir_entry_family.is_directory()) {
      
      // we would see family at this level
      family = dir_entry_family.path().filename().string();

      // look at the directories inside the 'family' dir for 'foundry' entries
      for (const std::filesystem::directory_entry& dir_entry_foundry : 
                    std::filesystem::directory_iterator(dir_entry_family.path())) {

        if(dir_entry_foundry.is_directory()) {
      
          // we would see foundry at this level
          foundry = dir_entry_foundry.path().filename().string();

          // look at the directories inside the 'foundry' dir for 'node' entries
          for (const std::filesystem::directory_entry& dir_entry_node : 
                          std::filesystem::directory_iterator(dir_entry_foundry.path())) {

            if(dir_entry_node.is_directory()) {
            
              // we would see devices at this level
              node = dir_entry_node.path().filename().string();

              // get all the device_variants for this device:
              std::vector<std::string> device_variants;

              device_variants = list_device_variants(family,
                                                     foundry,
                                                     node,
                                                     dir_entry_node.path());
              if(device_variants.empty()) {
                // display error, but continue with other devices.
                Message("error in parsing variants for device\n");
              }
              else {
                // add all the device_variants into the list of devices.
                list_of_devices.insert(list_of_devices.end(),
                                      device_variants.begin(),
                                      device_variants.end());
              }
            }
          }
        }
      }
    }
  }

  return list_of_devices;
}


// should we use a reference or return vector by value?
// https://stackoverflow.com/a/15704602
std::vector<std::string> CompilerOpenFPGA_ql::list_device_variants(
    std::string family,
    std::string foundry,
    std::string node,
    std::filesystem::path device_data_dir_path) {

  std::string device = DeviceString(family,
                                    foundry,
                                    node,
                                    "",
                                    "");
  Message("parsing device: " + device);

  std::vector<std::string> empty_list_of_devices = {};

  // [1] check for valid path
  // convert to canonical path, which will also check that the path exists.
  std::error_code ec;
  std::filesystem::path device_data_dir_path_c = 
          std::filesystem::canonical(device_data_dir_path, ec);
  if(ec) {
    // error
    ErrorMessage("Please check if the path specified exists!");
    ErrorMessage("path: " + device_data_dir_path.string());
    return empty_list_of_devices;
  }

  // [2] check dir structure of the device_data_dir_path
  // [2][a] atleast one set of vpr.xml and openfpga.xml files should exist.
  // [2][b] all xmls sets should be in one of the following:
  //          - device_data_dir_path (DEFAULT device)
  //          - device_data_dir_path/<ANY_DIR_NAME_VT>/<ANY_DIR_NAME_PVT_CORNER> (device_variants)
  //        <ANY_DIR_NAME_VT>(s) represent the Cell Threshold Voltage(s)
  //        <ANY_DIR_NAME_PVT_CORNER>(s) represent the PVT Corner(s) 
  // [2][c] check that we have all the (other)required XML files for the device
  
  // [2][a] search for all vpr.xml/openfpga.xml files, and check the dir paths:
  std::vector<std::filesystem::path> vpr_xml_files;
  std::vector<std::filesystem::path> openfpga_xml_files;
  for (const std::filesystem::directory_entry& dir_entry :
      std::filesystem::recursive_directory_iterator(device_data_dir_path_c,
                                                    std::filesystem::directory_options::skip_permission_denied,
                                                    ec)) {
    if(ec) {
      ErrorMessage(std::string("failed listing contents of ") +
                              device_data_dir_path_c.string());
      return empty_list_of_devices;
    }

    if(dir_entry.is_regular_file(ec)) {

      // this will match both .xml and .xml.en(encrypted) files
      std::string vpr_xml_pattern = "vpr\\.xml.*";
      std::string openfpga_xml_pattern = "openfpga\\.xml.*";
      
      if (std::regex_match(dir_entry.path().filename().string(),
                            std::regex(vpr_xml_pattern,
                              std::regex::icase))) {
        vpr_xml_files.push_back(dir_entry.path().string());
      }
      if (std::regex_match(dir_entry.path().filename().string(),
                            std::regex(openfpga_xml_pattern,
                              std::regex::icase))) {
        openfpga_xml_files.push_back(dir_entry.path().string());
      }
    }

    if(ec) {
      ErrorMessage(std::string("error while checking: ") +  dir_entry.path().string());
      return empty_list_of_devices;
    }
  }

  // sort the entries for easier processing
  std::sort(vpr_xml_files.begin(),vpr_xml_files.end());
  std::sort(openfpga_xml_files.begin(),openfpga_xml_files.end());

  // check that we have atleast one set.
  if(vpr_xml_files.size() == 0) {
    ErrorMessage("No VPR XML files were found in the source device data dir !");
    return empty_list_of_devices;
  }
  if(openfpga_xml_files.size() == 0) {
    ErrorMessage("No OPENFPGA XML files were found in the source device data dir !");
    return empty_list_of_devices;
  }

  // check that we have the same number of entries for both vpr.xml and openfpga.xml
  // as they should be travelling in pairs.
  if(vpr_xml_files.size() != openfpga_xml_files.size()) {
    ErrorMessage("Mismatched number of VPR XML(s) w.r.t OPENFPGA XML(s) !");
    return empty_list_of_devices;
  }

  // [2][b] gather all the 'parent' dirs of the XMLs, and check that they are in the expected hierarchy
  std::vector<std::filesystem::path> vpr_xml_file_parent_dirs;
  std::vector<std::filesystem::path> openfpga_xml_file_parent_dirs;
  for(std::filesystem::path xmlpath : vpr_xml_files) {
    vpr_xml_file_parent_dirs.push_back(xmlpath.parent_path());
  }
  for(std::filesystem::path xmlpath : openfpga_xml_files) {
    openfpga_xml_file_parent_dirs.push_back(xmlpath.parent_path());
  }

  // sort the entries for easier processing
  std::sort(vpr_xml_file_parent_dirs.begin(),vpr_xml_file_parent_dirs.end());
  std::sort(openfpga_xml_file_parent_dirs.begin(),openfpga_xml_file_parent_dirs.end());

  // check that we have the same set of dir paths for both XMLs, as they travel in pairs.
  // redundant?
  if(vpr_xml_file_parent_dirs != openfpga_xml_file_parent_dirs) {
    ErrorMessage("Mismatched number of VPR XML(s) w.r.t OPENFPGA XML(s) !");
    return empty_list_of_devices;
  }
  // now we can take any one of the file_dirs vector for further steps as they are the same.

  // debug prints
  // std::cout << "vpr xmls" << std::endl;
  // for(auto path : vpr_xml_files) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "openfpga xmls" << std::endl;
  // for(auto path : openfpga_xml_files) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "vpr xml dirs" << std::endl;
  // for(auto path : vpr_xml_file_parent_dirs) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "openfpga xml dirs" << std::endl;
  // for(auto path : openfpga_xml_file_parent_dirs) std::cout << path << std::endl;
  // std::cout << std::endl;

  // now that the dir paths for both xml(s) are identical vectors, take one of them.
  // each dir *should be* one of:
  // - source_device_data_dir_path ('default' XMLs not belonging to any device variant)
  // - source_device_data_dir_path/<voltage_threshold>/<p_v_t_corner> (for variants)
  //          <voltage_threshold> should be one of LVT, RVT, ULVT
  //          <p_v_t_corner> can be any name, usually something like TYPICAL, BEST, WORST ...
  // from this vector, we can deduce all of the possible device variants, and check correctness of hierarchy
  std::vector<std::string> device_variants;
  for (std::filesystem::path dirpath: vpr_xml_file_parent_dirs) {

    // canonicalize to remove any trailing slashes and normalize path to full path
    std::filesystem::path dirpath_c = std::filesystem::canonical(dirpath, ec);
    if(ec) {
      // filesystem error
      return empty_list_of_devices;
    }
    
    // check if this is the same as the source_device_data_dir_path itself (then this is the 'default')
    if(std::filesystem::equivalent(dirpath_c, device_data_dir_path_c)) {
      device = DeviceString(family,
                            foundry,
                            node,
                            "",
                            "");
      device_variants.push_back(device);
    }
    // otherwise this should be a device_variant
    else {
      // get the dir-name component of the path, this is the p_v_t_corner
      std::string p_v_t_corner = dirpath_c.filename().string();
      
      // get the dir-name component of the parent of the path, this is the voltage_threshold
      std::string voltage_threshold = dirpath_c.parent_path().filename().string();
      
      // add the variant to the list
      device = DeviceString(family,
                            foundry,
                            node,
                            voltage_threshold,
                            p_v_t_corner);
      device_variants.push_back(device);


      // check that p_v_t_corner dir is 2 levels down from the source_device_data_dir_path
      if(!std::filesystem::equivalent(dirpath_c.parent_path().parent_path(), device_data_dir_path_c)) {
        std::cout << dirpath_c.parent_path() << std::endl;
        std::cout << device_data_dir_path_c << std::endl;
        ErrorMessage("p_v_t_corner dirs with XMLs are not 2 levels down from the source_device_data_dir_path!!!");
        return empty_list_of_devices;
      }
    }
  }

  // sort the devices found
  std::sort(device_variants.begin(),device_variants.end());

  // debug prints
  // std::cout << std::endl;
  // std::cout << "device variants parsed:" << std::endl;
  // std::cout << "<family>,<foundry>,<node>,[voltage_threshold],[p_v_t_corner]" << std::endl;
  // int index = 1;
  // for (auto device_variant: device_variants) {
  //   std::cout << index << ". " << device_variant << std::endl;
  //   index++;
  // }
  // std::cout << std::endl;

  // [2][c] check other required and optional XML files for the device:
  // required:
  std::filesystem::path fixed_sim_openfpga_xml = 
      device_data_dir_path_c / "fixed_sim_openfpga.xml";
  std::filesystem::path fixed_sim_openfpga_xml_en = 
      device_data_dir_path_c / "fixed_sim_openfpga.xml.en";
  if(!std::filesystem::exists(fixed_sim_openfpga_xml) &&
     !std::filesystem::exists(fixed_sim_openfpga_xml_en)) {
    ErrorMessage("fixed_sim_openfpga.xml not found in source_device_data_dir_path!!!");
    return empty_list_of_devices;
  }

  // optional: not checking these for now, if needed we can add in later.
  //std::filesystem::path bitstream_annotation_xml = 
  //    source_device_data_dir_path_c / "bitstream_annotation.xml";
  //std::filesystem::path repack_design_constraint_xml = 
  //    source_device_data_dir_path_c / "repack_design_constraint.xml";
  //std::filesystem::path fabric_key_xml = 
  //    source_device_data_dir_path_c / "fabric_key.xml";

  return device_variants;
}
// clang-format on
