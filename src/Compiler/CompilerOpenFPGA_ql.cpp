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
#include <QJsonArray>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/CompilerOpenFPGA_ql.h"
#include "Compiler/Constraints.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "MainWindow/main_window.h"
#include "Main/WidgetFactory.h"
#include <QWidget>
#include <QVBoxLayout>

using namespace FOEDAG;

extern const char* foedag_version_number;
extern const char* foedag_git_hash;
void CompilerOpenFPGA_ql::Version(std::ostream* out) {
  (*out) << "Foedag OpenFPGA_ql Compiler"
         << "\n";
  if (std::string(foedag_version_number) != "${VERSION_NUMBER}")
    (*out) << "Version : " << foedag_version_number << "\n";
  if (std::string(foedag_git_hash) != "${GIT_HASH}")
    (*out) << "Git Hash: " << foedag_git_hash << "\n";
  (*out) << "Built   : " << std::string(__DATE__) << "\n";
}

void CompilerOpenFPGA_ql::Help(std::ostream* out) {
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
  (*out) << "   --verific        : Uses Verific parser" << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help                       : This help" << std::endl;
  (*out) << "   create_design <name>       : Creates a design with <name> name"
         << std::endl;
  (*out) << "   architecture <vpr_file.xml> ?<openfpga_file.xml>? :"
         << std::endl;
  (*out) << "                                Uses the architecture file and "
            "optional openfpga arch file (For bitstream generation)"
         << std::endl;
  (*out) << "   bitstream_config_files <bitstream_setting.xml> "
            "?<sim_setting.xml>? ?<repack_setting.xml>? :"
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
  (*out) << "   add_design_file <file>... <type> (-VHDL_1987, -VHDL_1993, "
            "-VHDL_2000, "
            "-VHDL_2008 (.vhd default), -V_1995, "
            "-V_2001 (.v default), -SV_2005, -SV_2009, -SV_2012, -SV_2017 (.sv "
            "default)) "
         << std::endl;
  (*out) << "   read_netlist <file>        : Read a netlist instead of an RTL "
            "design (Skip Synthesis)"
         << std::endl;
  (*out) << "   add_include_path <path1>...: As in +incdir+" << std::endl;
  (*out) << "   add_library_path <path1>...: As in +libdir+" << std::endl;
  (*out) << "   set_macro <name>=<value>...: As in -D<macro>=<value>"
         << std::endl;
  (*out) << "   set_top_module <top>       : Sets the top module" << std::endl;
  (*out) << "   add_constraint_file <file> : Sets SDC + location constraints"
         << std::endl;
  (*out) << "                                Constraints: set_pin_loc, "
            "set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   ipgenerate" << std::endl;
  (*out) << "   verific_parser <on/off>    : Turns on/off Verific parser"
         << std::endl;
  (*out) << "   synthesize <optimization>  : Optional optimization (area, "
            "delay, mixed, none)"
         << std::endl;
  (*out) << "   pnr_options <option list>  : VPR Options" << std::endl;
  (*out) << "   packing                    : Packing" << std::endl;
  (*out) << "   global_placement           : Analytical placer" << std::endl;
  (*out) << "   place                      : Detailed placer" << std::endl;
  (*out) << "   route                      : Router" << std::endl;
  (*out) << "   sta                        : Statistical Timing Analysis"
         << std::endl;
  (*out) << "   power                      : Power estimator" << std::endl;
  (*out) << "   bitstreamm                 : Bitstream generation" << std::endl;
  (*out) << "----------------------------------" << std::endl;
}

const std::string qlYosysScript = R"( 

${PLUGIN_LOAD}

${READ_DESIGN_FILES}

synth_quicklogic -top ${TOP_MODULE} -family ${FAMILY} -blif ${OUTPUT_BLIF} ${YOSYS_OPTIONS}

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
  )";

