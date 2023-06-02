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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <functional>
#include <sstream>
#include <thread>

#include "Compiler.h"
#include "Compiler/Constraints.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "CompilerDefines.h"
#include "Configuration/CFGCompiler/CFGCompiler.h"
#include "DesignQuery/DesignQuery.h"
#include "IPGenerate/IPCatalogBuilder.h"
#include "Log.h"
#include "Main/Settings.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "TaskManager.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "scope_guard/scope_guard.hpp"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
static const int CHATGPT_TIMEOUT{180000};

auto CreateDummyLog =
    [](ProjectManager* projManager,
       const std::string& outfileName) -> std::filesystem::path {
  std::filesystem::path outputPath{};

  if (projManager) {
    std::filesystem::path projectPath(projManager->projectPath());
    outputPath = projectPath / outfileName;
    if (FileUtils::FileExists(outputPath)) {
      std::filesystem::remove(outputPath);
    }
    std::ofstream ofs(outputPath);
    ofs << "Dummy log for " << outfileName << "\n";
    ofs.close();
  }

  return outputPath;
};

void Compiler::Version(std::ostream* out) {
  (*out) << "Foedag FPGA Compiler"
         << "\n";
  LogUtils::PrintVersion(out);
}

void Compiler::Help(ToolContext* context, std::ostream* out) {
  auto dataPath = context->DataPath();
  dataPath = dataPath / "etc" / "help.txt";
  std::ifstream stream(dataPath);
  if (stream.good()) {
    std::string helpContent((std::istreambuf_iterator<char>(stream)),
                            std::istreambuf_iterator<char>());
    (*out) << QtUtils::replaceTags(helpContent, helpTags());
  }
  stream.close();
}

void Compiler::CustomSimulatorSetup(Simulator::SimulationType action) {}

