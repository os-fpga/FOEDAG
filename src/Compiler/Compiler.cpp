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

#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QProcess>
#include <charconv>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler.h"
#include "Compiler/Constraints.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "CompilerDefines.h"
#include "IPGenerate/IPCatalogBuilder.h"
#include "Log.h"
#include "Main/Settings.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "TaskManager.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

extern const char* foedag_version_number;
extern const char* foedag_git_hash;
extern const char* foedag_build_type;

void Compiler::Version(std::ostream* out) {
  (*out) << "Foedag FPGA Compiler"
         << "\n";
  PrintVersion(out);
}

void Compiler::Help(std::ostream* out) {
  (*out) << "-------------------------" << std::endl;
  (*out) << "-----  FOEDAG HELP  -----" << std::endl;
  (*out) << "-------------------------" << std::endl;
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
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help                       : This help" << std::endl;
  (*out) << "   create_design <name>       : Creates a design with <name> name"
         << std::endl;
  (*out)
      << "   add_design_file <option> (-work, -L) <libName> <file>... <type> "
         "(-VHDL_1987, -VHDL_1993, -VHDL_2000, -VHDL_2008, -V_1995, "
         "-V_2001, -SV_2005, -SV_2009, -SV_2012, -SV_2017) "
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
  (*out) << "     Constraints: set_pin_loc, set_region_loc, all SDC commands"
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
  (*out)
      << "   synthesize <optimization> ?clean? : Optional optimization (area, "
         "delay, mixed, none)"
      << std::endl;
  (*out) << "   place ?clean" << std::endl;
  (*out)
      << "   pin_loc_assign_method <Method>: (in_define_order(Default)/random)"
      << std::endl;
  (*out) << "   synth_options <option list>: Synthesis Options" << std::endl;
  (*out) << "   pnr_options <option list>  : PnR Options" << std::endl;
  (*out) << "   packing ?clean?" << std::endl;
  (*out) << "   global_placement ?clean?" << std::endl;
  (*out) << "   place ?clean?" << std::endl;
  (*out) << "   route ?clean?" << std::endl;
  (*out) << "   sta ?clean?" << std::endl;
  (*out) << "   power ?clean?" << std::endl;
  (*out) << "   bitstream ?clean?" << std::endl;
  (*out) << "   tcl_exit" << std::endl;
  (*out) << "-------------------------" << std::endl;
}

Compiler::Compiler(TclInterpreter* interp, std::ostream* out,
                   TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_out(out),
      m_tclInterpreterHandler(tclInterpreterHandler) {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
  m_constraints = new Constraints();
  m_constraints->registerCommands(interp);
  IPCatalog* catalog = new IPCatalog();
  m_IPGenerator = new IPGenerator(catalog, this);
}

void Compiler::SetTclInterpreterHandler(
    TclInterpreterHandler* tclInterpreterHandler) {
  m_tclInterpreterHandler = tclInterpreterHandler;
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
}

Compiler::~Compiler() {
  delete m_taskManager;
  delete m_tclCmdIntegration;
  delete m_IPGenerator;
}

void Compiler::Message(const std::string& message) {
  if (m_out) (*m_out) << message << std::endl;
}

void Compiler::ErrorMessage(const std::string& message) {
  if (m_err) (*m_err) << "ERROR: " << message << std::endl;
  Tcl_AppendResult(m_interp->getInterp(), message.c_str(), nullptr);
}

static std::string TclInterpCloneScript() {
  std::string script = R"(
    # Simple Tcl Interpreter State copy utility
  proc tcl_interp_clone { } {
    set procs [info proc]
    set script ""
    foreach pr $procs {
        set args [info args $pr]
        set body [info body $pr]
        append script "proc $pr {$args} {
$body
}

"
    }

    foreach gl [info global] {
        if {$gl == {tcl_interactive}} {
          continue
        }
        if {$gl == {errorInfo}} {
          continue
        }
        upvar $gl x
        if [array exist x] {
        } else {
            append script "set $gl \"$x\"

"
        }
    }
    return $script  
}
tcl_interp_clone
    )";

  return script;
}

static std::string TclInterpCloneVar() {
  std::string script = R"(
    # Simple Tcl Interpreter State copy utility
  proc tcl_interp_clone { } {
    foreach gl [info global] {
        if {$gl == {tcl_interactive}} {
          continue
        }
        upvar $gl x
        if [array exist x] {
        } else {
            append script "set $gl \"$x\"

"
        }
    }
    return $script  
}
tcl_interp_clone

    )";

  return script;
}