bool CompilerOpenFPGA_ql::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  Compiler::RegisterCommands(interp, batchMode);
  auto select_architecture_file = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
    std::string name;
    if (argc < 2) {
      compiler->ErrorMessage("Specify an architecture file");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (compiler->FileExists(expandedFile)) {
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
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
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
      std::string expandedFile = argv[i];
      bool use_orig_path = false;
      if (compiler->FileExists(expandedFile)) {
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
      if (i == 1) {
        compiler->OpenFpgaBitstreamSettingFile(expandedFile);
        compiler->Message("OpenFPGA Bitstream Setting file: " + expandedFile);
      } else if (i == 2) {
        compiler->OpenFpgaSimSettingFile(expandedFile);
        compiler->Message("OpenFPGA Simulation Setting file: " + expandedFile);
      } else if (i == 3) {
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
    if (compiler->FileExists(expandedFile)) {
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
    if (compiler->FileExists(expandedFile)) {
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

  auto set_device_size = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
    std::string name;
    if (argc != 2) {
      compiler->ErrorMessage("Specify a device size: xXy");
      return TCL_ERROR;
    }
    compiler->DeviceSize(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("set_device_size", set_device_size, this, 0);

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

  auto demovprsettings = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
                            
  CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;

  //std::string settings_json_filename = compiler->m_projManager->projectName() + ".json";
  std::string settings_json_filename = "../counter_16bit.json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = compiler->GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path), "Settings");


  // create a temp dialog to show the widgets
  QDialog* dlg = new QDialog();
  dlg->setWindowTitle("VPR Settings");
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  QVBoxLayout* layout = new QVBoxLayout();
  dlg->setLayout(layout);

  // settings
  //  category
  //    subcategory
  //      param object, has "widgetType"= in the object, can be gui'ed.

  // settings is a dialog
  // inside the dialog HLAYOUT are : 1. listview 2. QStackedWidget
  // listview has the list of all 'categories'
  // on selecting a 'category', QStackedWidget switches to the appropriate stacked 'page', which is each a widget
  // use a QTabWidget for each 'page', and each 'subcategory' is represented with a tab 'page'
  // on each tab 'page' we have a widget that contains all the settings objects.
 
  QWidget* tasks = new QWidget();
  tasks->setObjectName("tasksWidget");
  QVBoxLayout* VLayout = new QVBoxLayout();
  tasks->setLayout(VLayout);

  QJsonValue tasksVal = currentSettings->getNested("Settings.VPR", ".");
  if (tasksVal.isObject()) {
    // Step through Tasks
    // Convert value to object and step through object keys
    QJsonObject tasksObj = tasksVal.toObject();
    for (const QString& taskName : tasksObj.keys()) {
      // Get task object values
      QJsonValue taskVal = tasksObj.value(taskName);
      if (taskVal.isArray()) {
        // Step through task settings
        for (QJsonValue setting : taskVal.toArray()) {
          if (setting.isObject()) {
            QJsonObject metaObj = setting.toObject();

            QWidget* subWidget = FOEDAG::createWidget(metaObj);
            VLayout->addWidget(subWidget);
          }
        }
      }
    }
  }

  layout->addWidget(tasks);
  dlg->show();

  return TCL_OK;
  };
  GetSession()->TclInterp()->registerCmd("demovprsettings", demovprsettings, 0, 0);

  return true;
}

bool CompilerOpenFPGA_ql::IPGenerate() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_projManager->projectName()
           << "..." << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " IPs are generated!"
           << std::endl;
  m_state = IPGenerated;
  return true;
}

bool CompilerOpenFPGA_ql::DesignChanged(
    const std::string& synth_script,
    const std::filesystem::path& synth_scrypt_path) {
  bool result = false;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(path /
                                m_projManager->projectName());  // setting path
  std::string output = m_projManager->projectName() + "_post_synth.blif";
  time_t time_netlist = Mtime(output);
  if (time_netlist == -1) {
    result = true;
  }
  for (const auto& lang_file : m_projManager->DesignFiles()) {
    std::vector<std::string> tokens;
    Tokenize(lang_file, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : m_projManager->includePathList()) {
    std::vector<std::string> tokens;
    Tokenize(path, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
        if ((tf > time_netlist) || (tf == -1)) {
          result = true;
          break;
        }
      }
    }
  }
  for (auto path : m_projManager->libraryPathList()) {
    std::vector<std::string> tokens;
    Tokenize(path, " ", tokens);
    for (auto file : tokens) {
      file = Trim(file);
      if (file.size()) {
        time_t tf = Mtime(file);
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

bool CompilerOpenFPGA_ql::Synthesize() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << m_projManager->projectName() << "..."
           << std::endl;

  std::string yosysScript = InitSynthesisScript();

  // read settings -> SynthArray of Objects
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path), "Settings");
  QJsonArray settingsSynthArray = currentSettings->getNested("Settings.Tasks.SYNTHESIS", ".").toArray();

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
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
    for (auto path : m_projManager->includePathList()) {
      includes += path + " ";
    }
    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : m_projManager->libraryPathList()) {
      libraries += path + " ";
    }
    fileList += "verific -vlog-libdir " + libraries + "\n";

    std::string macros;
    for (auto& macro_value : m_projManager->macroList()) {
      macros += macro_value.first + "=" + macro_value.second + " ";
    }
    fileList += "verific -vlog-define " + macros + "\n";

    for (const auto& lang_file : m_projManager->DesignFiles()) {
      std::string lang;
      switch (m_projManager->designFileData(lang_file)) {
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
      fileList += "verific " + lang + " " + lang_file + "\n";
    }
    fileList += "verific -import " + m_projManager->DesignTopModule() + "\n";
    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}", fileList);
  } else {
    // Default Yosys parser

#ifdef _WIN32
  // plugins are not supported on WIN32, yosys is built with the plugin code directly instead.
  yosysScript = ReplaceAll(yosysScript, "${PLUGIN_LOAD}", std::string(""));
#else // #ifdef _WIN32
  yosysScript = ReplaceAll(yosysScript, "${PLUGIN_LOAD}", std::string("plugin -i ql-qlf"));
#endif // #ifdef _WIN32

    std::string macros = "";
    // std::string macros = "verilog_defines ";
    // for (auto& macro_value : m_projManager->macroList()) {
    //   macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    // }
    // macros += "\n";
    // std::string includes;
    // for (auto path : m_projManager->includePathList()) {
    //   includes += "-I" + path + " ";
    // }

    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}",
                             macros +
                                 "read_verilog ${READ_VERILOG_OPTIONS} "
                                 "${INCLUDE_PATHS} ${VERILOG_FILES}");
    std::string fileList;
    std::string lang;
    for (const auto& lang_file : m_projManager->DesignFiles()) {
      fileList += lang_file + " ";
      switch (m_projManager->designFileData(lang_file)) {
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
    }
    std::string options = lang;
    options += " -nolatches";
    yosysScript = ReplaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", options);
    
    std::string includes;
    for (auto path : m_projManager->includePathList()) {
      includes += "-I" + path + " ";
    }
    yosysScript = ReplaceAll(yosysScript, "${INCLUDE_PATHS}", includes);
    
    
    yosysScript = ReplaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  }

  yosysScript = ReplaceAll(yosysScript, "${TOP_MODULE}", m_projManager->DesignTopModule());
  
  std::string family = currentSettings->getNested("Settings.FAMILY", ".").toString().toStdString();
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
  
  yosysScript = ReplaceAll(yosysScript, "${OUTPUT_BLIF}",
                           std::string(m_projManager->projectName() + "_post_synth.blif"));

  

  // use settings to populate yosys_options
  std::string yosys_options;
  for (QJsonValue setting : settingsSynthArray) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();
    //   for (const QString& settingKey : settingObj.keys()) {
    //     //qDebug() << "\t\t" << settingKey << "->" << settingObj.value(settingKey);
    //   }
    // qDebug() << "\n";

    //   qDebug() << settingObj["id"].toString() << settingObj["default"].toString() << "\n";

      if(settingObj["id"].toString() == "verilog" && settingObj["default"].toString() == "checked") {
        yosys_options += " -verilog " + std::string(m_projManager->projectName() + "_post_synth.v");
      }

      if(settingObj["id"].toString() == "no_abc_opt" && settingObj["default"].toString() == "checked") {
          yosys_options += " -no_abc_opt";
      }

      if(settingObj["id"].toString() == "no_adder" && settingObj["default"].toString() == "checked") {
        yosys_options += " -no_adder";
      }

      if(settingObj["id"].toString() == "no_ff_map" && settingObj["default"].toString() == "checked") {
        yosys_options += " -no_ff_map";
      }

      if(settingObj["id"].toString() == "no_dsp" && settingObj["default"].toString() == "checked") {
        yosys_options += " -no_dsp";
      }

      if(settingObj["id"].toString() == "no_bram" && settingObj["default"].toString() == "checked") {
        yosys_options += " -no_bram";
      }

      if(settingObj["id"].toString() == "edif" && settingObj["default"].toString() == "checked") {
        yosys_options += " -edif " + std::string(m_projManager->projectName() + ".edif");
      }
    }
  }

  yosysScript = ReplaceAll(yosysScript, "${YOSYS_OPTIONS}", yosys_options);

  yosysScript = FinishSynthesisScript(yosysScript);

  std::string script_path = m_projManager->projectName() + ".ys";
  if (!DesignChanged(yosysScript, script_path)) {
    (*m_out) << "Design didn't change: " << m_projManager->projectName()
             << ", skipping synthesis." << std::endl;
    return true;
  }
  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string(m_projManager->projectName() + "_post_synth.blif"));
  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string(m_projManager->projectName() + "_post_synth.v"));
  // Create Yosys command and execute
  script_path =
      (std::filesystem::path(m_projManager->projectName()) / script_path).string();
  std::ofstream ofs(script_path);
  ofs << yosysScript;
  ofs.close();