Compiler::Compiler(TclInterpreter* interp, std::ostream* out,
                   TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_out(out),
      m_tclInterpreterHandler(tclInterpreterHandler) {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
  SetConstraints(new Constraints{this});
  IPCatalog* catalog = new IPCatalog();
  m_IPGenerator = new IPGenerator(catalog, this);
  m_simulator = new Simulator(m_interp, this, m_out, m_tclInterpreterHandler);
  m_name = "dummy";
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
  delete m_simulator;
}

std::string Compiler::GetMessagePrefix() const {
  if (!GetTaskManager()) return std::string{};
  auto task = GetTaskManager()->currentTask();
  // Leave prefix empty if no abbreviation was given
  if (task && task->abbreviation() != "") {
    return task->abbreviation().toStdString() + ": ";
  }
  return std::string{};
}

void Compiler::Message(const std::string& message,
                       const std::string& messagePrefix, bool raw) const {
  if (m_out) {
    const std::string prefix =
        messagePrefix.empty() ? GetMessagePrefix() : messagePrefix;
    if (raw) {
      (*m_out) << prefix << message;
    } else {
      (*m_out) << "INFO: " << prefix << message << std::endl;
    }
  }
}

void Compiler::ErrorMessage(const std::string& message, bool append,
                            const std::string& messagePrefix, bool raw) const {
  if (m_err) {
    const std::string prefix =
        messagePrefix.empty() ? GetMessagePrefix() : messagePrefix;
    if (raw) {
      (*m_err) << prefix << message;
    } else {
      (*m_err) << "ERROR: " << prefix << message << std::endl;
    }
  }
  if (append) Tcl_AppendResult(m_interp->getInterp(), message.c_str(), nullptr);
}

std::vector<std::string> Compiler::GetCleanFiles(
    Action action, const std::string& projectName,
    const std::string& topModule) const {
  return {};
}

void Compiler::CleanFiles(Action action) {
  const std::filesystem::path base{ProjManager()->projectPath()};
  auto removeFiles = GetCleanFiles(action, ProjManager()->projectName(),
                                   ProjManager()->DesignTopModule());
  for (const auto& fileName : removeFiles) {
    const std::filesystem::path p{base / fileName};
    FileUtils::removeFile(p);
  }
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

bool Compiler::BuildLiteXIPCatalog(std::filesystem::path litexPath,
                                   bool namesOnly) {
  if (m_IPGenerator == nullptr) {
    IPCatalog* catalog = new IPCatalog();
    m_IPGenerator = new IPGenerator(catalog, this);
  }
  if (m_simulator == nullptr) {
    m_simulator = new Simulator(m_interp, this, m_out, m_tclInterpreterHandler);
  }
  IPCatalogBuilder builder(this);
  bool result = builder.buildLiteXCatalog(GetIPGenerator()->Catalog(),
                                          litexPath, namesOnly);
  return result;
}

// open_project and run_project tcl command implementation. As single
// boolean parameter (run) is the only difference and tcl lambdas can't get
// extra parameters or capture anything, static function had to be added.
static int openRunProjectImpl(void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[], bool run) {
  auto compiler = (Compiler*)clientData;
  auto cmdLine = compiler->GetSession()->CmdLine();
  if (argc != 2) {
    compiler->ErrorMessage("Specify a project file name");
    return TCL_ERROR;
  }
  std::string file = argv[1];
  std::string expandedFile = file;
  if (!FileUtils::FileExists(expandedFile)) {
    auto scriptFile = cmdLine->Script();
    std::filesystem::path script =
        scriptFile.empty() ? cmdLine->GuiTestScript() : scriptFile;
    std::filesystem::path scriptPath = script.parent_path();
    std::filesystem::path fullPath = scriptPath;
    fullPath.append(file);
    expandedFile = fullPath.string();
  }

  auto mainWindow = compiler->GetSession()->MainWindow();
  if (!mainWindow) {
    compiler->ErrorMessage(
        "GUI has to be started before calling 'open_project'");
    return TCL_ERROR;
  }
  auto mainWindowImpl = qobject_cast<FOEDAG::MainWindow*>(mainWindow);
  mainWindowImpl->openProject(QString::fromStdString(expandedFile), false, run);
  if (run) {
    // Wait till project run is finished
    while (mainWindowImpl->isRunning())
      QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }
  auto allTasks = compiler->GetTaskManager()->tasks();
  for (auto task : allTasks) {
    if (task && task->status() == TaskStatus::Fail) {
      compiler->ErrorMessage(task->title().toStdString() + "task failed");
      return TCL_ERROR;
    }
  }
  compiler->Message("Project run successful");
  return TCL_OK;
}

Simulator* Compiler::GetSimulator() {
  if (m_simulator == nullptr) {
    m_simulator = new Simulator(m_interp, this, m_out, m_tclInterpreterHandler);
  }
  return m_simulator;
}

bool Compiler::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  if (m_IPGenerator == nullptr) {
    IPCatalog* catalog = new IPCatalog();
    m_IPGenerator = new IPGenerator(catalog, this);
  }
  if (m_DesignQuery == nullptr) {
    m_DesignQuery = new DesignQuery(this);
  }
  if (m_simulator == nullptr) {
    m_simulator = new Simulator(m_interp, this, m_out, m_tclInterpreterHandler);
  }
  m_simulator->RegisterCommands(m_interp);
  m_IPGenerator->RegisterCommands(interp, batchMode);
  m_DesignQuery->RegisterCommands(interp, batchMode);
  if (m_constraints == nullptr) {
    SetConstraints(new Constraints{this});
  }
  if (m_configuration == nullptr) {
    SetConfiguration(new CFGCompiler(this));
    GetConfiguration()->RegisterCommands(interp, batchMode);
  }

  auto chatgpt = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    auto args = StringUtils::FromArgs(argc, argv);
    if (args.size() >= 2) {
      if (args.size() > 4) {
        if (args[3] == "-c") compiler->chatgptConfig(args[4]);
      }
      if (args[1] == "send") {
        WorkerThread* wthread =
            new WorkerThread(args[2], Action::IPGen, compiler);
        auto fn = [compiler](const std::string& str) -> bool {
          return compiler->chatGpt(str);
        };
        auto exitSt = wthread->Start(fn, args[2]);
        return exitSt ? TCL_OK : TCL_ERROR;
      } else if (args[1] == "reset") {
        WorkerThread* wthread =
            new WorkerThread("reset", Action::IPGen, compiler);
        auto fn = [compiler](const std::string&) -> bool {
          return compiler->chatGpt({});
        };
        auto exitSt = wthread->Start(fn, std::string{});
        return exitSt ? TCL_OK : TCL_ERROR;
      }
      compiler->ErrorMessage("Wrong arguments");
      return TCL_ERROR;
    }
    compiler->ErrorMessage("Wrong number of arguments");
    return TCL_ERROR;
  };
  interp->registerCmd("chatgpt", chatgpt, this, nullptr);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    compiler->Help(GlobalSession->Context(), compiler->GetOutStream());
    return TCL_OK;
  };
  interp->registerCmd("help", help, this, 0);

  auto script_path = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::filesystem::path script = compiler->GetSession()->CmdLine()->Script();
    std::filesystem::path scriptPath = script.parent_path();
    Tcl_SetResult(interp, (char*)scriptPath.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("script_path", script_path, this, 0);

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
    std::string type{"rtl"};
    if (argc >= 2) {
      name = argv[1];
    }
    if (argc > 3) {
      const std::string t = argv[2];
      if (t == "-type") type = argv[3];
    }
    compiler->GetOutput().clear();
    bool ok = compiler->CreateDesign(name, type);
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

  auto close_design = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler->HasInternalError()) return TCL_ERROR;
    compiler->m_tclCmdIntegration->TclCloseProject();
    return TCL_OK;
  };
  interp->registerCmd("close_design", close_design, this, nullptr);

  auto set_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc < 2) {
      compiler->ErrorMessage("Specify a top module name");
      return TCL_ERROR;
    }
    if (compiler->HasInternalError()) return TCL_ERROR;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::ostringstream out;
    bool ok = compiler->m_tclCmdIntegration->TclSetTopModule(argc, argv, out);
    if (!ok) {
      compiler->ErrorMessage(out.str());
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("set_top_module", set_top_module, this, 0);

  auto add_design_file = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (argc < 2) {
      compiler->ErrorMessage(
          "Incorrect syntax for add_design_file <file(s)> "
          "[-work libraryName]"
          "<type (-VHDL_1987, -VHDL_1993, -VHDL_2000, -VHDL_2008 (.vhd "
          "default), -V_1995, "
          "-V_2001 (.v default), -SV_2005, -SV_2009, -SV_2012, -SV_2017 (.sv "
          "default))>");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      const std::string type = argv[i];
      if (type == "-work") {
        if (i + 1 >= argc) {
          compiler->ErrorMessage(
              "Incorrect syntax for add_design_file <file(s)> "
              "Library name should follow '-work' tag");
          return TCL_ERROR;
        }
      }
    }
    if (compiler->ProjManager()->projectType() != RTL) {
      compiler->ErrorMessage(
          "Post synthesis flow. Please use read_netlist or change design "
          "type.");
      return TCL_ERROR;
    }
    return compiler->add_files(compiler, interp, argc, argv, Design);
  };
  interp->registerCmd("add_design_file", add_design_file, this, nullptr);

  auto add_simulation_file = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (argc < 2) {
      compiler->ErrorMessage(
          "Incorrect syntax for add_simulation_file <file(s)> "
          "[-work libraryName]"
          "<type (-VHDL_1987, -VHDL_1993, -VHDL_2000, -VHDL_2008 (.vhd "
          "default), -V_1995, "
          "-V_2001 (.v default), -SV_2005, -SV_2009, -SV_2012, -SV_2017 (.sv "
          "default), -C, -CPP)>");
      return TCL_ERROR;
    }
    for (int i = 1; i < argc; i++) {
      const std::string type = argv[i];
      if (type == "-work") {
        if (i + 1 >= argc) {
          compiler->ErrorMessage(
              "Incorrect syntax for add_simulation_file <file(s)> "
              "Library name should follow '-work' tag");
          return TCL_ERROR;
        }
      }
    }
    return compiler->add_files(compiler, interp, argc, argv, Simulation);
  };
  interp->registerCmd("add_simulation_file", add_simulation_file, this,
                      nullptr);

  auto clear_simulation_files = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler->HasInternalError()) return TCL_ERROR;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    std::ostringstream out;
    if (!compiler->m_tclCmdIntegration->TclClearSimulationFiles(out)) {
      compiler->ErrorMessage(out.str());
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("clear_simulation_files", clear_simulation_files, this,
                      nullptr);

  auto read_netlist = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler->HasInternalError()) return TCL_ERROR;
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc < 2) {
      compiler->ErrorMessage("Incorrect syntax for read_netlist <file>");
      return TCL_ERROR;
    }
    if (compiler->ProjManager()->projectType() != PostSynth) {
      compiler->ErrorMessage(
          "RTL Design flow. Please use add_design_file or change design type.");
      return TCL_ERROR;
    }

    const std::string file = argv[1];
    const std::string fileLowerCase = StringUtils::toLower(file);
    std::string actualType = "VERILOG";
    Design::Language language = Design::Language::VERILOG_NETLIST;
    if (strstr(fileLowerCase.c_str(), ".blif")) {
      language = Design::Language::BLIF;
      actualType = "BLIF";
    } else if (strstr(fileLowerCase.c_str(), ".eblif")) {
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
    if (!the_path.is_absolute()) {
      const auto& path = std::filesystem::current_path();
      expandedFile = std::filesystem::path(path / expandedFile).string();
    }

    compiler->Message("Reading " + actualType + " " + expandedFile);
    std::ostringstream out;
    bool ok = compiler->m_tclCmdIntegration->TclAddDesignFiles(
        {}, {}, expandedFile.c_str(), language, out);
    if (!ok) {
      compiler->ErrorMessage(out.str());
      return TCL_ERROR;
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
      if (FileUtils::FileExists(expandedFile) && (expandedFile != "./") &&
          (expandedFile != ".")) {
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
      if (FileUtils::FileExists(expandedFile) && (expandedFile != "./") &&
          (expandedFile != ".")) {
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
    if (compiler->HasInternalError()) return TCL_ERROR;
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
    compiler->Message("Adding constraint file " + expandedFile);
    std::ostringstream out;
    bool ok = compiler->m_tclCmdIntegration->TclAddConstrFiles(
        expandedFile.c_str(), out);
    if (!ok) {
      compiler->ErrorMessage(out.str());
      return TCL_ERROR;
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

    auto simulate = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      bool status = true;
      const auto default_sim_tool{Simulator::SimulatorType::Icarus};
      Simulator::SimulatorType sim_tool = default_sim_tool;
      if (argc < 2) {
        compiler->ErrorMessage(
            "Wrong number of arguments: simulate <type> ?<simulator>? "
            "?<waveform_file>?");
        return TCL_ERROR;
      }
      std::string sim_type;
      std::string wave_file;
      bool clean{false};
      bool sim_tool_valid{false};
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool ok{false};
        auto sim = Simulator::ToSimulatorType(arg, ok, default_sim_tool);
        if (ok) {
          sim_tool = sim;
          sim_tool_valid = true;
        } else if (arg == "rtl" || arg == "gate" || arg == "pnr" ||
                   arg == "bitstream_fd" || arg == "bitstream_bd") {
          sim_type = arg;
        } else if (arg == "clean") {
          clean = true;
        } else {
          wave_file = arg;
        }
      }
      if (!sim_tool_valid) {
        bool ok{false};
        auto level = Simulator::ToSimulationType(sim_type, ok);
        if (ok) {
          ok = false;
          auto simTool =
              compiler->GetSimulator()->UserSimulationType(level, ok);
          if (ok) sim_tool = simTool;
        }
      }
      compiler->SetWaveformFile(wave_file);
      if (clean)
        compiler->GetSimulator()->SimulationOption(
            Simulator::SimulationOpt::Clean);
      if (sim_type.empty()) {
        compiler->ErrorMessage("Unknown simulation type: " + sim_type);
        return TCL_ERROR;
      } else {
        if (sim_type == "rtl") {
          status = compiler->GetSimulator()->Simulate(
              Simulator::SimulationType::RTL, sim_tool, wave_file);
        } else if (sim_type == "gate") {
          status = compiler->GetSimulator()->Simulate(
              Simulator::SimulationType::Gate, sim_tool, wave_file);
        } else if (sim_type == "pnr") {
          status = compiler->GetSimulator()->Simulate(
              Simulator::SimulationType::PNR, sim_tool, wave_file);
        } else if (sim_type == "bitstream_fd") {
          status = compiler->GetSimulator()->Simulate(
              Simulator::SimulationType::BitstreamFrontDoor, sim_tool,
              wave_file);
        } else if (sim_type == "bitstream_bd") {
          status = compiler->GetSimulator()->Simulate(
              Simulator::SimulationType::BitstreamBackDoor, sim_tool,
              wave_file);
        }
      }
      return (status) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("simulate", simulate, this, 0);

    auto analyze = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->AnalyzeOpt(Compiler::DesignAnalysisOpt::Clean);
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
        } else if (arg == "clean") {
          compiler->SynthOpt(Compiler::SynthesisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown optimization option: " + arg);
        }
      }
      if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
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
      compiler->Message("Warning: Global placement is disabled");
      return TCL_OK;

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
        } else if (arg == "free") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::Free);
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
            "No Argument passed: type random/in_define_order/free");
        return TCL_ERROR;
      }
      if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
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
#ifndef PRODUCTION_BUILD
        } else if (arg == "opensta") {
          compiler->TimingAnalysisEngineOpt(Compiler::STAEngineOpt::Opensta);
#endif
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
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
          compiler->BitsOpt(Compiler::BitstreamOpt::Force);
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else if (arg == "enable_simulation") {
          compiler->BitsOpt(Compiler::BitstreamOpt::EnableSimulation);
        } else if (arg == "write_xml") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "write_fabric_independent") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "pb_pin_fixup") {
          compiler->BitstreamMoreOpt(arg);
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
        } else if (arg == "-modules") {
          compiler->IPGenOpt(Compiler::IPGenerateOpt::List);
          i++;
          if (i < argc) {
            compiler->IPGenMoreOpt(argv[i]);
          } else {
            compiler->ErrorMessage(
                "Incorrect syntax for ipgenerate -modules "
                "moduleName or {moduleName1 moduleName2} should follow "
                "'-modules' tag");
            return TCL_ERROR;
          }
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread =
          new WorkerThread("ip_th", Action::IPGen, compiler, generateReport);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("ipgenerate", ipgenerate, this, 0);

    auto analyze = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "clean") {
          compiler->AnalyzeOpt(Compiler::DesignAnalysisOpt::Clean);
        } else {
          compiler->ErrorMessage("Unknown analysis option: " + arg);
        }
      }
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread = new WorkerThread("analyze_th", Action::Analyze,
                                               compiler, generateReport);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("analyze", analyze, this, 0);

    auto simulate = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      bool status = true;
      if (argc < 2) {
        compiler->ErrorMessage(
            "Wrong number of arguments: simulate <type> ?<simulator>? "
            "?<waveform_file>?");
        return TCL_ERROR;
      }
      std::string sim_type;
      std::string wave_file;
      bool clean{false};
      bool sim_tool_valid{false};
      const auto default_sim_tool{Simulator::SimulatorType::Icarus};
      Simulator::SimulatorType sim_tool = default_sim_tool;
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool ok{false};
        auto sim = Simulator::ToSimulatorType(arg, ok, default_sim_tool);
        if (ok) {
          sim_tool = sim;
          sim_tool_valid = true;
        } else if (arg == "rtl" || arg == "gate" || arg == "pnr" ||
                   arg == "bitstream_fd" || arg == "bitstream_bd") {
          sim_type = arg;
        } else if (arg == "clean") {
          clean = true;
        } else {
          wave_file = arg;
        }
      }
      if (!sim_tool_valid) {
        bool ok{false};
        auto level = Simulator::ToSimulationType(sim_type, ok);
        if (ok) {
          ok = false;
          auto simTool =
              compiler->GetSimulator()->UserSimulationType(level, ok);
          if (ok) sim_tool = simTool;
        }
      }
      compiler->SetWaveformFile(wave_file);
      compiler->GetSimulator()->SetSimulatorType(sim_tool);
      if (clean)
        compiler->GetSimulator()->SimulationOption(
            Simulator::SimulationOpt::Clean);
      if (sim_type.empty()) {
        compiler->ErrorMessage("Unknown simulation type: " + sim_type);
        return TCL_ERROR;
      } else {
        if (sim_type == "rtl") {
          WorkerThread* wthread = new WorkerThread(
              "simulate_rtl_th", Action::SimulateRTL, compiler);
          status = wthread->start();
          if (!status) return TCL_ERROR;
        } else if (sim_type == "gate") {
          WorkerThread* wthread = new WorkerThread(
              "simulate_rtl_th", Action::SimulateGate, compiler);
          status = wthread->start();
          if (!status) return TCL_ERROR;
        } else if (sim_type == "pnr") {
          WorkerThread* wthread = new WorkerThread(
              "simulate_rtl_th", Action::SimulatePNR, compiler);
          status = wthread->start();
          if (!status) return TCL_ERROR;
        } else if (sim_type == "bitstream_fd") {
          WorkerThread* wthread = new WorkerThread(
              "simulate_rtl_th", Action::SimulateBitstream, compiler);
          status = wthread->start();
          if (!status) return TCL_ERROR;
        } else if (sim_type == "bitstream_bd") {
          WorkerThread* wthread = new WorkerThread(
              "simulate_rtl_th", Action::SimulateBitstream, compiler);
          status = wthread->start();
          if (!status) return TCL_ERROR;
        }
      }
      return (status) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("simulate", simulate, this, 0);

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
      if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread = new WorkerThread("synth_th", Action::Synthesis,
                                               compiler, generateReport);
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
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread =
          new WorkerThread("pack_th", Action::Pack, compiler, generateReport);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("packing", packing, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      compiler->Message("Warning: Global placement is disabled");
      return TCL_OK;

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
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread = new WorkerThread("place_th", Action::Detailed,
                                               compiler, generateReport);
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
        } else if (arg == "free") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::Free);
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
            "No Argument passed: type random/in_define_order/free");
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
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread = new WorkerThread("route_th", Action::Routing,
                                               compiler, generateReport);
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
#ifndef PRODUCTION_BUILD
        } else if (arg == "opensta") {
          compiler->TimingAnalysisEngineOpt(Compiler::STAEngineOpt::Opensta);
#endif
        } else {
          compiler->ErrorMessage("Unknown option: " + arg);
        }
      }
      if (compiler->GuiTclSync()) compiler->GuiTclSync()->saveSettings();
      std::function<void(int)> generateReport =
          std::bind(&Compiler::GenerateReport, compiler, std::placeholders::_1);
      WorkerThread* wthread =
          new WorkerThread("sta_th", Action::STA, compiler, generateReport);
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
          compiler->BitsOpt(Compiler::BitstreamOpt::Force);
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else if (arg == "enable_simulation") {
          compiler->BitsOpt(Compiler::BitstreamOpt::EnableSimulation);
        } else if (arg == "write_xml") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "write_fabric_independent") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "pb_pin_fixup") {
          compiler->BitstreamMoreOpt(arg);
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

  auto open_project = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    return openRunProjectImpl(clientData, interp, argc, argv, false);
  };

  auto run_project = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    return openRunProjectImpl(clientData, interp, argc, argv, true);
  };
  interp->registerCmd("open_project", open_project, this, nullptr);
  interp->registerCmd("run_project", run_project, this, nullptr);

  auto wave_cmd = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      // Convert args to a single string
      std::vector<std::string> args{};
      for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
      }
      std::string cmd = StringUtils::join(args, " ") + "\n";

      // Send cmd to GTKWave
      compiler->GTKWaveSendCmd(cmd);
    }
    return TCL_OK;
  };
  interp->registerCmd("wave_cmd", wave_cmd, this, nullptr);

  auto wave_get_return = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      auto wave = compiler->GetGTKWaveProcess();
      QString result{};
      // ReturnVal gets set in the readyReadStandardOutput handler set in
      // GetGTKWaveProcess()
      auto retVal = wave->property("ReturnVal");
      if (retVal.isValid()) {
        result = retVal.toString();
      }
      Tcl_AppendResult(interp, qPrintable(result), nullptr);
    }
    return TCL_OK;
  };
  interp->registerCmd("wave_get_return", wave_get_return, this, nullptr);

  auto wave_open = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      std::string file{};
      // check if a file was requested
      if (argc > 1) {
        file = argv[1];
        if (file.size() > 0 && file[0] == '~') {
          // ~/ doesn't get substitued so we'll manually turn ~ paths into
          // absolute paths
          std::string separator(1, std::filesystem::path::preferred_separator);
          auto pos = file.find_first_of(separator, 0);
          if (pos != std::string::npos) {
            // this covers an uncommon scenario where a user provides a homepath
            // like ~user/file.txt as well as the standard ~/
            file.erase(0, pos);
          }
          // Manally add separator now that we've stripped the home/~ portions
          file = QDir::homePath().toStdString() + separator + file;
        }
      }

      // GTKWave sets its current directory to its bin dir for dependency
      // loading. As such, relative user paths might not work when passed from
      // the ui which invokes from its own bin dir so we'll convert to fullpath
      auto path = FileUtils::GetFullPath(std::filesystem::path(file));

      // if a file was passed, set the loadFile command
      std::string cmd{};
      if (!file.empty()) {
        if (FileUtils::FileExists(path)) {
          cmd = "gtkwave::loadFile " + path.string();
        } else {
          Tcl_AppendResult(interp, "Error: File doesn't exist", nullptr);
          return TCL_ERROR;
        }
      }

      // Send cmd to GTKWave
      compiler->GTKWaveSendCmd(cmd);
    }
    return TCL_OK;
  };
  interp->registerCmd("wave_open", wave_open, this, nullptr);

  auto wave_time = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      if (argc != 2) {
        Tcl_AppendResult(
            interp,
            qPrintable("Expected Syntax: wave_time <timeString>\ntimeString "
                       "can specify its time units. Ex: 1ps, 1000ns, 1s"),
            nullptr);
        return TCL_ERROR;
      }

      std::string cmd = std::string("gtkwave::setMarker ") + argv[1];

      // Send cmd to GTKWave
      compiler->GTKWaveSendCmd(cmd);
    }
    return TCL_OK;
  };
  interp->registerCmd("wave_time", wave_time, this, nullptr);

  auto wave_show = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      if (argc != 2) {
        Tcl_AppendResult(interp,
                         qPrintable("Expected Syntax: wave_show <signalName>"),
                         nullptr);
        return TCL_ERROR;
      }
      std::string cmd = "foedag::show_signal " + std::string(argv[1]);
      compiler->GTKWaveSendCmd(cmd);
    }

    return TCL_OK;
  };
  interp->registerCmd("wave_show", wave_show, this, nullptr);

  auto wave_refresh = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler) {
      compiler->GTKWaveSendCmd("gtkwave::reLoadFile");
    }
    return TCL_OK;
  };
  interp->registerCmd("wave_refresh", wave_refresh, this, nullptr);

  auto diagnostic = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    for (int i = 1; i < argc; i++) {
      const std::string arg{argv[i]};
      if (arg == "packer") {
        compiler->PackOpt(Compiler::PackingOpt::Debug);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("diagnostic", diagnostic, this, nullptr);

  return true;
}

// This will send a given command to the gtkwave wish interface over stdin
void Compiler::GTKWaveSendCmd(const std::string& gtkWaveCmd,
                              bool raiseGtkWindow /* true */) {
  QProcess* process = GetGTKWaveProcess();

  std::string cmd = gtkWaveCmd;
  if (process && process->isWritable()) {
    if (cmd.back() != '\n') {
      // Add a newline to end of command if one is missing
      // (GTKWave will hang if you don't send a newline with your command)
      cmd += "\n";
    }

    if (raiseGtkWindow) {
      // Add a window raise command before the command to try to bring gtkwave
      // to focus
      cmd = "gtkwave::presentWindow\n" + cmd;
    }

    // Send command to GTKWave's stdin
    process->write(cmd.c_str());
    process->waitForBytesWritten();
  }
}

void Compiler::PinmapCSVFile(const std::filesystem::path& path) {
  m_PinMapCSV = path;
}

const std::filesystem::path& Compiler::PinmapCSVFile() const {
  return m_PinMapCSV;
}

// This will return a pointer to the current gtkwave process, if no process is
// running, one will be started
QProcess* Compiler::GetGTKWaveProcess() {
  if (!m_gtkwave_process) {
    m_gtkwave_process = new QProcess();

    // Get a path to our local gtkwave binary
    auto binPath = GlobalSession->Context()->BinaryPath();
    auto exePath = binPath / "gtkwave" / "bin";
    QString wd = QString::fromStdString(exePath.string());
    // Need to be in the gtkwave bin dir to resolve some dependency issues
    m_gtkwave_process->setWorkingDirectory(wd);

    // Enable process control via tcl interface over std in with --wish
    QString wishArg = "--wish";
    QStringList args{wishArg};

    auto cleanMessage = [](const QString& msg) {
      // GTKWave seems to prefix every error message with its version along with
      // way too many empty lines so we'll clear both of those out
      QStringList validLines{};
      for (auto line : msg.split('\n')) {
        if (line.isEmpty() || line.startsWith("GTKWave Analyzer v")) {
          // ignore pointless lines
          continue;
        } else if (line.startsWith("WM Destroy")) {
          // Make the WM Destory message more user friendly
          validLines << "window has been closed";
          continue;
        }

        validLines << line;
      }

      return validLines.join('\n');
    };

    auto handleStdout = [this, cleanMessage]() {
      // If we want to listen for return values from gtkwave
      // getters, this is where we should check the value.

      // Possible way of returning getter results:
      // `wave_cmd puts \[gtkwave::getWaveWidth\]`
      // also see installGTKWaveHelpers() for fine control

      // Read stdout data
      QByteArray data = m_gtkwave_process->readAllStandardOutput();
      QString trimmed = data.trimmed();

      // Listen for the wish interface being opened
      if (trimmed.startsWith("Interpreter id is")) {
        // Install extra tcl helpers
        installGTKWaveHelpers();
      }

      // If the user had gtkwave print _RETURN_, capture and
      // store the rest of the output, this can be retrieved
      // with wave_get_return
      QString retStr = "_RETURN_";
      if (trimmed.startsWith(retStr)) {
        trimmed.remove(0, retStr.size());
        m_gtkwave_process->setProperty("ReturnVal", trimmed);
      }

      // Print message
      QString msg = cleanMessage(trimmed);
      if (!msg.isEmpty()) {
        Message("GTKWave - " + msg.toStdString());
        finish();  // this is required to update the console to show a new
                   // prompt for the user
      }
    };

    QRegularExpression fileLoadedRegex =
        QRegularExpression("\\[\\d*\\] start time.\\n\\[\\d*\\] end time.");
    auto handleStderr = [this, cleanMessage, fileLoadedRegex]() {
      // Read stderr data
      QByteArray data = m_gtkwave_process->readAllStandardError();

      // Print message
      QString msg = cleanMessage(data.trimmed());
      if (!msg.isEmpty()) {
        // GTKWave uses stderr for some normal messages like the wave times when
        // loading a file so we have to convert those to normal status messages
        auto match = fileLoadedRegex.match(msg);
        if (match.hasMatch()) {
          // Print as normal status message instead
          Message("GTKWave - " + msg.toStdString());
        } else {
          ErrorMessage("GTKWave - " + msg.toStdString());
        }

        finish();  // this is required to update the console to show a new
                   // prompt for the user
      }
    };

    // Start GTKWave Process.
    // Invoking ./gtkwave to ensure we load the local copy
    m_gtkwave_process->start("./gtkwave", args);
    if (m_gtkwave_process->waitForStarted()) {
      // Clear pointer on process close
      QObject::connect(
          m_gtkwave_process,
          // static_cast required due to overload of signal params
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
          [this](int exitCode, QProcess::ExitStatus exitStauts) {
            if (m_gtkwave_process) {
              m_gtkwave_process->deleteLater();
              m_gtkwave_process = nullptr;
            }
          });

      // Listen to stdout
      QObject::connect(m_gtkwave_process, &QProcess::readyReadStandardOutput,
                       handleStdout);

      // Listen to stderr
      QObject::connect(m_gtkwave_process, &QProcess::readyReadStandardError,
                       handleStderr);
    }
  }
  return m_gtkwave_process;
}

// This will install helper functions that build upon existing gtkwave::
// commands to provide the functionality we need
void Compiler::installGTKWaveHelpers() {
  std::string cmds = R"(
    namespace eval foedag {}

    proc foedag::unselect_all {} {
      set signals [gtkwave::getDisplayedSignals]
      gtkwave::unhighlightSignalsFromList $signals
    }

    proc foedag::add_signal_once {signal} {
      # This will check the existing signals for a dupe before adding the signal
      set signals [gtkwave::getDisplayedSignals]
      if { [lsearch $signals $signal] < 0 } {
        # signal doesn't exist, add it
        gtkwave::addSignalsFromList $signal
      }
    }

    proc foedag::show_signal { signal } {
      # This is a helper function to add and highlight a signal.
      # It will only add the signal if it doesn't exist so duplicates are avoided
      # It will unhighlight all signals before selecting the given signal
      foedag::add_signal_once $signal

      # unhighlight all signals
      foedag::unselect_all

      # highglight signal
      gtkwave::highlightSignalsFromList $signal
    }

  )";

  GTKWaveSendCmd(cmds);
}