bool Compiler::BuildLiteXIPCatalog(std::filesystem::path litexPath) {
  if (m_IPGenerator == nullptr) {
    IPCatalog* catalog = new IPCatalog();
    m_IPGenerator = new IPGenerator(catalog, this);
  }
  IPCatalogBuilder builder(this);
  bool result =
      builder.buildLiteXCatalog(GetIPGenerator()->Catalog(), litexPath);
  return result;
}

bool Compiler::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  if (m_IPGenerator == nullptr) {
    IPCatalog* catalog = new IPCatalog();
    m_IPGenerator = new IPGenerator(catalog, this);
  }
  m_IPGenerator->RegisterCommands(interp, batchMode);
  if (m_constraints == nullptr) {
    m_constraints = new Constraints();
    m_constraints->registerCommands(interp);
  }

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    compiler->Help(compiler->GetOutStream());
    return TCL_OK;
  };
  interp->registerCmd("help", help, this, 0);

  auto version = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    compiler->Version(compiler->GetOutStream());
    return TCL_OK;
  };
  interp->registerCmd("version", version, this, 0);

  auto get_state = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    auto state = compiler->CompilerState();
    std::array<char, 3> str;
    str.fill('\0');
    if (auto [ptr, ec] = std::to_chars(str.data(), str.data() + str.size(),
                                       static_cast<int>(state));
        ec == std::errc()) {
      Tcl_AppendResult(interp, str.data(), nullptr);
      return TCL_OK;
    } else {
      Tcl_AppendResult(interp, std::make_error_code(ec).message().c_str(),
                       nullptr);
      return TCL_ERROR;
    }
  };
  interp->registerCmd("get_state", get_state, this, nullptr);

  auto create_design = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc == 2) {
      name = argv[1];
    }
    compiler->GetOutput().clear();
    bool ok = compiler->CreateDesign(name);
    if (!compiler->m_output.empty())
      Tcl_AppendResult(interp, compiler->m_output.c_str(), nullptr);
    if (!FileUtils::FileExists(name)) {
      compiler->Message("Create design directory: " + name);
      bool created = std::filesystem::create_directory(name);
      if (!created) {
        ok = created;
        compiler->ErrorMessage("Cannot create design directory: " + name);
      }
    }
    return ok ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("create_design", create_design, this, 0);

  auto set_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc != 2) {
      compiler->ErrorMessage("Specify a top module name");
      return TCL_ERROR;
    }
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok = compiler->m_tclCmdIntegration->TclSetTopModule(argc, argv, out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_top_module", set_top_module, this, 0);

  auto add_design_file = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc < 2) {
      compiler->ErrorMessage(
          "Incorrect syntax for add_design_file <file(s)> "
          "[-work libraryName] [-L libraryName1 -L libraryName2]"
          "<type (-VHDL_1987, -VHDL_1993, -VHDL_2000, -VHDL_2008 (.vhd "
          "default), -V_1995, "
          "-V_2001 (.v default), -SV_2005, -SV_2009, -SV_2012, -SV_2017 (.sv "
          "default))>");
      return TCL_ERROR;
    }
    std::string actualType = "VERILOG_2001";
    Design::Language language = Design::Language::VERILOG_2001;
    const std::string file = argv[1];
    if (strstr(file.c_str(), ".vhd")) {
      language = Design::Language::VHDL_2008;
      actualType = "VHDL_2008";
    }
    if (strstr(file.c_str(), ".sv")) {
      language = Design::Language::SYSTEMVERILOG_2017;
      actualType = "SV_2017";
    }
    std::string commandsList;
    std::string libList;
    std::string fileList;
    for (int i = 1; i < argc; i++) {
      const std::string type = argv[i];
      if (type == "-work" || type == "-L") {
        if (i + 1 >= argc) {
          compiler->ErrorMessage(
              "Incorrect syntax for add_design_file <file(s)> "
              "Library name should follow '-work' or '-L' tags");
          return TCL_ERROR;
        }
        commandsList += type + " ";
        const std::string libName = argv[++i];
        libList += libName + " ";
      } else if (type == "-VHDL_1987") {
        language = Design::Language::VHDL_1987;
        actualType = "VHDL_1987";
      } else if (type == "-VHDL_1993") {
        language = Design::Language::VHDL_1993;
        actualType = "VHDL_1993";
      } else if (type == "-VHDL_2000") {
        language = Design::Language::VHDL_2000;
        actualType = "VHDL_2000";
      } else if (type == "-VHDL_2008") {
        language = Design::Language::VHDL_2008;
        actualType = "VHDL_2008";
      } else if (type == "-V_1995") {
        language = Design::Language::VERILOG_1995;
        actualType = "VERILOG_1995";
      } else if (type == "-V_2001") {
        language = Design::Language::VERILOG_2001;
        actualType = "VERILOG_2001";
      } else if (type == "-V_2005") {
        language = Design::Language::SYSTEMVERILOG_2005;
        actualType = "SV_2005";
      } else if (type == "-SV_2009") {
        language = Design::Language::SYSTEMVERILOG_2009;
        actualType = "SV_2009";
      } else if (type == "-SV_2012") {
        language = Design::Language::SYSTEMVERILOG_2012;
        actualType = "SV_2012";
      } else if (type == "-SV_2017") {
        language = Design::Language::SYSTEMVERILOG_2017;
        actualType = "SV_2017";
      } else if (type.find("-D") != std::string::npos) {
        fileList += type + " ";
      } else {
        const std::string file = argv[i];
        std::string expandedFile = file;
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
          fullPath.append(file);
          expandedFile = fullPath.string();
        }
        std::filesystem::path the_path = expandedFile;
        if (!the_path.is_absolute()) {
          const auto& path = std::filesystem::current_path();
          expandedFile = std::filesystem::path(path / expandedFile).string();
        }
        fileList += expandedFile + " ";
      }
    }

    compiler->Message(std::string("Adding ") + actualType + " " + fileList +
                      std::string("\n"));
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok = compiler->m_tclCmdIntegration->TclAddDesignFiles(
          commandsList.c_str(), libList.c_str(), fileList.c_str(), language,
          out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("add_design_file", add_design_file, this, nullptr);

  auto read_netlist = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc < 2) {
      compiler->ErrorMessage("Incorrect syntax for read_netlist <file>");
      return TCL_ERROR;
    }

    const std::string file = argv[1];
    std::string actualType = "VERILOG";
    Design::Language language = Design::Language::VERILOG_NETLIST;
    if (strstr(file.c_str(), ".blif")) {
      language = Design::Language::BLIF;
      actualType = "BLIF";
    } else if (strstr(file.c_str(), ".eblif")) {
      language = Design::Language::EBLIF;
      actualType = "EBLIF";
    }

    std::string expandedFile = file;
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
      fullPath.append(file);
      expandedFile = fullPath.string();
    }
    std::filesystem::path the_path = expandedFile;
    std::string origPathFileList = expandedFile;
    if (!the_path.is_absolute()) {
      const auto& path = std::filesystem::current_path();
      expandedFile = std::filesystem::path(path / expandedFile).string();
    }

    compiler->Message(std::string("Reading ") + actualType + " " +
                      expandedFile + std::string("\n"));
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok = compiler->m_tclCmdIntegration->TclAddDesignFiles(
          {}, {}, origPathFileList.c_str(), language, out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("read_netlist", read_netlist, this, nullptr);

  auto add_include_path = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string file = argv[i];
      std::string expandedFile = file;
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
        fullPath.append(file);
        expandedFile = fullPath.string();
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
      }
      compiler->ProjManager()->addIncludePath(expandedFile);
    }
    return TCL_OK;
  };
  interp->registerCmd("add_include_path", add_include_path, this, nullptr);

  auto add_library_path = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string file = argv[i];
      std::string expandedFile = file;
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
        fullPath.append(file);
        expandedFile = fullPath.string();
      }
      std::filesystem::path the_path = expandedFile;
      if (!the_path.is_absolute()) {
        expandedFile =
            std::filesystem::path(std::filesystem::path("..") / expandedFile)
                .string();
      }
      compiler->ProjManager()->addLibraryPath(expandedFile);
    }
    return TCL_OK;
  };
  interp->registerCmd("add_library_path", add_library_path, this, nullptr);

  auto add_library_ext = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      std::string ext = argv[i];
      compiler->ProjManager()->addLibraryExtension(ext);
    }
    return TCL_OK;
  };
  interp->registerCmd("add_library_ext", add_library_ext, this, nullptr);

  auto set_macro = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    ProjectManager* proj = compiler->m_tclCmdIntegration->GetProjectManager();
    for (int i = 1; i < argc; i++) {
      const std::string arg = argv[i];
      const size_t loc = arg.find('=');
      if (loc == std::string::npos) {
        const std::string def = arg.substr(2);
        proj->addMacro(def, "");
      } else {
        const std::string def = arg.substr(0, loc);
        const std::string value = arg.substr(loc + 1);
        proj->addMacro(def, value);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_macro", set_macro, this, nullptr);

  auto add_constraint_file = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc != 2) {
      compiler->ErrorMessage("Specify a constraint file name");
      return TCL_ERROR;
    }
    const std::string file = argv[1];
    std::string expandedFile = file;
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
      fullPath.append(file);
      expandedFile = fullPath.string();
    }
    std::filesystem::path the_path = expandedFile;
    if (!the_path.is_absolute()) {
      const auto& path = std::filesystem::current_path();
      expandedFile = std::filesystem::path(path / expandedFile).string();
    }
    compiler->Message(std::string("Adding constraint file ") + expandedFile +
                      std::string("\n"));
    int status = Tcl_Eval(
        interp, std::string("read_sdc {" + expandedFile + "}").c_str());
    if (status) {
      return TCL_ERROR;
    }
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok = compiler->m_tclCmdIntegration->TclAddConstrFiles(
          expandedFile.c_str(), out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }

    return TCL_OK;
  };
  interp->registerCmd("add_constraint_file", add_constraint_file, this, 0);

  auto pnr_options = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::string opts;
    for (int i = 1; i < argc; i++) {
      std::string opt = argv[i];
      std::string expandedFile = opt;
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
        fullPath.append(opt);
        expandedFile = fullPath.string();
      }
      if (FileUtils::FileExists(expandedFile)) {
        std::filesystem::path p = expandedFile;
        p = std::filesystem::absolute(p);
        opt = p.string();
      }
      opts += opt;
      if (i < (argc - 1)) {
        opts += " ";
      }
    }
    compiler->PnROpt(opts);
    return TCL_OK;
  };
  interp->registerCmd("pnr_options", pnr_options, this, 0);

  auto synth_options = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::string opts;
    for (int i = 1; i < argc; i++) {
      opts += std::string(argv[i]);
      if (i < (argc - 1)) {
        opts += " ";
      }
    }
    compiler->SynthMoreOpt(opts);
    return TCL_OK;
  };
  interp->registerCmd("synth_options", synth_options, this, 0);

  // Long runtime commands have to have different scheduling in batch and GUI
  // modes
  if (batchMode) {
    auto ipgenerate = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->IPGenOpt(Compiler::IPGenerateOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::IPGen) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("ipgenerate", ipgenerate, this, 0);

    auto analyze = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown analysis option: " + arg);
        }
      }
      return compiler->Compile(Action::Analyze) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("analyze", analyze, this, 0);

    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "mixed") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Mixed);
        } else if (arg == "area") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Area);
        } else if (arg == "delay") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Delay);
        } else if (arg == "none") {
          compiler->SynthOpt(Compiler::SynthesisOpt::None);
        } else if (arg == "clean") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown optimization option: " + arg);
        }
      }
      return compiler->Compile(Action::Synthesis) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

    auto packing = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PackOpt(Compiler::PackingOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::Pack) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("packing", packing, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->GlobPlacementOpt(Compiler::GlobalPlacementOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::Global) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("global_placement", globalplacement, this, 0);
    interp->registerCmd("globp", globalplacement, this, 0);

    auto placement = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PlaceOpt(Compiler::PlacementOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::Detailed) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("detailed_placement", placement, this, 0);
    interp->registerCmd("place", placement, this, 0);

    auto pin_loc_assign_method = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      auto setPlaceOption = [compiler](const std::string& arg) {
        if (arg == "random") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
          compiler->Message("Pin Method :" + arg);
        } else if (arg == "in_define_order") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
          compiler->Message("Pin Method :" + arg);
        } else {
          compiler->ErrorMessage("Unknown Placement Option: " + arg);
        }
      };

      // If we received a tcl argument
      if (argc > 1) {
        setPlaceOption(argv[1]);
      } else {
        compiler->ErrorMessage(
            "No Argument passed: type random/in_define_order");
        return TCL_ERROR;
      }
      return TCL_OK;
    };
    interp->registerCmd("pin_loc_assign_method", pin_loc_assign_method, this,
                        0);

    auto route = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->RouteOpt(Compiler::RoutingOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::Routing) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("route", route, this, 0);

    auto sta = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::Clean);
        } else if (arg == "view") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::View);
        } else if (arg == "opensta") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::Opensta);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::STA) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("sta", sta, this, 0);

    auto power = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PowerAnalysisOpt(Compiler::PowerOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      return compiler->Compile(Action::Power) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("power", power, this, 0);

    auto bitstream = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "force") {
#ifndef PRODUCTION_BUILD
          compiler->BitsOpt(Compiler::BitstreamOpt::Force);
#endif
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown bitstream option: " + arg);
        }
      }
      return compiler->Compile(Action::Bitstream) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("bitstream", bitstream, this, 0);

    auto stop = [](void* clientData, Tcl_Interp* interp, int argc,
                   const char* argv[]) -> int {
      for (auto th : ThreadPool::threads) {
        th->stop();
      }
      ThreadPool::threads.clear();
      return 0;
    };
    interp->registerCmd("stop", stop, 0, 0);
    interp->registerCmd("abort", stop, 0, 0);
  } else {
    auto ipgenerate = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->IPGenOpt(Compiler::IPGenerateOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("ip_th", Action::IPGen, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("ipgenerate", ipgenerate, this, 0);

    auto analyze = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown analysis option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("analyze_th", Action::Analyze, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("analyze", analyze, this, 0);

    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;

      auto setSynthOption = [compiler](std::string arg) {
        if (arg == "mixed") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Mixed);
        } else if (arg == "area") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Area);
        } else if (arg == "delay") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Delay);
        } else if (arg == "none") {
          compiler->SynthOpt(Compiler::SynthesisOpt::None);
        } else if (arg == "clean") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown optimization option: " + arg);
        }
      };

      // If we received a tcl argument
      if (argc > 1) {
        for (int i = 1; i < argc; i++) {
          setSynthOption(argv[i]);
        }
      } else {
        // otherwise, check settings for a value
        Settings* settings = compiler->GetSession()->GetSettings();
        auto json = settings->getJson()["Tasks"]["Synthesis"]["opt_dropdown"];
        std::string option = "<unset>";
        if (json.contains("userValue")) {
          option = json["userValue"];
        } else if (json.contains("default")) {
          option = json["default"].get<std::string>();
        }

        // If a valid value was specified update the SynthOpt
        if (option != "<unset>") {
          QString lookupVal =
              Settings::getLookupValue(json, QString::fromStdString(option));
          setSynthOption(lookupVal.toStdString());
        }
      }
      WorkerThread* wthread =
          new WorkerThread("synth_th", Action::Synthesis, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

    auto packing = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PackOpt(Compiler::PackingOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("pack_th", Action::Pack, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("packing", packing, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->GlobPlacementOpt(Compiler::GlobalPlacementOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("glob_th", Action::Global, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("global_placement", globalplacement, this, 0);
    interp->registerCmd("globp", globalplacement, this, 0);

    auto placement = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PlaceOpt(Compiler::PlacementOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("place_th", Action::Detailed, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("detailed_placement", placement, this, 0);
    interp->registerCmd("place", placement, this, 0);

    auto pin_loc_assign_method = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      auto setPlaceOption = [compiler](const std::string& arg) {
        if (arg == "random") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
          compiler->Message("Pin Method :" + arg);
        } else if (arg == "in_define_order") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
          compiler->Message("Pin Method :" + arg);
        } else {
          compiler->ErrorMessage("Unknown Placement Option: " + arg);
        }
      };

      // If we received a tcl argument
      if (argc > 1) {
        setPlaceOption(argv[1]);
      } else {
        compiler->ErrorMessage(
            "No Argument passed: type random/in_define_order");
        return TCL_ERROR;
      }
      return TCL_OK;
    };
    interp->registerCmd("pin_loc_assign_method", pin_loc_assign_method, this,
                        0);

    auto route = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->RouteOpt(Compiler::RoutingOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("route_th", Action::Routing, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("route", route, this, 0);

    auto sta = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::Clean);
        } else if (arg == "view") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::View);
        } else if (arg == "opensta") {
          compiler->TimingAnalysisOpt(Compiler::STAOpt::Opensta);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread = new WorkerThread("sta_th", Action::STA, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("sta", sta, this, 0);

    auto power = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->PowerAnalysisOpt(Compiler::PowerOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("power_th", Action::Power, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("power", power, this, 0);

    auto bitstream = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "force") {
#ifndef PRODUCTION_BUILD
          compiler->BitsOpt(Compiler::BitstreamOpt::Force);
#endif
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown bitstream option: " + arg);
        }
      }
      WorkerThread* wthread =
          new WorkerThread("bitstream_th", Action::Bitstream, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("bitstream", bitstream, this, 0);

    auto stop = [](void* clientData, Tcl_Interp* interp, int argc,
                   const char* argv[]) -> int {
      for (auto th : ThreadPool::threads) {
        th->stop();
      }
      ThreadPool::threads.clear();
      return 0;
    };
    interp->registerCmd("stop", stop, 0, 0);
    interp->registerCmd("abort", stop, 0, 0);

    auto batch = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      std::string script;

      // Pass state from master to worker interpreter
      Tcl_Eval(interp, "set tcl_interactive false");
      std::string interpStateScript = TclInterpCloneScript();
      Tcl_Eval(interp, interpStateScript.c_str());
      script = Tcl_GetStringResult(interp);
      Tcl_Eval(interp, "set tcl_interactive true");
      std::string payload;
      // Build batch script
      for (int i = 1; i < argc; i++) {
        payload += argv[i] + std::string(" ");
      }
      script += payload;

      compiler->BatchScript(script);
      WorkerThread* wthread =
          new WorkerThread("batch_th", Action::Batch, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("batch", batch, this, 0);

    auto update_result = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      std::string script = compiler->getResult();

      // Pass state from worker interpreter to master
      Tcl_Eval(interp, "set tcl_interactive false");
      Tcl_Eval(interp, script.c_str());
      Tcl_Eval(interp, "set tcl_interactive true");

      return 0;
    };
    interp->registerCmd("update_result", update_result, this, 0);
  }
  auto architecture = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    // stub function
    return TCL_OK;
  };
  interp->registerCmd("architecture", architecture, nullptr, nullptr);
  return true;
}

bool Compiler::Compile(Action action) {
  uint task{toTaskId(static_cast<int>(action), this)};
  m_stop = false;
  bool res{false};
  if (task != TaskManager::invalid_id && m_taskManager) {
    m_taskManager->task(task)->setStatus(TaskStatus::InProgress);
  }
  res = RunCompileTask(action);
  if (task != TaskManager::invalid_id && m_taskManager) {
    m_taskManager->task(task)->setStatus(res ? TaskStatus::Success
                                             : TaskStatus::Fail);
  }
  return res;
}

void Compiler::Stop() {
  m_stop = true;
  if (m_process) m_process->terminate();
}

bool Compiler::Analyze() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  if (AnalyzeOpt() == DesignAnalysisOpt::Clean) {
    Message("Cleaning analysis results for " + m_projManager->projectName());
    m_state = State::IPGenerated;
    AnalyzeOpt(DesignAnalysisOpt::None);
    return true;
  }
  (*m_out) << "Analyzing design: " << m_projManager->projectName() << "..."
           << std::endl;
  auto currentPath = std::filesystem::current_path();
  auto it = std::filesystem::directory_iterator{currentPath};
  for (int i = 0; i < 100; i = i + 10) {
    (*m_out) << std::setw(2) << i << "%";
    if (it != std::filesystem::end(it)) {
      std::string str =
          " File: " + (*it).path().filename().string() + " just for test";
      (*m_out) << str;
      it++;
    }
    (*m_out) << std::endl;
    std::chrono::milliseconds dura(1000);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Analyzed;
  (*m_out) << "Design " << m_projManager->projectName() << " is analyzed!"
           << std::endl;
  return true;
}

bool Compiler::Synthesize() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + m_projManager->projectName());
    m_state = State::IPGenerated;
    SynthOpt(SynthesisOpt::None);
    return true;
  }
  (*m_out) << "Synthesizing design: " << m_projManager->projectName() << "..."
           << std::endl;
  for (auto constraint : m_constraints->getConstraints()) {
    (*m_out) << "Constraint: " << constraint << "\n";
  }
  for (auto keep : m_constraints->GetKeeps()) {
    (*m_out) << "Keep name: " << keep << "\n";
  }
  auto currentPath = std::filesystem::current_path();
  auto it = std::filesystem::directory_iterator{currentPath};
  for (int i = 0; i < 100; i = i + 10) {
    (*m_out) << std::setw(2) << i << "%";
    if (it != std::filesystem::end(it)) {
      std::string str =
          " File: " + (*it).path().filename().string() + " just for test";
      (*m_out) << str;
      it++;
    }
    (*m_out) << std::endl;
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Synthesized;
  (*m_out) << "Design " << m_projManager->projectName() << " is synthesized!"
           << std::endl;
  return true;
}

bool Compiler::GlobalPlacement() {
  if (!m_projManager->HasDesign()) {
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
  if (m_state != State::Packed && m_state != State::GloballyPlaced) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_projManager->projectName()
           << "..." << std::endl;
  for (int i = 0; i < 100; i = i + 10) {
    (*m_out) << i << "%" << std::endl;
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << m_projManager->projectName()
           << " is globally placed!" << std::endl;
  return true;
}

bool Compiler::RunBatch() {
  (*m_out) << "Running batch..." << std::endl;
  TclInterpreter* batchInterp = new TclInterpreter("batchInterp");
  if (m_tclInterpreterHandler)
    m_tclInterpreterHandler->initIterpreter(batchInterp);
  RegisterCommands(batchInterp, true);
  (*m_out) << batchInterp->evalCmd(m_batchScript);
  (*m_out) << std::endl << "Batch Done." << std::endl;

  // Save resulting state
  batchInterp->evalCmd("set tcl_interactive false");
  m_result = batchInterp->evalCmd(TclInterpCloneVar());
  batchInterp->evalCmd("set tcl_interactive true");
  return true;
}

void Compiler::start() {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->notifyStart();
}

void Compiler::finish() {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->notifyFinish();
}

bool Compiler::RunCompileTask(Action action) {
  switch (action) {
    case Action::IPGen:
      return IPGenerate();
    case Action::Analyze:
      return Analyze();
    case Action::Synthesis:
      return Synthesize();
    case Action::Pack:
      return Packing();
    case Action::Global:
      return GlobalPlacement();
    case Action::Detailed:
      return Placement();
    case Action::Routing:
      return Route();
    case Action::STA:
      return TimingAnalysis();
    case Action::Power:
      return PowerAnalysis();
    case Action::Bitstream:
      return GenerateBitstream();
    case Action::Batch:
      return RunBatch();
    default:
      break;
  }
  return false;
}

void Compiler::setTaskManager(TaskManager* newTaskManager) {
  m_taskManager = newTaskManager;
  if (m_taskManager) {
    m_taskManager->bindTaskCommand(IP_GENERATE, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("ipgenerate"));
    });
    m_taskManager->bindTaskCommand(SYNTHESIS, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("synth"));
    });
    m_taskManager->bindTaskCommand(SYNTHESIS_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("synth clean"));
    });
    m_taskManager->bindTaskCommand(PACKING, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("packing"));
    });
    m_taskManager->bindTaskCommand(PACKING_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("packing clean"));
    });
    m_taskManager->bindTaskCommand(GLOBAL_PLACEMENT, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("globp"));
    });
    m_taskManager->bindTaskCommand(GLOBAL_PLACEMENT_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("globp clean"));
    });
    m_taskManager->bindTaskCommand(PLACEMENT, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("place"));
    });
    m_taskManager->bindTaskCommand(PLACEMENT_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("place clean"));
    });
    m_taskManager->bindTaskCommand(ROUTING, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("route"));
    });
    m_taskManager->bindTaskCommand(ROUTING_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("route clean"));
    });
    m_taskManager->bindTaskCommand(TIMING_SIGN_OFF, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("sta"));
    });
    m_taskManager->bindTaskCommand(POWER, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("power"));
    });
    m_taskManager->bindTaskCommand(BITSTREAM, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("bitstream"));
    });
    m_taskManager->bindTaskCommand(PLACE_AND_ROUTE_VIEW, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("sta view"));
    });
  }
}