//   if (!FileExists(m_yosysExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
//     return false;
//   }
  std::string command = m_yosysExecutablePath.string() + " -s " +
                        std::string(m_projManager->projectName() + ".ys -l " +
                                    m_projManager->projectName() + "_synth.log");
  (*m_out) << "Synthesis command: " << command << std::endl;
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() + " synthesis failed!");
    return false;
  } else {
    m_state = State::Synthesized;
    (*m_out) << "Design " << m_projManager->projectName() << " is synthesized!"
             << std::endl;
    return true;
  }
}

std::string CompilerOpenFPGA_ql::InitSynthesisScript() {
  // Default or custom Yosys script
  if (m_yosysScript.empty()) {
    m_yosysScript = qlYosysScript;//basicYosysScript;
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
    (*m_out) << "Keep name: " << keep << "\n";
    keeps += "setattr -set keep 1 " + keep + "\n";
  }
  result = ReplaceAll(result, "${KEEP_NAMES}", keeps);
  result = ReplaceAll(result, "${OPTIMIZATION}", "");
  result = ReplaceAll(result, "${LUT_SIZE}", std::to_string(m_lut_size));
  return result;
}

std::string CompilerOpenFPGA_ql::BaseVprCommand() {

  // read settings -> SynthArray of Objects
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path), "Settings");