// Helper function to print help entries in a uniform fashion
void Compiler::writeHelp(
    std::ostream* out,
    const std::vector<std::pair<std::string, std::string>>& cmdDescPairs,
    int frontSpacePadCount, int descColumn) {
  // Create front padding
  std::string prePad = std::string(frontSpacePadCount, ' ');

  for (auto [cmd, desc] : cmdDescPairs) {
    // Create padding to try to line up description text at descColumn
    int postPadCount = descColumn - prePad.length() - cmd.length();
    std::string postPad = std::string((std::max)(0, postPadCount), ' ');

    // Print help entry line with padding
    (*out) << prePad << cmd << postPad << ": " << desc << std::endl;
  }
}

void Compiler::writeWaveHelp(std::ostream* out, int frontSpacePadCount,
                             int descColumn) {
  std::vector<std::pair<std::string, std::string>> helpEntries = {
      {"wave_*",
       "All wave commands will launch a GTKWave process if one hasn't been "
       "launched already. Subsequent commands will be sent to the launched "
       "process."},
      {"wave_cmd ...",
       "Sends given tcl commands to GTKWave process. See GTKWave docs for "
       "gtkwave:: commands."},
      {"wave_open <filename>", "Load given file in current GTKWave process."},
      {"wave_refresh", "Reloads the current active wave file"},
      {"wave_show <signal>",
       "Add the given signal to the GTKWave window and highlight it."},
      {"wave_time <time>",
       "Set the primary marker to <time>. Time units can be specified, without "
       "a space. Ex: wave_time 100ps."}};

  writeHelp(out, helpEntries, frontSpacePadCount, descColumn);
}