void Compiler::setGuiTclSync(TclCommandIntegration* tclCommands) {
  m_tclCmdIntegration = tclCommands;
  if (m_tclCmdIntegration)
    m_projManager = m_tclCmdIntegration->GetProjectManager();
}

bool Compiler::IPGenerate() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_projManager->projectName()
           << "..." << std::endl;
  bool status = GetIPGenerator()->Generate();
  if (status) {
    (*m_out) << "Design " << m_projManager->projectName()
             << " IPs are generated!" << std::endl;
    m_state = State::IPGenerated;
  } else {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " IPs generation failed!");
  }
  return status;
}

bool Compiler::Packing() {
  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    std::filesystem::remove(
        std::filesystem::path(ProjManager()->projectPath()) /
        std::string(ProjManager()->projectName() + "_post_synth.net"));
    return true;
  }
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Packing for design: " << m_projManager->projectName() << "..."
           << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " is packed!"
           << std::endl;
  m_state = State::Packed;
  return true;
}

bool Compiler::Placement() {
  if (!m_projManager->HasDesign()) {
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
  (*m_out) << "Placement for design: " << m_projManager->projectName() << "..."
           << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " is placed!"
           << std::endl;
  m_state = State::Placed;
  return true;
}

bool Compiler::Route() {
  if (!m_projManager->HasDesign()) {
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
  (*m_out) << "Routing for design: " << m_projManager->projectName() << "..."
           << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " is routed!"
           << std::endl;
  m_state = State::Routed;
  return true;
}

bool Compiler::TimingAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Timing analysis for design: " << m_projManager->projectName()
           << "..." << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " is analyzed!"
           << std::endl;
  return true;
}

bool Compiler::PowerAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Timing analysis for design: " << m_projManager->projectName()
           << "..." << std::endl;

  (*m_out) << "Design " << m_projManager->projectName() << " is analyzed!"
           << std::endl;
  return true;
}