// not using these as of now, as they don't match neatly with VPR.
//   QJsonArray settingsPACKING = currentSettings->getNested("Settings.Tasks.PACKING", ".").toArray();
//   QJsonArray settingsGLOBAL_PLACEMENT = currentSettings->getNested("Settings.Tasks.GLOBAL_PLACEMENT", ".").toArray();
//   QJsonArray settingsPLACEMENT = currentSettings->getNested("Settings.Tasks.PLACEMENT", ".").toArray();
//   QJsonArray settingsROUTING = currentSettings->getNested("Settings.Tasks.ROUTING", ".").toArray();
//   QJsonArray settingsTIMING = currentSettings->getNested("Settings.Tasks.TIMING", ".").toArray();
//   QJsonArray settingsPOWER = currentSettings->getNested("Settings.Tasks.POWER", ".").toArray();

  QJsonArray settingsVPR_GENERAL = currentSettings->getNested("Settings.VPR.GENERAL", ".").toArray();
  QJsonArray settingsVPR_FILENAME = currentSettings->getNested("Settings.VPR.FILENAME", ".").toArray();
  QJsonArray settingsVPR_NETLIST = currentSettings->getNested("Settings.VPR.NETLIST", ".").toArray();
  QJsonArray settingsVPR_PACK = currentSettings->getNested("Settings.VPR.PACK", ".").toArray();
  QJsonArray settingsVPR_PLACE = currentSettings->getNested("Settings.VPR.PLACE", ".").toArray();
  QJsonArray settingsVPR_ROUTE = currentSettings->getNested("Settings.VPR.ROUTE", ".").toArray();
  QJsonArray settingsVPR_ANALYSIS = currentSettings->getNested("Settings.VPR.ANALYSIS", ".").toArray();

  std::string vpr_options;
  for (QJsonValue setting : settingsVPR_GENERAL) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();
    

      if(settingObj["id"].toString() == "device") {
        vpr_options += std::string(" --device") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "timing_analysis") {
        vpr_options += std::string(" --timing_analysis");
        if(settingObj["default"].toString() == "checked") {
            vpr_options += std::string(" on");
        }
        else {
            vpr_options += std::string(" off");
        }
      }

      if(settingObj["id"].toString() == "constant_net_method") {
        vpr_options += std::string(" --constant_net_method") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "clock_modeling") {
        vpr_options += std::string(" --clock_modeling") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "exit_before_pack") {
        vpr_options += std::string(" --exit_before_pack");
        if(settingObj["default"].toString() == "checked") {
            vpr_options += std::string(" on");
        }
        else {
            vpr_options += std::string(" off");
        }
      }
    }
  }

  for (QJsonValue setting : settingsVPR_FILENAME) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();

      if(settingObj["id"].toString() == "circuit_format") {
        vpr_options += std::string(" --circuit_format") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "net_file" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --net_file") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "place_file" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --place_file") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "route_file" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --route_file") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "sdc_file" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --sdc_file") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      if(settingObj["id"].toString() == "write_rr_graph" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --write_rr_graph") + std::string(" ") + settingObj["default"].toString().toStdString();
      }
    }
  }

  for (QJsonValue setting : settingsVPR_NETLIST) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();

      if(settingObj["id"].toString() == "absorb_buffer_luts") {
        vpr_options += std::string(" --absorb_buffer_luts");
        if(settingObj["default"].toString() == "checked") {
            vpr_options += std::string(" on");
        }
        else {
            vpr_options += std::string(" off");
        }
      }
    }
  }

  // PACK - nothing yet

  // PLACE - nothing yet

  for (QJsonValue setting : settingsVPR_ROUTE) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();

      if(settingObj["id"].toString() == "route_chan_width" && !settingObj["default"].toString().isEmpty()) {
        vpr_options += std::string(" --route_chan_width") + std::string(" ") + settingObj["default"].toString().toStdString();
      }

      // fallback values - should this be enforced if empty in the settings JSON?
      // if(family == "QLF_K6N10") {
      //   vpr_options += std::string(" --route_chan_width") + std::string(" ") + std::string("180")
      // }
      // else if(family == "QLF_K4N8") {
      //   vpr_options += std::string(" --route_chan_width") + std::string(" ") + std::string("60")
      // }
      
    }
  }

    for (QJsonValue setting : settingsVPR_ANALYSIS) {
    if (setting.isObject()) {
      QJsonObject settingObj = setting.toObject();

      if(settingObj["id"].toString() == "gen_post_synthesis_netlist") {
        vpr_options += std::string(" --gen_post_synthesis_netlist");
        if(settingObj["default"].toString() == "checked") {
            vpr_options += std::string(" on");
        }
        else {
            vpr_options += std::string(" off");
        }
      }
    }
  }


  std::string netlistFile = m_projManager->projectName() + "_post_synth.blif";

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file;
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

  //std::string pnrOptions;
  //if (!PnROpt().empty()) pnrOptions = " " + PnROpt();

  std::string family = currentSettings->getNested("Settings.FAMILY", ".").toString().toStdString();
  std::string foundry = currentSettings->getNested("Settings.FOUNDRY", ".").toString().toStdString();
  std::string node = currentSettings->getNested("Settings.NODE", ".").toString().toStdString();
  m_architectureFile = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("vpr.xml"));
  

  std::string command =
      m_vprExecutablePath.string() + std::string(" ") +
      // TODO: select architecture file from family name, from settings/tcl
      m_architectureFile.string() + std::string(" ") +
      //std::string("aurora2/device_data/QLF_K6N10/TSMC/22nm/vpr.xml") + std::string(" ") +
      std::string(netlistFile) + // NOTE: don't add a " " here
      // TODO: sdc file needs to be taken and preprocessed before use?
      //std::string(" --sdc_file ") + std::string(m_projManager->projectName() + "_openfpga.sdc") + // NOTE: don't add a " " here
      vpr_options;
  return command;
}

