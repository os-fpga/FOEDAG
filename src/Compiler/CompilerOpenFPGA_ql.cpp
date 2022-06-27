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

CompilerOpenFPGA_ql::~CompilerOpenFPGA_ql() {
  CleanTempFiles();
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
  (*out) << "   add_library_ext <.v> <.sv> ...: As in +libext+" << std::endl;
  (*out) << "   set_macro <name>=<value>...: As in -D<macro>=<value>"
         << std::endl;
  (*out) << "   set_top_module <top>       : Sets the top module" << std::endl;
  (*out) << "   add_constraint_file <file> : Sets SDC + location constraints"
         << std::endl;
  (*out) << "                                Constraints: set_pin_loc, "
            "set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   ipgenerate ?clean?" << std::endl;
  (*out) << "   verific_parser <on/off>    : Turns on/off Verific parser"
         << std::endl;
  (*out) << "   synthesis_type Yosys/QL/RS : Selects Synthesis type"
         << std::endl;
  (*out)
      << "   synthesize <optimization> ?clean? : Optional optimization (area, "
         "delay, mixed, none)"
      << std::endl;
  (*out) << "   synth_options <option list>: Yosys Options" << std::endl;
  (*out) << "   pnr_options <option list>  : VPR Options" << std::endl;
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

  auto target_device = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    CompilerOpenFPGA_ql* compiler = (CompilerOpenFPGA_ql*)clientData;
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

    std::vector<std::string> device_variants;

    // [2] check dir structure of the source_device_data_dir_path of the device to be added
    // and return the list of device_variants if everything is ok.
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
    std::vector<std::filesystem::path> source_device_data_file_list;
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
          // we want xml files:
          if (std::regex_match(dir_entry.path().filename().string(), 
                                std::regex(".+\\.xml", 
                                std::regex::icase))) {
            source_device_data_file_list.push_back(dir_entry.path().string());
          }
      }

      if(ec) {
        compiler->ErrorMessage(std::string("error while checking: ") +  dir_entry.path().string());
        return TCL_ERROR;
      }
    }

    // debug prints
    // std::sort(source_device_data_file_list.begin(),source_device_data_file_list.end());
    // std::cout << "source_device_data_file_list" << std::endl;
    // for(auto path : source_device_data_file_list) std::cout << path << std::endl;
    // std::cout << std::endl;

    // encrypt the list of files
    if (!CRFileCryptProc::getInstance()->encryptFiles(source_device_data_file_list)) {
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
    for(std::filesystem::path source_file_path : source_device_data_file_list) {

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

    compiler->Message("\ndevice added ok: " + device);

    return TCL_OK;
  };
  interp->registerCmd("add_device", add_device, this, 0);

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

bool CompilerOpenFPGA_ql::IPGenerate() {
  PERF_LOG("IPGenerate has started");
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << ProjManager()->projectName()
           << "..." << std::endl;

  // placeholder for ipgenerate process ++
  std::string settings_json_filename = m_projManager->projectName() + ".json";
  std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  GetSession()->GetSettings()->loadJsonFile(QString::fromStdString(settings_json_path));
  json settings_general_device_obj = GetSession()->GetSettings()->getJson()["general"]["device"];
  

  std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
  std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
  std::string node = settings_general_device_obj["node"]["default"].get<std::string>();

  // use script from project dir:
  //std::filesystem::path python_script_path = std::filesystem::path(std::filesystem::current_path() / std::string("example.py"));
  // use script from scripts dir: try getting from environment variable
  const char* const path_scripts = std::getenv("AURORA2_SCRIPTS_DIR"); // this is from setup.sh
  std::filesystem::path scriptsDir;
  std::error_code ec;
  if (path_scripts != nullptr) {
    std::filesystem::path dirpath = std::string(path_scripts);
    if (std::filesystem::exists(dirpath, ec)) {
      scriptsDir = dirpath;
    }
  }

  // proceed if we have a valid scripts directory
  if (!scriptsDir.empty()) {
    std::filesystem::path python_script_path =
        std::filesystem::path(scriptsDir / std::string("example.py"));
    std::string command = std::string("python3") + std::string(" ") +
                          python_script_path.string() + std::string(" ") +
                          std::string("IPGenerate") + std::string(" ") +
                          m_projManager->projectName();

    int status = ExecuteAndMonitorSystemCommand(command);
    CleanTempFiles();
    if (status) {
      ErrorMessage("Design " + m_projManager->projectName() +
                   " IP generation failed!");
      return false;
    }
  }
  // placeholder for ipgenerate process --

  (*m_out) << "Design " << ProjManager()->projectName() << " IPs are generated!"
           << std::endl;
  m_state = State::IPGenerated;
  return true;
}

bool CompilerOpenFPGA_ql::DesignChanged(
    const std::string& synth_script,
    const std::filesystem::path& synth_scrypt_path) {
  bool result = false;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(ProjManager()->projectPath());  // setting path
  std::string output = ProjManager()->projectName() + "_post_synth.blif";
  time_t time_netlist = Mtime(output);
  if (time_netlist == -1) {
    result = true;
  }
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    std::vector<std::string> tokens;
    Tokenize(lang_file.second, " ", tokens);
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
  for (auto path : ProjManager()->includePathList()) {
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
  for (auto path : ProjManager()->libraryPathList()) {
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
  PERF_LOG("Synthesize has started");
  if (!ProjManager()->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << ProjManager()->projectName() << "..."
           << std::endl;

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

  if (m_useVerific) {
    // Verific parser
    std::string fileList;
    std::string includes;
    for (auto path : ProjManager()->includePathList()) {
      includes += path + " ";
    }
    fileList += "verific -vlog-incdir " + includes + "\n";

    std::string libraries;
    for (auto path : ProjManager()->libraryPathList()) {
      libraries += path + " ";
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

    for (const auto& lang_file : ProjManager()->DesignFiles()) {
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
        case Design::Language::VERILOG_NETLIST:
          lang = "";
          break;
        case Design::Language::BLIF:
        case Design::Language::EBLIF:
          lang = "BLIF";
          ErrorMessage("Unsupported file format:" + lang);
          return false;
      }
      fileList += "verific " + lang + " " + lang_file.second + "\n";
    }
    fileList += "verific -import " + ProjManager()->DesignTopModule() + "\n";
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
    //std::string macros = "verilog_defines ";
    //for (auto& macro_value : ProjManager()->macroList()) {
    //  macros += "-D" + macro_value.first + "=" + macro_value.second + " ";
    //}
    //macros += "\n";
    //std::string includes;
    //for (auto path : ProjManager()->includePathList()) {
    //  includes += "-I" + path + " ";
    // }

    yosysScript = ReplaceAll(yosysScript, "${READ_DESIGN_FILES}",
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
    }
    std::string options = lang;
    options += " -nolatches";
    yosysScript = ReplaceAll(yosysScript, "${READ_VERILOG_OPTIONS}", options);
    yosysScript = ReplaceAll(yosysScript, "${INCLUDE_PATHS}", "");
    yosysScript = ReplaceAll(yosysScript, "${VERILOG_FILES}", fileList);
  }

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

  if( (settings_yosys_general_obj.contains("edif")) && 
      (settings_yosys_general_obj["edif"]["default"].get<std::string>() == "checked") ) {

    yosys_options += " -edif " + std::string(m_projManager->projectName() + ".edif");
  }

  yosysScript = ReplaceAll(yosysScript, "${YOSYS_OPTIONS}", yosys_options);

  yosysScript =
      ReplaceAll(yosysScript, "${OUTPUT_VERILOG}",
                 std::string(ProjManager()->projectName() + "_post_synth.v"));

  yosysScript = FinishSynthesisScript(yosysScript);

  std::string script_path = ProjManager()->projectName() + ".ys";
  if (!DesignChanged(yosysScript, script_path)) {
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
//   if (!FileExists(m_yosysExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_yosysExecutablePath.string());
//     return false;
//   }
  std::string command =
      m_yosysExecutablePath.string() + " -s " +
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
    keep = ReplaceAll(keep, "@", "[");
    keep = ReplaceAll(keep, "%", "]");
    (*m_out) << "Keep name: " << keep << "\n";
    keeps += "setattr -set keep 1 " + keep + "\n";
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

  std::string vpr_options;

  // parse vpr general options
  if(settings_vpr_general_obj.contains("device")) {
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

  //std::string pnrOptions;
  //if (!PnROpt().empty()) pnrOptions = " " + PnROpt();

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

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath)) {
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

  // construct the base vpr command with all the options here.
  std::string base_vpr_command =
      m_vprExecutablePath.string() + std::string(" ") +
      m_architectureFile.string() + std::string(" ") +
      std::string(netlistFile) + // NOTE: don't add a " " here as vpr options start with a " "
      vpr_options;
  
  return base_vpr_command;
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
  PERF_LOG("Packing has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
  // const std::string sdcOut =
  //     (std::filesystem::path(ProjManager()->projectName()) /
  //      std::string(ProjManager()->projectName() + "_openfpga.sdc"))
  //         .string();
  // std::ofstream ofssdc(sdcOut);
  // // TODO: Massage the SDC so VPR can understand them
  // for (auto constraint : m_constraints->getConstraints()) {
  //   // Parse RTL and expand the get_ports, get_nets
  //   // Temporary dirty filtering:
  //   constraint = ReplaceAll(constraint, "@", "[");
  //   constraint = ReplaceAll(constraint, "%", "]");
  //   (*m_out) << "Constraint: " << constraint << "\n";
  //   std::vector<std::string> tokens;
  //   Tokenize(constraint, " ", tokens);
  //   constraint = "";
  //   // VPR does not understand: create_clock -period 2 clk -name <logical_name>
  //   // Pass the constraint as-is anyway
  //   for (uint32_t i = 0; i < tokens.size(); i++) {
  //     const std::string& tok = tokens[i];
  //     constraint += tok + " ";
  //   }

  //   // pin location constraints have to be translated to .place:
  //   if (constraint.find("set_pin_loc") != std::string::npos) {
  //     continue;
  //   }
  //   ofssdc << constraint << "\n";
  // }
  // ofssdc.close();

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

  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() + " packing failed!");
    return false;
  }
  m_state = State::Packed;
  (*m_out) << "Design " << ProjManager()->projectName() << " is packed!"
           << std::endl;

  // placeholder for pin_placement process ++
  // we are already loaded, so, no need to read the json again at this point.
  //std::string settings_json_filename = m_projManager->projectName() + ".json";
  //std::string settings_json_path = (std::filesystem::path(settings_json_filename)).string();
  //GetSession()->GetSettings()->loadJsonFile(QString::fromStdString(settings_json_path));
  json settings_general_device_obj = GetSession()->GetSettings()->getJson()["general"]["device"];
  

  std::string family = settings_general_device_obj["family"]["default"].get<std::string>();
  std::string foundry = settings_general_device_obj["foundry"]["default"].get<std::string>();
  std::string node = settings_general_device_obj["node"]["default"].get<std::string>();
  m_OpenFpgaPinMapXml = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("pinmap.xml"));
  m_OpenFpgaPinMapCSV = std::filesystem::path(GetSession()->Context()->DataPath() / family / foundry / node / std::string("pinmap.csv"));

  // use script from project dir:
  //std::filesystem::path python_script_path = std::filesystem::path(std::filesystem::current_path() / std::string("example.py"));
  // use script from scripts dir:
  const char* const path_scripts =
      std::getenv("AURORA2_SCRIPTS_DIR");  // this is from setup.sh
  std::filesystem::path scriptsDir;
  std::error_code ec;
  if (path_scripts != nullptr) {
    std::filesystem::path dirpath = std::string(path_scripts);
    if (std::filesystem::exists(dirpath, ec)) {
      scriptsDir = dirpath;
    }
  }

  // proceed if we have a valid scripts directory
  if (!scriptsDir.empty()) {
    std::filesystem::path python_script_path =
        std::filesystem::path(scriptsDir / std::string("example.py"));
    command = std::string("python3") + std::string(" ") +
              python_script_path.string() + std::string(" ") +
              m_OpenFpgaPinMapXml.string() + std::string(" ") +
              m_OpenFpgaPinMapCSV.string();

    status = ExecuteAndMonitorSystemCommand(command);
    CleanTempFiles();
    if (status) {
      ErrorMessage("Design " + m_projManager->projectName() +
                   " PinPlacement failed!");
      return false;
    }
    (*m_out) << "Design " << m_projManager->projectName()
             << " PinPlacement Done!" << std::endl;
  }
  // placeholder for pin_placement process --

  return true;
}

bool CompilerOpenFPGA_ql::GlobalPlacement() {
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in packed state"));
    return false;
  }
  if (GlobPlacementOpt() == GlobalPlacementOpt::Clean) {
    Message("Cleaning global placement results for " +
            ProjManager()->projectName());
    m_state = State::Packed;
    GlobPlacementOpt(GlobalPlacementOpt::None);
    return true;
  }
  (*m_out) << "Global Placement for design: " << ProjManager()->projectName()
           << "..." << std::endl;
  // TODO:
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << ProjManager()->projectName()
           << " is globally placed!" << std::endl;
  return true;
}

bool CompilerOpenFPGA_ql::Placement() {
  PERF_LOG("Placement has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in packed/globally_placed state"));
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
  (*m_out) << "Placement for design: " << ProjManager()->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
#if 0 // disabling this unless we actually use pin_c executable
  const std::string pcfOut =
      (std::filesystem::path(ProjManager()->projectName()) /
       std::string(ProjManager()->projectName() + "_openfpga.pcf"))
          .string();
  std::ofstream ofspcf(pcfOut);
  bool pinLocConstraints = false;
  for (auto constraint : m_constraints->getConstraints()) {
    std::vector<std::string> tokens;
    Tokenize(constraint, " ", tokens);
    constraint = "";
    for (uint32_t i = 0; i < tokens.size(); i++) {
      const std::string& tok = tokens[i];
      constraint += tok + " ";
    }
    constraint = ReplaceAll(constraint, "@", "[");
    constraint = ReplaceAll(constraint, "%", "]");
    // pin location constraints have to be translated to .place:
    if (constraint.find("set_pin_loc") == std::string::npos) {
      continue;
    }
    constraint = ReplaceAll(constraint, "set_pin_loc", "set_io");
    ofspcf << constraint << "\n";
    pinLocConstraints = true;
  }
  ofspcf.close();
  std::string pin_loc_constraint_file;
  if (pinLocConstraints) {
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

    std::string pincommand = m_pinConvExecutablePath.string();
    if (FileExists(pincommand) && (!m_OpenFpgaPinMapXml.empty())) {
      if (!std::filesystem::is_regular_file(m_OpenFpgaPinMapXml)) {
        ErrorMessage(
            "No pin description xml file available for this device, required "
            "for set_pin_loc constraints");
        return false;
      }
      if (!std::filesystem::is_regular_file(m_OpenFpgaPinMapCSV)) {
        ErrorMessage(
            "No pin description csv file available for this device, required "
            "for set_pin_loc constraints");
        return false;
      }
      pincommand += " --xml " + m_OpenFpgaPinMapXml.string();
      pincommand += " --csv " + m_OpenFpgaPinMapCSV.string();
      pincommand += " --pcf " +
                    std::string(ProjManager()->projectName() + "_openfpga.pcf");
      pincommand += " --blif " + netlistFile;
      std::string pin_locFile = ProjManager()->projectName() + "_pin_loc.place";
      pincommand += " --output " + pin_locFile;

      std::ofstream ofsp(
          (std::filesystem::path(ProjManager()->projectName()) /
           std::string(ProjManager()->projectName() + "_pin_loc.cmd"))
              .string());
      ofsp << pincommand << std::endl;
      ofsp.close();
      int status = ExecuteAndMonitorSystemCommand(pincommand);
      CleanTempFiles();
      if (status) {
        ErrorMessage("Design " + ProjManager()->projectName() +
                     " pin conversion failed!");
        return false;
      } else {
        pin_loc_constraint_file = pin_locFile;
      }
    }
  }
#endif // #if 0 // disabling this unless we actually use pin_c executable
  
  std::string command = BaseVprCommand();
  if(command.empty()) {
    ErrorMessage("Base VPR Command is empty!");
    return false;
  }
  command += std::string(" ") + 
             std::string("--place");

#if 0 // disabling this unless we actually use pin_c executable
  if (!pin_loc_constraint_file.empty()) {
    command += " --fix_pins " + pin_loc_constraint_file;
  }
#endif // #if 0 // disabling this unless we actually use pin_c executable
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

bool CompilerOpenFPGA_ql::Route() {
  PERF_LOG("Route has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Placed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in placed state"));
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
  (*m_out) << "Routing for design: " << ProjManager()->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }
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
  PERF_LOG("TimingAnalysis has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  if (m_state != State::Routed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in routed state"));
    return false;
  }

  (*m_out) << "Analysis for design: " << ProjManager()->projectName() << "..."
           << std::endl;