bool Compiler::GenerateBitstream() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Bitstream generation for design: "
           << m_projManager->projectName() << "..." << std::endl;

  (*m_out) << "Design " << m_projManager->projectName()
           << " bitstream is generated!" << std::endl;
  return true;
}

bool Compiler::VerifyTargetDevice() const {
  return !ProjManager()->getTargetDevice().empty();
}

bool Compiler::HasTargetDevice() {
  if (!VerifyTargetDevice()) {
    ErrorMessage("Please specify target device or architecture file");
    return false;
  }
  return true;
}

bool Compiler::CreateDesign(const std::string& name) {
  if (m_tclCmdIntegration) {
    if (m_projManager->HasDesign()) {
      ErrorMessage("Design already exists");
      return false;
    }

    std::ostringstream out;
    bool ok = m_tclCmdIntegration->TclCreateProject(name.c_str(), out);
    if (!out.str().empty()) m_output = out.str();
    if (!ok) return false;
    Message(std::string("Created design: ") + name + std::string("\n"));
  }
  return true;
}

void Compiler::PrintVersion(std::ostream* out) {
  if (std::string(foedag_version_number) != "${VERSION_NUMBER}")
    (*out) << "Version    : " << foedag_version_number << "\n";
  if (std::string(foedag_git_hash) != "${GIT_HASH}")
    (*out) << "Git Hash   : " << foedag_git_hash << "\n";
  (*out) << "Built      : " << __DATE__ << "\n";
  (*out) << "Built type : " << foedag_build_type << "\n";
}