bool CompilerOpenFPGA_ql::Packing() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
  const std::string sdcOut =
      (std::filesystem::path(m_projManager->projectName()) /
       std::string(m_projManager->projectName() + "_openfpga.sdc"))
          .string();
  std::ofstream ofssdc(sdcOut);
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

    // pin location constraints have to be translated to .place:
    if (constraint.find("set_pin_loc") != std::string::npos) {
      continue;
    }
    ofssdc << constraint << "\n";
  }
  ofssdc.close();

  std::string command = BaseVprCommand() + " --pack";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_pack.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();

  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() + " packing failed!");
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << m_projManager->projectName() << " is packed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::GlobalPlacement() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_projManager->projectName()
           << "..." << std::endl;
  // TODO:
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << m_projManager->projectName()
           << " is globally placed!" << std::endl;
  return true;
}

bool CompilerOpenFPGA_ql::Placement() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced &&
      m_state != State::Placed) {
    ErrorMessage("Design needs to be in packed or globally placed state");
    return false;
  }
  (*m_out) << "Placement for design: " << m_projManager->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
  std::string command = BaseVprCommand() + " --place";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_place.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " placement failed!");
    return false;
  }
  m_state = State::Placed;
  (*m_out) << "Design " << m_projManager->projectName() << " is placed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::Route() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Placed) {
    ErrorMessage("Design needs to be in placed state");
    return false;
  }
  (*m_out) << "Routing for design: " << m_projManager->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
  std::string command = BaseVprCommand() + " --route";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_route.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() + " routing failed!");
    return false;
  }
  m_state = State::Routed;
  (*m_out) << "Design " << m_projManager->projectName() << " is routed!"
           << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::TimingAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  (*m_out) << "Analysis for design: " << m_projManager->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }

  std::string command = BaseVprCommand() + " --analysis";
  std::ofstream ofs((std::filesystem::path(m_projManager->projectName()) /
                     std::string(m_projManager->projectName() + "_sta.cmd"))
                        .string());
  ofs << command << " --disp on" << std::endl;
  ofs.close();

  // testing only
  //command = std::string("vpr ") + m_architectureFile.string() + " counter_16bit_post_synth.blif --route_chan_width 180 --analysis";

  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " timing analysis failed! status: " + std::to_string(status));
    return false;
  }

  (*m_out) << "Design " << m_projManager->projectName()
           << " is timing analysed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::PowerAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  (*m_out) << "Analysis for design: " << m_projManager->projectName() << "..."
           << std::endl;
  std::string command = BaseVprCommand() + " --analysis";
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " power analysis failed! status: " + std::to_string(status));
    return false;
  }

  (*m_out) << "Design " << m_projManager->projectName() << " is power analysed!"
           << std::endl;
  return true;
}