//   if (!FileExists(m_vprExecutablePath)) {
//     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
//     return false;
//   }

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
             std::string(" ") + 
             std::string("--analysis") +
             std::string(" ") + 
             std::string("--disp on");

  std::ofstream ofs((std::filesystem::path(ProjManager()->projectName()) /
                     std::string(ProjManager()->projectName() + "_sta.cmd"))
                        .string());
  ofs << command << std::endl;
  ofs.close();
  int status = ExecuteAndMonitorSystemCommand(command);
  CleanTempFiles();
  if (status) {
    ErrorMessage("Design " + ProjManager()->projectName() +
                 " timing analysis failed!");
    return false;
  }

  (*m_out) << "Design " << ProjManager()->projectName()
           << " is timing analysed!" << std::endl;

  return true;
}

bool CompilerOpenFPGA_ql::PowerAnalysis() {
  PERF_LOG("PowerAnalysis has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }

  if (m_state != State::Routed) {
    ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in routed state"));
    return false;
  }

  (*m_out) << "Analysis for design: " << ProjManager()->projectName() << "..."
           << std::endl;
  
  //   if (!FileExists(m_vprExecutablePath)) {
  //     ErrorMessage("Cannot find executable: " + m_vprExecutablePath.string());
  //     return false;
  //   }
  
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
             std::string(" ") + 
             std::string("--analysis") +
             std::string(" ") + 
             std::string("--disp on");

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
vpr ${VPR_ARCH_FILE} ${VPR_TESTBENCH_BLIF} --clock_modeling ideal${OPENFPGA_VPR_DEVICE_LAYOUT} --net_file ${NET_FILE} --place_file ${PLACE_FILE} --route_file ${ROUTE_FILE} --route_chan_width ${OPENFPGA_VPR_ROUTE_CHAN_WIDTH} --sdc_file ${SDC_FILE} --absorb_buffer_luts off --constant_net_method route --circuit_format ${OPENFPGA_VPR_CIRCUIT_FORMAT}  --analysis

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

build_architecture_bitstream

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

# Read OpenFPGA simulation setting
read_openfpga_simulation_setting -f ${OPENFPGA_SIM_SETTING_FILE}

# Read OpenFPGA bitstream setting
${READ_OPENFPGA_BITSTREAM_SETTING_COMMAND}

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
#repack --design_constraints ${OPENFPGA_REPACK_CONSTRAINTS}
repack

# Build bitstream database
#build_architecture_bitstream --write_file fabric_independent_bitstream.xml
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

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath)) {
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

  return result;
}