// Search the project directory for files ending in .rpt and add our header if
// the file doesn't have it already
void Compiler::AddHeadersToLogs() {
  auto projManager = ProjManager();
  if (projManager) {
    std::filesystem::path projectPath(projManager->projectPath());
    LogUtils::AddHeadersToLogs(projectPath);
  }
}

void Compiler::AddErrorLink(const Task* const current) {
  if (!current) return;
  auto logFile = current->logFileReadPath();
  if (logFile.isEmpty()) return;
  logFile.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
  static const QRegularExpression ERROR_REGEXP("Error [0-9].*:");
  QFile file{logFile};
  if (!file.open(QFile::ReadOnly)) return;

  uint lineNumber{0};
  while (!file.atEnd()) {
    auto line = file.readLine();
    auto match = ERROR_REGEXP.match(line);
    if (match.hasMatch()) {
      ErrorMessage(QString{"file \"%1\" line %2"}
                       .arg(logFile, QString::number(lineNumber + 1))
                       .toStdString(),
                   false);
      break;
    }
    lineNumber++;
  }
}

bool Compiler::HasInternalError() const {
  if (!m_tclCmdIntegration) {
    ErrorMessage("TCL command integration did not initialize");
    return true;
  }
  return false;
}

DeviceData Compiler::deviceData() const { return m_deviceData; }