const std::string basicOpenFPGABitstreamScript = R"( 
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --absorb_buffer_luts off --write_rr_graph rr_graph.openfpga.xml --constant_net_method route --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation settings
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

read_openfpga_bitstream_setting -f ${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

# Apply fix-up to clustering nets based on routing results
pb_pin_fixup --verbose

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --compress_routing --duplicate_grid_pin 

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}

#build_architecture_bitstream --write_file fabric_independent_bitstream.xml

build_fabric_bitstream
write_fabric_bitstream --format plain_text --file fabric_bitstream.bit
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit

)";

const std::string qlOpenFPGABitstreamScript = R"( 
${VPR_ANALYSIS_COMMAND}

# Read OpenFPGA architecture definition
read_openfpga_arch -f ${OPENFPGA_ARCH_FILE}

# Read OpenFPGA simulation setting (not in aurora)
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

# Read OpenFPGA bitstream setting (not in aurora)
read_openfpga_bitstream_setting -f ${OPENFPGA_BITSTREAM_SETTING_FILE}

# Annotate the OpenFPGA architecture to VPR data base
# to debug use --verbose options
link_openfpga_arch --sort_gsb_chan_node_in_edges 

# Apply fix-up to clustering nets based on routing results
pb_pin_fixup --verbose

# Apply fix-up to Look-Up Table truth tables based on packing results
lut_truth_table_fixup

# Build the module graph
#  - Enabled compression on routing architecture modules
#  - Enable pin duplication on grid modules
build_fabric --compress_routing --duplicate_grid_pin 

# Dump GSB data
# Necessary for creation of rr graph for SymbiFlow
write_gsb_to_xml --file gsb 

# Repack the netlist to physical pbs
# This must be done before bitstream generator and testbench generation
# Strongly recommend it is done after all the fix-up have been applied
#repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS} # not in aurora
repack