bool CompilerOpenFPGA_ql::GenerateBitstream() {
  PERF_LOG("GenerateBitstream has started");
  if (!ProjManager()->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  
  if (BitsOpt() == BitstreamOpt::NoBitsOpt) {
    if (m_state != State::Routed) {
      ErrorMessage(std::string(__func__) + std::string("(): Design needs to be in routed state"));
      return false;
    }
  }

  (*m_out) << "Bitstream generation for design: "
           << ProjManager()->projectName() << "..." << std::endl;

  std::string command = m_openFpgaExecutablePath.string() + " -f " +
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
//   if (!FileExists(m_openFpgaExecutablePath)) {
//     ErrorMessage("Cannot find executable: " +
//                  m_openFpgaExecutablePath.string());
//     return false;
//   }

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
                 " bitream generation failed!");
    return false;
  }
  m_state = State::BistreamGenerated;

  (*m_out) << "Design " << ProjManager()->projectName()
           << " bitstream is generated!" << std::endl;
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
              std::filesystem::path fullPath;
              if (FileExists(file)) {
                fullPath = file;  // Absolute path
              } else {
                fullPath = datapath / std::string("etc") /
                           std::string("devices") / file;
              }
              if (!FileExists(fullPath.string())) {
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
                // if (name == "QL")
                //   SynthType(SynthesisType::QL);
                // else if (name == "RS")
                //   SynthType(SynthesisType::RS);
                // else if (name == "Yosys")
                //   SynthType(SynthesisType::Yosys);
                // else {
                //   ErrorMessage("Invalid synthesis type: " + name + "\n");
                //   status = false;
                // }
              } else if (file_type == "synth_opts") {
                DefaultSynthOptions(name);
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

  return status;
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

    // generate a temp file path in the temp directory
    std::string temp_file_path_str = std::tmpnam(nullptr);
    std::filesystem::path temp_file_path(temp_file_path_str);

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
      family = dir_entry_family.path().filename();

      // look at the directories inside the 'family' dir for 'foundry' entries
      for (const std::filesystem::directory_entry& dir_entry_foundry : 
                    std::filesystem::directory_iterator(dir_entry_family.path())) {

        if(dir_entry_foundry.is_directory()) {
      
          // we would see foundry at this level
          foundry = dir_entry_foundry.path().filename();

          // look at the directories inside the 'foundry' dir for 'node' entries
          for (const std::filesystem::directory_entry& dir_entry_node : 
                          std::filesystem::directory_iterator(dir_entry_foundry.path())) {

            if(dir_entry_node.is_directory()) {
            
              // we would see devices at this level
              node = dir_entry_node.path().filename();

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