void Compiler::setDeviceData(const DeviceData& newDeviceData) {
  m_deviceData = newDeviceData;
}

void Compiler::ClbPackingOption(ClbPacking clbPacking) {
  m_clbPacking = clbPacking;
}

ClbPacking Compiler::ClbPackingOption() const { return m_clbPacking; }

bool Compiler::Compile(Action action) {
  uint task{toTaskId(static_cast<int>(action), this)};
  if (m_stop) {
    ResetStopFlag();
    if (task != TaskManager::invalid_id && m_taskManager) {
      m_taskManager->task(task)->setStatus(TaskStatus::Fail);
    }
    return false;
  }
  ResetStopFlag();
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

void Compiler::GenerateReport(int action) {
  handleJsonReportGeneration(m_taskManager->task(toTaskId(action, this)),
                             m_taskManager, m_projManager->getProjectPath());
}

void Compiler::Stop() {
  m_stop = true;
  ErrorMessage("Interrupted by user");
  if (m_process) m_process->terminate();
  FileUtils::terminateSystemCommand();
}

void Compiler::ResetStopFlag() { m_stop = false; }

bool Compiler::Analyze() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  if (AnalyzeOpt() == DesignAnalysisOpt::Clean) {
    Message("Cleaning analysis results for " + m_projManager->projectName());
    m_state = State::IPGenerated;
    AnalyzeOpt(DesignAnalysisOpt::None);
    return true;
  }
  Message("Analyzing design: " + m_projManager->projectName());

  auto currentPath = std::filesystem::current_path();
  auto it = std::filesystem::directory_iterator{currentPath};
  for (int i = 0; i < 100; i = i + 10) {
    std::stringstream outStr;
    outStr << std::setw(2) << i << "%";
    if (it != std::filesystem::end(it)) {
      std::string str =
          " File: " + (*it).path().filename().string() + " just for test";
      outStr << str;
      it++;
    }
    Message(outStr.str());
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Analyzed;
  Message(("Design ") + m_projManager->projectName() + " is analyzed");

  CreateDummyLog(m_projManager, ANALYSIS_LOG);
  return true;
}

bool Compiler::Synthesize() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + m_projManager->projectName());
    m_state = State::IPGenerated;
    SynthOpt(SYNTH_OPT_DEFAULT);
    return true;
  }
  Message("Synthesizing design: " + m_projManager->projectName());
  for (auto constraint : m_constraints->getConstraints()) {
    Message("Constraint: " + constraint);
  }
  for (auto keep : m_constraints->GetKeeps()) {
    Message("Keep name: " + keep);
  }
  auto currentPath = std::filesystem::current_path();
  auto it = std::filesystem::directory_iterator{currentPath};
  for (int i = 0; i < 100; i = i + 10) {
    std::stringstream outStr;
    outStr << std::setw(2) << i << "%";
    if (it != std::filesystem::end(it)) {
      std::string str =
          " File: " + (*it).path().filename().string() + " just for test";
      outStr << str;
      it++;
    }
    Message(outStr.str());
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Synthesized;
  Message("Design " + m_projManager->projectName() + " is synthesized");

  CreateDummyLog(m_projManager, SYNTHESIS_LOG);
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
  Message("Global Placement for design:" + m_projManager->projectName());
  for (int i = 0; i < 100; i = i + 10) {
    std::stringstream outStr;
    outStr << std::setw(2) << i << "%";
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);
    Message(outStr.str());
    if (m_stop) return false;
  }
  m_state = State::GloballyPlaced;
  Message("Design " + m_projManager->projectName() + " globally placed");

  CreateDummyLog(m_projManager, GLOBAL_PLACEMENT_LOG);
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
  auto currentTask = m_taskManager->currentTask();
  // Use Scope Guard to add headers to new logs whenever this function exits
  auto guard = sg::make_scope_guard([this, currentTask] {
    AddHeadersToLogs();
    AddErrorLink(currentTask);
  });

  switch (action) {
    case Action::IPGen:
      return IPGenerate();
    case Action::Analyze:
      return Analyze();
    case Action::Synthesis:
      return Synthesize();
    case Action::Pack:
      return Packing();
    // case Action::Global:
    //   return GlobalPlacement();
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
    case Action::SimulateRTL:
      return GetSimulator()->Simulate(Simulator::SimulationType::RTL,
                                      GetSimulator()->GetSimulatorType(),
                                      m_waveformFile);
    case Action::SimulateGate:
      return GetSimulator()->Simulate(Simulator::SimulationType::Gate,
                                      GetSimulator()->GetSimulatorType(),
                                      m_waveformFile);
    case Action::SimulatePNR:
      return GetSimulator()->Simulate(Simulator::SimulationType::PNR,
                                      GetSimulator()->GetSimulatorType(),
                                      m_waveformFile);
    case Action::SimulateBitstream:
      return GetSimulator()->Simulate(
          Simulator::SimulationType::BitstreamBackDoor,
          GetSimulator()->GetSimulatorType(), m_waveformFile);
    case Action::Configuration:
      return GetConfiguration()->Configure();
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
    m_taskManager->bindTaskCommand(ANALYSIS, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("analyze"));
    });
    m_taskManager->bindTaskCommand(ANALYSIS_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("analyze clean"));
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
    // m_taskManager->bindTaskCommand(GLOBAL_PLACEMENT, []() {
    //   GlobalSession->CmdStack()->push_and_exec(new Command("globp"));
    // });
    // m_taskManager->bindTaskCommand(GLOBAL_PLACEMENT_CLEAN, []() {
    //   GlobalSession->CmdStack()->push_and_exec(new Command("globp clean"));
    // });
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
    m_taskManager->bindTaskCommand(TIMING_SIGN_OFF_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("sta clean"));
    });
    m_taskManager->bindTaskCommand(POWER, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("power"));
    });
    m_taskManager->bindTaskCommand(POWER_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("power clean"));
    });
    m_taskManager->bindTaskCommand(BITSTREAM, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("bitstream"));
    });
    m_taskManager->bindTaskCommand(BITSTREAM_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("bitstream clean"));
    });
    m_taskManager->bindTaskCommand(PLACE_AND_ROUTE_VIEW, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("sta view"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_RTL, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("simulate rtl"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_RTL_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(
          new Command("simulate rtl clean"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_GATE, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("simulate gate"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_GATE_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(
          new Command("simulate gate clean"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_PNR, []() {
      GlobalSession->CmdStack()->push_and_exec(new Command("simulate pnr"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_PNR_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(
          new Command("simulate pnr clean"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_BITSTREAM, []() {
      GlobalSession->CmdStack()->push_and_exec(
          new Command("simulate bitstream"));
    });
    m_taskManager->bindTaskCommand(SIMULATE_BITSTREAM_CLEAN, []() {
      GlobalSession->CmdStack()->push_and_exec(
          new Command("simulate bitstream clean"));
    });
    m_taskManager->task(SIMULATE_RTL)
        ->setCustomData({CustomDataType::Sim,
                         QVariant::fromValue(Simulator::SimulationType::RTL)});
    m_taskManager->task(SIMULATE_PNR)
        ->setCustomData({CustomDataType::Sim,
                         QVariant::fromValue(Simulator::SimulationType::PNR)});
    m_taskManager->task(SIMULATE_GATE)
        ->setCustomData({CustomDataType::Sim,
                         QVariant::fromValue(Simulator::SimulationType::Gate)});
    m_taskManager->task(SIMULATE_BITSTREAM)
        ->setCustomData({CustomDataType::Sim,
                         QVariant::fromValue(
                             Simulator::SimulationType::BitstreamBackDoor)});
  }
}

TaskManager* Compiler::GetTaskManager() const { return m_taskManager; }

void Compiler::setGuiTclSync(TclCommandIntegration* tclCommands) {
  m_tclCmdIntegration = tclCommands;
  if (m_tclCmdIntegration)
    m_projManager = m_tclCmdIntegration->GetProjectManager();
}

std::vector<std::string> Compiler::helpTags() const { return {"foedag"}; }

TclCommandIntegration* Compiler::GuiTclSync() const {
  return m_tclCmdIntegration;
}

bool Compiler::IPGenerate() {
  if (!m_projManager->HasDesign() && !CreateDesign("noname")) return false;
  if (!HasIPInstances()) {
    // No instances configured, no-op w/o error
    return true;
  }
  Message("IP generation for design: " + m_projManager->projectName());
  bool status = GetIPGenerator()->Generate();
  if (status) {
    Message("Design " + m_projManager->projectName() + " IPs are generated");
    m_state = State::IPGenerated;
  } else {
    ErrorMessage("Design " + m_projManager->projectName() +
                 " IPs generation failed");
  }

  CreateDummyLog(m_projManager, IP_GENERATE_LOG);
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
  Message("Packing for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is packed");
  m_state = State::Packed;

  CreateDummyLog(m_projManager, PACKING_LOG);
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
  Message("Placement for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is placed");
  m_state = State::Placed;

  CreateDummyLog(m_projManager, PLACEMENT_LOG);
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

  Message("Routing for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is routed");
  m_state = State::Routed;

  CreateDummyLog(m_projManager, ROUTING_LOG);
  return true;
}

bool Compiler::TimingAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Timing analysis for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is analyzed");
  CreateDummyLog(m_projManager, TIMING_ANALYSIS_LOG);
  return true;
}

bool Compiler::PowerAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Power analysis for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is analyzed");

  CreateDummyLog(m_projManager, POWER_ANALYSIS_LOG);
  return true;
}

bool Compiler::GenerateBitstream() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Bitstream generation for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " bitstream is generated");

  CreateDummyLog(m_projManager, BITSTREAM_LOG);
  return true;
}

bool Compiler::chatGpt(const std::string& message) {
  emit m_tclCmdIntegration->chatGptStatus(true);
  bool result{true};
  if (message.empty()) {
    result = resetChatGpt();
  } else {
    result = sendChatGpt(message);
  }
  if (!result) emit m_tclCmdIntegration->chatGptStatus(false);
  return result;
}

bool Compiler::sendChatGpt(const std::string& message) {
  auto path = GlobalSession->Context()->DataPath();
  path = path / ".." / "envs" / "chatGPT" / "bin";
  path = path / "python";
  std::filesystem::path pythonPath{path};
  if (pythonPath.empty()) {
    ErrorMessage(
        "Unable to find python interpreter in local "
        "environment.\n");
    return false;
  }

  const std::string file{"chatgpt"};

  std::string command = pythonPath.string();
  std::vector<std::string> args;
  args.push_back("-m");
  args.push_back("chatgpt_raptor");
  args.push_back("-o");
  args.push_back(file);
  args.push_back("-p");
  args.push_back("\'" + message + "\'");
  if (!m_chatgptConfigFile.empty()) {
    args.push_back("-c");
    args.push_back(m_chatgptConfigFile);
  }
  std::ostringstream help;

  if (FileUtils::ExecuteSystemCommand(pythonPath.string(), args, &help,
                                      CHATGPT_TIMEOUT)
          .code != 0) {
    ErrorMessage("ChatGPT, " + help.str(), false);
    return false;
  }

  std::ifstream stream{file};
  if (!stream.good()) {
    ErrorMessage("Can't open file: " + file);
    return false;
  }
  std::stringstream buffer;
  buffer << stream.rdbuf();
  const std::string& buf = buffer.str();
  stream.close();

  json json{};
  try {
    json.update(json::parse(buf));
  } catch (json::parse_error& e) {
    // output exception information
    std::cerr << "Json Error: " << e.what() << std::endl;
    return false;
  }

  std::string responce = json["message"];

  // read content here from json
  m_tclCmdIntegration->TclshowChatGpt(message, responce);

  return true;
}

bool Compiler::resetChatGpt() {
  auto path = GlobalSession->Context()->DataPath();
  path = path / ".." / "envs" / "chatGPT" / "bin";
  path = path / "python";
  std::filesystem::path pythonPath{path};
  if (pythonPath.empty()) {
    ErrorMessage(
        "Unable to find python interpreter in local "
        "environment.\n");
    return false;
  }

  std::string command = pythonPath.string();
  std::vector<std::string> args;
  args.push_back("-m");
  args.push_back("chatgpt_raptor");
  args.push_back("-n");
  if (!m_chatgptConfigFile.empty()) {
    args.push_back("-c");
    args.push_back(m_chatgptConfigFile);
  }
  std::ostringstream help;

  if (FileUtils::ExecuteSystemCommand(pythonPath.string(), args, &help,
                                      CHATGPT_TIMEOUT)
          .code != 0) {
    ErrorMessage("ChatGPT, " + help.str(), false);
    return false;
  }

  m_tclCmdIntegration->TclshowChatGpt({}, {});
  return true;
}

void Compiler::chatgptConfig(const std::string& file) {
  m_chatgptConfigFile = file;
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

bool Compiler::HasIPInstances() {
  bool result = false;
  auto ipGen = GetIPGenerator();
  if (ipGen) {
    result = !ipGen->IPInstances().empty();
  }
  return result;
}

bool Compiler::HasIPDefinitions() {
  bool result = false;
  auto ipGen = GetIPGenerator();
  if (ipGen && ipGen->Catalog()) {
    result = !ipGen->Catalog()->Definitions().empty();
  }
  return result;
}

bool Compiler::CreateDesign(const std::string& name, const std::string& type) {
  if (m_tclCmdIntegration) {
    if (m_projManager->HasDesign()) {
      ErrorMessage("Design already exists");
      return false;
    }

    std::ostringstream out;
    bool ok = m_tclCmdIntegration->TclCreateProject(
        QString::fromStdString(name), QString::fromStdString(type), out);
    if (!out.str().empty()) m_output = out.str();
    if (!ok) return false;
    std::string message{"Created design: " + name};
    if (!type.empty()) message += ". Project type: " + type;
    Message(message);
  }
  return true;
}

const std::string Compiler::GetNetlistPath() {
  std::string netlistFile =
      (std::filesystem::path(ProjManager()->projectPath()) /
       (ProjManager()->projectName() + "_post_synth.blif"))
          .string();

  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
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

void Compiler::SetConstraints(Constraints* c) {
  m_constraints = c;
  if (m_interp) m_constraints->registerCommands(m_interp);
}

void Compiler::SetEnvironmentVariable(const std::string variable,
                                      const std::string value) {
  m_environmentVariableMap.emplace(variable, value);
}

int Compiler::ExecuteAndMonitorSystemCommand(const std::string& command,
                                             const std::string logFile,
                                             bool appendLog) {
  auto start = Time::now();
  PERF_LOG("Command: " + command);
  (*m_out) << "Command: " << command << std::endl;
  std::error_code ec;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(m_projManager->projectPath(),
                                ec);  // setting path
  // new QProcess must be created here to avoid issues related to creating
  // QObjects in different threads
  m_process = new QProcess;
  QStringList env = QProcess::systemEnvironment();
  if (!m_environmentVariableMap.empty()) {
    for (std::map<std::string, std::string>::iterator itr =
             m_environmentVariableMap.begin();
         itr != m_environmentVariableMap.end(); itr++) {
      env << strdup(((*itr).first + "=" + (*itr).second).c_str());
    }
  }
  m_process->setEnvironment(env);
  std::ofstream ofs;
  if (!logFile.empty()) {
    std::ios_base::openmode openMode{std::ios_base::out};
    if (appendLog) openMode = std::ios_base::out | std::ios_base::app;
    ofs.open(logFile, openMode);
    QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                     [this, &ofs]() {
                       qint64 bytes = m_process->bytesAvailable();
                       QByteArray bufout = m_process->readAllStandardOutput();
                       ofs.write(bufout, bytes);
                       m_out->write(bufout, bytes);
                     });
    QObject::connect(m_process, &QProcess::readyReadStandardError,
                     [this, &ofs]() {
                       QByteArray data = m_process->readAllStandardError();
                       int bytes = data.size();
                       ofs.write(data, bytes);
                       m_err->write(data, bytes);
                     });
  } else {
    QObject::connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
      m_out->write(m_process->readAllStandardOutput(),
                   m_process->bytesAvailable());
    });
    QObject::connect(m_process, &QProcess::readyReadStandardError, [this]() {
      QByteArray data = m_process->readAllStandardError();
      m_err->write(data, data.size());
    });
  }
  ProcessUtils utils;
  QObject::connect(m_process, &QProcess::started,
                   [&utils, this]() { utils.Start(m_process->processId()); });

  QString cmd{command.c_str()};
  QStringList args = QtUtils::StringSplit(cmd, ' ');
  QStringList adjustedArgs;

  QString program = args.first();
  args.pop_front();  // remove program
  QString current_arg;
  for (int i = 0; i < args.size(); i++) {
    QString arg = args[i];
    if (args[i].front() == '\"' &&
        args[i].back() != '\"') {      // Starting single-quote
      current_arg = arg.remove(0, 1);  // remove leading quote
    } else if (args[i].front() != '\"' &&
               args[i].back() == '\"') {    // Ending single-quote
      current_arg += " " + arg.chopped(1);  // remove trailing quote
      adjustedArgs.push_back(current_arg);
      current_arg = "";
    } else if (args[i].front() == '\"' &&
               args[i].back() == '\"') {  // Single-quoted argument
      current_arg += " " + arg;
      adjustedArgs.push_back(current_arg);
      current_arg = "";
    } else if (!current_arg.isEmpty()) {  // Continuing single-quoted argument
      current_arg += " " + arg;
    } else {  // Non-quoted argument
      adjustedArgs.push_back(arg);
    }
  }

  m_process->start(program, adjustedArgs);
  std::filesystem::current_path(path);
  m_process->waitForFinished(-1);
  utils.Stop();
  // DEBUG: (*m_out) << "Changed path to: " << (path).string() << std::endl;
  uint max_utiliation{utils.Utilization()};
  auto status = m_process->exitStatus();
  auto exitCode = m_process->exitCode();
  delete m_process;
  m_process = nullptr;
  if (!logFile.empty()) {
    ofs.close();
  }
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

std::pair<bool, std::string> Compiler::IsDeviceSizeCorrect(
    const std::string& size) const {
  return std::make_pair(true, std::string{});
}

int Compiler::add_files(Compiler* compiler, Tcl_Interp* interp, int argc,
                        const char* argv[], AddFilesType filesType) {
  if (compiler->HasInternalError()) return TCL_ERROR;
  if (!compiler->ProjManager()->HasDesign()) {
    compiler->ErrorMessage("Create a design first: create_design <name>");
    return TCL_ERROR;
  }
  std::string actualType;
  Design::Language language = Design::Language::VERILOG_2001;

  std::string commandsList;
  std::string libList;
  std::string fileList;
  for (int i = 1; i < argc; i++) {
    const std::string type = argv[i];
    if (type == "-work") {
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
    } else if (type == "-VHDL_2019") {
      language = Design::Language::VHDL_2019;
      actualType = "VHDL_2019";
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
    } else if (type == "-C") {
      language = Design::Language::C;
      actualType = "C";
    } else if (type == "-CPP") {
      language = Design::Language::CPP;
      actualType = "C++";
    } else if (type.find("-D") != std::string::npos) {
      fileList += type + " ";
    } else {
      if (actualType.empty()) {
        auto fileLowerCase = StringUtils::toLower(argv[i]);
        if (strstr(fileLowerCase.c_str(), ".vhd")) {
          language = Design::Language::VHDL_2008;
          actualType = "VHDL_2008";
        } else if (strstr(fileLowerCase.c_str(), ".sv")) {
          language = Design::Language::SYSTEMVERILOG_2017;
          actualType = "SV_2017";
        } else if (StringUtils::endsWith(fileLowerCase.c_str(), ".c") ||
                   StringUtils::endsWith(fileLowerCase.c_str(), ".cc")) {
          language = Design::Language::C;
          actualType = "C";
        } else if (strstr(fileLowerCase.c_str(), ".cpp")) {
          language = Design::Language::CPP;
          actualType = "C++";
        } else {
          actualType = "VERILOG_2001";
        }
      }
      const std::string file = argv[i];
      std::string expandedFile = file;
      if (expandedFile.find(" ") != std::string::npos) {
        compiler->ErrorMessage(expandedFile +
                               ": file name with space not supported.");
        return TCL_ERROR;
      }
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

  compiler->Message("Adding " + actualType + " " + fileList);
  std::ostringstream out;
  bool ok{true};
  if (filesType == Design) {
    ok = compiler->m_tclCmdIntegration->TclAddDesignFiles(
        commandsList.c_str(), libList.c_str(), fileList.c_str(), language, out);
  } else {
    ok = compiler->m_tclCmdIntegration->TclAddSimulationFiles(
        commandsList.c_str(), libList.c_str(), fileList.c_str(), language, out);
  }

  if (!ok) {
    compiler->ErrorMessage(out.str());
    return TCL_ERROR;
  }
  return TCL_OK;
}