# Build bitstream database
#build_architecture_bitstream --write_file fabric_independent_bitstream.xml # not in aurora
build_architecture_bitstream

# Build fabric bitstream
build_fabric_bitstream

# Write fabric bitstream
write_fabric_bitstream --format plain_text --keep_dont_care_bits --file fabric_bitstream.bit

# Write io mapping 
write_io_mapping -f PinMapping.xml

# Finish and exit OpenFPGA
exit
)";

std::string CompilerOpenFPGA_ql::InitOpenFPGAScript() {
  // Default or custom OpenFPGA script
  if (m_openFPGAScript.empty()) {
    m_openFPGAScript = qlOpenFPGABitstreamScript;//basicOpenFPGABitstreamScript;
  }
  return m_openFPGAScript;
}

std::string CompilerOpenFPGA_ql::FinishOpenFPGAScript(const std::string& script) {
  std::string result = script;

  // read settings -> SynthArray of Objects
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  Settings * currentSettings = GetSession()->GetSettings();
  currentSettings->loadJsonFile(QString::fromStdString(settings_json_path), "Settings");


  std::string family = currentSettings->getNested("Settings.FAMILY", ".").toString().toStdString();
  std::string foundry = currentSettings->getNested("Settings.FOUNDRY", ".").toString().toStdString();
  std::string node = currentSettings->getNested("Settings.NODE", ".").toString().toStdString();
  m_OpenFpgaArchitectureFile = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("openfpga.xml"));
  m_OpenFpgaBitstreamSettingFile = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("bitstream_annotation.xml"));
  m_OpenFpgaSimSettingFile  = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("fixed_sim_openfpga.xml"));
  // (1) call vpr to execute analysis ?
  std::string vpr_analysis_command = BaseVprCommand();// + " --analysis";
  result = ReplaceAll(result, "${VPR_ANALYSIS_COMMAND}", vpr_analysis_command);

  std::string netlistFilePrefix = m_projManager->projectName() + "_post_synth";

  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        std::filesystem::path the_path = lang_file;
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
                      m_projManager->projectName() + "_openfpga.sdc");

  std::string netlistFile = m_projManager->projectName() + "_post_synth.blif";
  for (const auto& lang_file : m_projManager->DesignFiles()) {
    switch (m_projManager->designFileData(lang_file)) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file;
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
  return result;
}

bool CompilerOpenFPGA_ql::GenerateBitstream() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (BitsOpt() == BitstreamOpt::NoBitsOpt) {
    if (m_state != State::Routed) {
      ErrorMessage("Design needs to be in routed state");
      return false;
    }
  }
  (*m_out) << "Bitstream generation for design: "
           << m_projManager->projectName() << "..." << std::endl;

  if (BitsOpt() == BitstreamOpt::NoBitsOpt) {
    (*m_out) << "Design " << m_projManager->projectName()
             << " bitstream is generated!" << std::endl;
    return true;
  }
  // This is WIP, have to force it to execute (bitstream force)

  std::string command = m_openFpgaExecutablePath.string() + " -f " +
                        m_projManager->projectName() + ".openfpga";

  std::string script = InitOpenFPGAScript();

  script = FinishOpenFPGAScript(script);

  std::string script_path = m_projManager->projectName() + ".openfpga";

  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string("fabric_bitstream.bit"));
  std::filesystem::remove(std::filesystem::path(m_projManager->projectName()) /
                          std::string("fabric_independent_bitstream.xml"));
  // Create OpenFpga command and execute
  script_path =
      (std::filesystem::path(m_projManager->projectName()) / script_path)
          .string();
  std::ofstream sofs(script_path);
  sofs << script;
  sofs.close();
//   if (!FileExists(m_openFpgaExecutablePath)) {
//     ErrorMessage("Cannot find executable: " +
//                  m_openFpgaExecutablePath.string());
//     return false;
//   }

  std::ofstream ofs(
      (std::filesystem::path(m_projManager->projectName()) /
       std::string(m_projManager->projectName() + "_bitstream.cmd"))
          .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  if (status) {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " bitream generation failed!");
    return false;
  }
  m_state = State::BistreamGenerated;

  (*m_out) << "Design " << m_projManager->projectName()
           << " bitstream is generated!" << std::endl;
  return true;
}