bool Compiler::ExecuteSystemCommand(const std::string& command) {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  // TODO: Windows System call
#else
  int result = system(command.c_str());
  if (result == 0) {
    return true;
  }
#endif

  return false;
}

const std::string Compiler::GetNetlistPath() {
  std::string netlistFile =
      (std::filesystem::path(ProjManager()->projectPath()) /
       (ProjManager()->projectName() + "_post_synth.blif"))
          .string();

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first) {
      case Design::Language::VERILOG_NETLIST:
      case Design::Language::BLIF:
      case Design::Language::EBLIF: {
        netlistFile = lang_file.second;
        std::filesystem::path the_path = netlistFile;
        if (!the_path.is_absolute()) {
          netlistFile = std::filesystem::path(netlistFile).string();
        }
        break;
      }
      default:
        break;
    }
  }
  return netlistFile;
}

int Compiler::ExecuteAndMonitorSystemCommand(const std::string& command) {
  auto start = Time::now();
  PERF_LOG("Command: " + command);
  (*m_out) << "Command: " << command << std::endl;
  auto path = std::filesystem::current_path();  // getting path
  (*m_out) << "Path: " << path.string() << std::endl;
  std::filesystem::current_path(m_projManager->projectPath());  // setting path
  (*m_out) << "Changed path to: " << std::filesystem::current_path().string()
           << std::endl;
  // new QProcess must be created here to avoid issues related to creating
  // QObjects in different threads
  m_process = new QProcess;
  if (m_out)
    QObject::connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
      m_out->write(m_process->readAllStandardOutput(),
                   m_process->bytesAvailable());
    });
  if (m_err)
    QObject::connect(m_process, &QProcess::readyReadStandardError, [this]() {
      QByteArray data = m_process->readAllStandardError();
      m_err->write(data, data.size());
    });

  ProcessUtils utils;
  QObject::connect(m_process, &QProcess::started,
                   [&utils, this]() { utils.Start(m_process->processId()); });

  QString cmd{command.c_str()};
  QStringList args = cmd.split(" ");
  QString program = args.first();
  args.pop_front();  // remove program
  m_process->start(program, args);
  std::filesystem::current_path(path);
  (*m_out) << "Changed path to: " << (path).string() << std::endl;
  m_process->waitForFinished(-1);
  utils.Stop();
  uint max_utiliation{utils.Utilization()};
  auto status = m_process->exitStatus();
  auto exitCode = m_process->exitCode();
  delete m_process;
  m_process = nullptr;

  auto end = Time::now();
  auto fs = end - start;
  ms d = std::chrono::duration_cast<ms>(fs);
  std::stringstream stream;
  stream << "Duration: " << d.count() << " ms. Max utilization: ";
  if (max_utiliation <= 1024)
    stream << max_utiliation << " kiB";
  else
    stream << max_utiliation / 1024 << " MB";
  PERF_LOG(stream.str());
  return (status == QProcess::NormalExit) ? exitCode : -1;
}

std::string Compiler::ReplaceAll(std::string_view str, std::string_view from,
                                 std::string_view to) {
  size_t start_pos = 0;
  std::string result(str);
  while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
    result.replace(start_pos, from.length(), to);
    start_pos += to.length();  // Handles case where 'to' is a substr of 'from'
  }
  return result;
}
