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

#include "Compiler.h"

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

#include "Compiler/Constraints.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "CompilerDefines.h"
#include "Configuration/CFGCompiler/CFGCompiler.h"
#include "DesignQuery/DesignQuery.h"
#include "DeviceModeling/DeviceModeling.h"
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
#include "scope_guard.hpp"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
LogLevel SpeedLog::speed_logLevel = LOG_INFO;

auto CreateDummyLog = [](Compiler::Action action,
                         const std::string& outfileName,
                         Compiler* compiler) -> std::filesystem::path {
  std::filesystem::path outputPath = compiler->FilePath(action, outfileName);
  if (FileUtils::FileExists(outputPath)) {
    std::filesystem::remove(outputPath);
  }
  FileUtils::WriteToFile(outputPath, "Dummy log for " + outfileName);
  return outputPath;
};

void Compiler::Version(std::ostream* out) {
  (*out) << "FOEDAG"
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

Compiler::Action Compiler::ToCompilerAction(Simulator::SimulationType type) {
  switch (type) {
    case Simulator::SimulationType::RTL:
      return Compiler::Action::SimulateRTL;
    case Simulator::SimulationType::PNR:
      return Compiler::Action::SimulatePNR;
    case Simulator::SimulationType::Gate:
      return Compiler::Action::SimulateGate;
    case Simulator::SimulationType::BitstreamBackDoor:
    case Simulator::SimulationType::BitstreamFrontDoor:
      return Compiler::Action::SimulateBitstream;
  }
  return Compiler::Action::NoAction;
}

Compiler::Compiler(TclInterpreter* interp, std::ostream* out,
                   TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_out(out),
      m_tclInterpreterHandler(tclInterpreterHandler) {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
  SetConstraints(new Constraints{this});
  IPCatalog* catalog = new IPCatalog();
  SetIPGenerator(new IPGenerator(catalog, this));
  m_simulator = new Simulator(m_interp, this, m_out, m_tclInterpreterHandler);
  m_netlistEditData = new NetlistEditData();
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
  delete m_netlistEditData;
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

void Compiler::SetIPGenerator(IPGenerator* generator) {
  m_IPGenerator = generator;
  if (m_tclCmdIntegration) m_tclCmdIntegration->setIPGenerator(m_IPGenerator);
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
  if (m_interp != nullptr)
    if (append)
      Tcl_AppendResult(m_interp->getInterp(), message.c_str(), nullptr);
}

void Compiler::CleanFiles(Action action) {
  auto base = FilePath(action);
  if (!base.empty()) FileUtils::removeAll(base);
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
    SetIPGenerator(new IPGenerator(catalog, this));
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
    auto allTasks = compiler->GetTaskManager()->tasks();
    for (auto task : allTasks) {
      if (task && task->status() == TaskStatus::Fail) {
        compiler->ErrorMessage(task->title().toStdString() + " task failed");
        return TCL_ERROR;
      }
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
    SetIPGenerator(new IPGenerator(catalog, this));
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
  if (m_DeviceModeling == nullptr) {
    m_DeviceModeling = new DeviceModeling(this);
  }
  m_DeviceModeling->RegisterCommands(interp, batchMode);

  auto device_file = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Compiler* compiler = static_cast<Compiler*>(clientData);
    if (argc < 2) {
      compiler->ErrorMessage("File is missing");
      return TCL_ERROR;
    }
    if (compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("device_file can be set before design created");
      return TCL_ERROR;
    }
    std::string deviceFile{argv[1]};
    std::filesystem::path deviceFilePath{deviceFile};
    if (!FileUtils::FileExists(deviceFilePath)) {
      compiler->ErrorMessage(
          StringUtils::format("Can't open file %", deviceFile));
      return TCL_ERROR;
    }
    compiler->DeviceFile(deviceFilePath);
    return TCL_OK;
  };
  interp->registerCmd("device_file", device_file, this, nullptr);

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

  auto get_design_name = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = compiler->ProjManager()->projectName();
    Tcl_SetResult(interp, (char*)name.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("get_design_name", get_design_name, this, nullptr);

  auto get_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = compiler->ProjManager()->DesignTopModule();
    Tcl_SetResult(interp, (char*)name.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("get_top_module", get_top_module, this, nullptr);

  auto get_bin_path = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = compiler->GetBinPath().string();
    Tcl_SetResult(interp, (char*)name.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("get_bin_path", get_bin_path, this, nullptr);

  auto get_data_path = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = compiler->GetDataPath().string();
    Tcl_SetResult(interp, (char*)name.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("get_data_path", get_data_path, this, nullptr);

  auto get_python3_path = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::filesystem::path python3Path = compiler->GetDataPath() / ".." /
                                        "envs" / "python3.8" / "bin" /
                                        "python3";
    std::string name = python3Path.string();
    Tcl_SetResult(interp, (char*)name.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("get_python3_path", get_python3_path, this, nullptr);

  auto message = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string text;
    for (int i = 1; i < argc; i++) {
      text += std::string(argv[i]) + " ";
    }
    compiler->Message(text);
    return TCL_OK;
  };
  interp->registerCmd("message", message, this, nullptr);

  auto error_message = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string text;
    for (int i = 1; i < argc; i++) {
      text += std::string(argv[i]) + " ";
    }
    compiler->ErrorMessage(text);
    return TCL_ERROR;
  };
  interp->registerCmd("error_message", error_message, this, nullptr);

  auto get_top_simulation_module = [](void* clientData, Tcl_Interp* interp,
                                      int argc, const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = compiler->ProjManager()->SimulationTopModule();
    Tcl_AppendResult(interp, strdup(name.c_str()), nullptr);
    return TCL_OK;
  };
  interp->registerCmd("get_top_simulation_module", get_top_simulation_module,
                      this, nullptr);

  auto get_design_files = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;

    std::string fileList{};
    for (const auto& lang_file : compiler->ProjManager()->DesignFiles()) {
      fileList += lang_file.second + " ";
    }
    Tcl_AppendResult(interp, strdup(fileList.c_str()), nullptr);
    return TCL_OK;
  };
  interp->registerCmd("get_design_files", get_design_files, this, nullptr);

  auto get_simulation_files = [](void* clientData, Tcl_Interp* interp, int argc,
                                 const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;

    std::string fileList{};
    for (const auto& lang_file : compiler->ProjManager()->SimulationFiles()) {
      fileList += lang_file.second + " ";
    }
    Tcl_AppendResult(interp, strdup(fileList.c_str()), nullptr);
    return TCL_OK;
  };
  interp->registerCmd("get_simulation_files", get_simulation_files, this,
                      nullptr);

  auto create_design = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    std::string type{"rtl"};
    bool cleanup{false};
    if (argc >= 2) {
      name = argv[1];
    }
    for (int i = 2; i < argc; i++) {
      std::string arg{argv[i]};
      if (arg == "clean")
        cleanup = true;
      else if (arg == "-type" && (i < argc - 1))
        type = argv[i + 1];
    }
    compiler->m_tclCmdIntegration->TclCloseProject();
    Tcl_ResetResult(interp);  // cleanup after close project
    const auto& [ok, message] = compiler->CreateDesign(name, type, cleanup);
    if (!message.empty()) Tcl_AppendResult(interp, message.c_str(), nullptr);
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
    if (compiler->ProjManager()->projectType() != GateLevel) {
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
        {}, {}, {expandedFile}, language, out);
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
    bool ok =
        compiler->m_tclCmdIntegration->TclAddConstrFiles(expandedFile, out);
    if (!ok) {
      compiler->ErrorMessage(out.str());
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("add_constraint_file", add_constraint_file, this, 0);

  auto constraint_file_policy = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (argc != 2) {
      compiler->ErrorMessage("Please specify one constraint file policy");
      return TCL_ERROR;
    }
    std::string policy = argv[1];
    if (policy == "vpr") {
      compiler->getConstraints()->SetPolicy(ConstraintPolicy::VPR);
    } else if (policy == "SDCCompatible") {
      compiler->getConstraints()->SetPolicy(ConstraintPolicy::SDCCompatible);
    } else if (policy == "SDC") {
      compiler->getConstraints()->SetPolicy(ConstraintPolicy::SDC);
    } else {
      compiler->ErrorMessage(
          "Please specify a valid constraint file policy: vpr, SDC, or "
          "SDCCompatible");
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("constraint_file_policy", constraint_file_policy, this,
                      0);

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

  auto verify_synth_ports = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    return compiler->verifySynthPorts(compiler, interp, argc, argv);
  };
  interp->registerCmd("verify_synth_ports", verify_synth_ports, this, nullptr);

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
                   arg == "timed_pnr" || arg == "bitstream_fd" ||
                   arg == "bitstream_bd") {
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
        } else if (sim_type == "pnr" || sim_type == "timed_pnr") {
          compiler->GetSimulator()->SetTimedSimulation(
              (sim_type == "timed_pnr"));
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
          compiler->SynthOptimization(SynthesisOptimization::Mixed);
        } else if (arg == "area") {
          compiler->SynthOptimization(SynthesisOptimization::Area);
        } else if (arg == "delay") {
          compiler->SynthOptimization(SynthesisOptimization::Delay);
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
      return compiler->Compile(Action::Placement) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("detailed_placement", placement, this, 0);
    interp->registerCmd("place", placement, this, 0);

    auto pin_loc_assign_method = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      auto setPlaceOption = [compiler](const std::string& arg) {
        if (arg == "random") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
          compiler->Message("Pin Method: " + arg);
        } else if (arg == "in_define_order") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
          compiler->Message("Pin Method: " + arg);
        } else if (arg == "free") {
          compiler->PinAssignOpts(
              Compiler::PinAssignOpt::Pin_constraint_disabled);
          compiler->Message("Warning: Deprecated Pin Method: " + arg);
        } else if (arg == "pin_constraint_disabled") {
          compiler->PinAssignOpts(
              Compiler::PinAssignOpt::Pin_constraint_disabled);
          compiler->Message("Pin Method: " + arg);
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
        } else if (arg == "opensta") {
          compiler->TimingAnalysisEngineOpt(Compiler::STAEngineOpt::Opensta);
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
          compiler->BitsFlags(BitstreamFlags::Force);
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else if (arg == "enable_simulation") {
          compiler->BitsFlags(BitstreamFlags::EnableSimulation);
        } else if (arg == "write_xml") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "write_fabric_independent") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "pb_pin_fixup") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "wl_decremental_order") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "ignore_dont_care_bits") {
          compiler->BitstreamMoreOpt(arg);
        } else {
          compiler->ErrorMessage("Unknown bitstream option: " + arg);
        }
      }
      return compiler->Compile(Action::Bitstream) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("bitstream", bitstream, this, 0);

    auto compile2bits = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      compiler->Compile2bits(true);
      return compiler->Compile(Action::Bitstream) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("compile2bits", compile2bits, this, 0);

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
                   arg == "timed_pnr" || arg == "bitstream_fd" ||
                   arg == "bitstream_bd") {
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
        } else if (sim_type == "pnr" || sim_type == "timed_pnr") {
          compiler->GetSimulator()->SetTimedSimulation(
              (sim_type == "timed_pnr"));
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
          compiler->SynthOptimization(SynthesisOptimization::Mixed);
        } else if (arg == "area") {
          compiler->SynthOptimization(SynthesisOptimization::Area);
        } else if (arg == "delay") {
          compiler->SynthOptimization(SynthesisOptimization::Delay);
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
      WorkerThread* wthread = new WorkerThread("place_th", Action::Placement,
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
          compiler->Message("Pin Method: " + arg);
        } else if (arg == "in_define_order") {
          compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
          compiler->Message("Pin Method: " + arg);
        } else if (arg == "free") {
          compiler->PinAssignOpts(
              Compiler::PinAssignOpt::Pin_constraint_disabled);
          compiler->Message("Warning: Deprecated Pin Method: " + arg);
        } else if (arg == "pin_constraint_disabled") {
          compiler->PinAssignOpts(
              Compiler::PinAssignOpt::Pin_constraint_disabled);
          compiler->Message("Pin Method: " + arg);
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
        } else if (arg == "opensta") {
          compiler->TimingAnalysisEngineOpt(Compiler::STAEngineOpt::Opensta);
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
          compiler->BitsFlags(BitstreamFlags::Force);
        } else if (arg == "clean") {
          compiler->BitsOpt(Compiler::BitstreamOpt::Clean);
        } else if (arg == "enable_simulation") {
          compiler->BitsFlags(BitstreamFlags::EnableSimulation);
        } else if (arg == "write_xml") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "write_fabric_independent") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "pb_pin_fixup") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "wl_decremental_order") {
          compiler->BitstreamMoreOpt(arg);
        } else if (arg == "ignore_dont_care_bits") {
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

    auto compile2bits = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      compiler->Compile2bits(true);
      WorkerThread* wthread =
          new WorkerThread("bitstream_th", Action::Bitstream, compiler);
      return wthread->start() ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("compile2bits", compile2bits, this, 0);

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

  auto ip_add_to_design = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (compiler && compiler->GuiTclSync()) {
      if (argc < 2) {
        compiler->ErrorMessage("IP name missed.");
        return TCL_ERROR;
      }
      for (int i = 1; i < argc; i++) {
        std::stringstream out;
        const std::string ipName = argv[i];
        if (!compiler->GuiTclSync()->TclAddIpToDesign(ipName, out)) {
          compiler->ErrorMessage(out.str());
          return TCL_ERROR;
        }
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("ip_add_to_design", ip_add_to_design, this, nullptr);

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
void Compiler::AddHeadersToLogs(Action action) {
  auto projManager = ProjManager();
  if (projManager) {
    LogUtils::AddHeadersToLogs(FilePath(action));
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

void Compiler::SetError(const std::string& message) {
  m_errorState = ErrorState{message};
}

void Compiler::ResetError() { m_errorState = ErrorState{}; }

std::filesystem::path Compiler::FilePath(Action action) const {
  if (!ProjManager()) return {};

  fs::path synth{ProjectManager::synthPath(ProjManager()->projectPath())};
  fs::path impl{ProjectManager::implPath(ProjManager()->projectPath())};
  fs::path run{ProjectManager::projectBasePath(ProjManager()->projectPath())};
  switch (action) {
    case Action::Analyze:
      return synth / "analysis";
    case Action::Synthesis:
      return synth / "synthesis";
    case Action::SimulateRTL:
      return run / "simulate_rtl";
    case Action::SimulateGate:
      return synth / "simulate_gate";
    case Action::SimulatePNR:
      return impl / "simulate_pnr";
    case Action::SimulateBitstream:
      return impl / "simulate_bitstream";
    case Action::Pack:
      return impl / "packing";
    case Action::Placement:
      return impl / "placement";
    case Action::Routing:
      return impl / "routing";
    case Action::STA:
      return impl / "timing_analysis";
    case Action::Power:
      return impl / "power_analysis";
    case Action::Bitstream:
      return impl / "bitstream";
    default:
      return {};
  }
}

std::filesystem::path Compiler::FilePath(Action action,
                                         const std::string& file) const {
  return FilePath(action) / file;
}

std::vector<std::string> Compiler::TopModules(
    const std::filesystem::path& ports_info) const {
  std::vector<std::string> topModules;
  if (FileUtils::FileExists(ports_info)) {
    std::ifstream file(ports_info);
    json data = json::parse(file);
    if (data.is_array()) {
      std::transform(
          data.begin(), data.end(), std::back_inserter(topModules),
          [](json val) -> std::string { return val.value("topModule", ""); });
    }
  }
  return topModules;
}

DesignQuery* Compiler::GetDesignQuery() { return m_DesignQuery; }

void Compiler::Compile2bits(bool compile2bits) {
  m_compile2bits = compile2bits;
}

std::filesystem::path Compiler::DeviceFile() const { return m_deviceFile; }

void Compiler::DeviceFile(const std::filesystem::path& file) {
  m_deviceFile = file;
}

string Compiler::DesignTopModule() const {
  if (!ProjManager()) return {};
  if (ProjManager()->DesignTopModule().empty()) {
    auto port_info = FilePath(Action::Analyze, "hier_info.json");
    auto topModules = TopModules(port_info);
    for (const auto& topModule : topModules) {
      if (!topModule.empty())
        m_projManager->setCurrentFileSet(
            m_projManager->getDesignActiveFileSet());
      m_projManager->setTopModule(QString::fromStdString(topModule));
    }
  }
  return ProjManager()->DesignTopModule();
}

bool Compiler::DeviceFileLocal() const { return m_deviceFileLocal; }

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
    m_taskManager->task(task)->setEnable(true);
  }
  m_utils = {};
  res = SwitchCompileContext(
      action, [this, action]() { return RunCompileTask(action); });
  if (task != TaskManager::invalid_id && m_taskManager) {
    m_taskManager->task(task)->setStatus(res ? TaskStatus::Success
                                             : TaskStatus::Fail);
    if (res) m_taskManager->task(task)->setUtilization(m_utils);
  }
  return res;
}

void Compiler::GenerateReport(int action) {
  Action act = static_cast<Action>(action);
  auto files = FileUtils::FindFilesByExtension(FilePath(act), ".rpt");
  for (const auto& file : files) {
    std::ifstream ifs(file);
    if (ifs.good()) {
      std::stringstream buffer;
      buffer << ifs.rdbuf();
      ifs.close();
      std::string contents = buffer.str();
      for (const auto& pair :
           getNetlistEditData()->getReversePrimaryInputMap()) {
        contents = StringUtils::replaceAll(contents, pair.first, pair.second);
      }
      for (const auto& pair :
           getNetlistEditData()->getReversePrimaryOutputMap()) {
        contents = StringUtils::replaceAll(contents, pair.first, pair.second);
      }
      std::filesystem::path temporaryFile = FilePath(act) / "tmp.rpt";
      std::ofstream ofs(temporaryFile.string());
      if (ofs.good()) {
        ofs << contents;
        if (ofs.good()) {
          ofs.close();

          std::error_code ec;
          std::filesystem::copy_file(
              temporaryFile, file,
              std::filesystem::copy_options::overwrite_existing, ec);
          if (ec) {
            qWarning() << "Failed to write data to: " << file.c_str()
                       << ". Error: " << ec.message().c_str();
          } else {
            std::filesystem::remove(temporaryFile);
          }
        } else {
          qWarning() << "Failed to write content to file: "
                     << temporaryFile.c_str();
        }
      } else {
        qWarning() << "Failed to open file: " << temporaryFile.c_str();
      }
    }
  }

  handleJsonReportGeneration(m_taskManager->task(toTaskId(action, this)),
                             m_taskManager,
                             QString::fromStdString(FilePath(act).string()));
  if (act == Action::Analyze && m_tclCmdIntegration) {
    m_tclCmdIntegration->updateHierarchyView();
  }
}

void Compiler::Stop() {
  m_stop = true;
  ErrorMessage("Interrupted by user");
  if (m_process) m_process->terminate();
  FileUtils::terminateSystemCommand();
}

void Compiler::ResetStopFlag() { m_stop = false; }

bool Compiler::Analyze() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
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

  CreateDummyLog(Action::Analyze, ANALYSIS_LOG, this);
  return true;
}

bool Compiler::Synthesize() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (SynthOpt() == SynthesisOpt::Clean) {
    Message("Cleaning synthesis results for " + m_projManager->projectName());
    SynthOpt(SynthesisOpt::None);
    m_state = State::IPGenerated;
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

  CreateDummyLog(Action::Synthesis, SYNTHESIS_LOG, this);
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

  CreateDummyLog(Action::Global, GLOBAL_PLACEMENT_LOG, this);
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
  auto guard = sg::make_scope_guard([this, currentTask, action] {
    AddHeadersToLogs(action);
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
    case Action::Placement:
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

bool Compiler::SwitchCompileContext(Action action,
                                    const std::function<bool()>& fn) {
  auto compilePath = FilePath(action);
  auto current_path = fs::current_path();
  if (!compilePath.empty()) {
    // make sure path exists
    bool ok = FileUtils::MkDirs(compilePath);
    if (ok) {
      // switch actions context here
      std::filesystem::current_path(compilePath);
    }
  }
  auto res = fn();
  if (!compilePath.empty()) fs::current_path(current_path);
  return res;
}

void Compiler::setTaskManager(TaskManager* newTaskManager) {
  m_taskManager = newTaskManager;
  if (m_taskManager) {
    static const std::vector<std::pair<uint, std::string>> commands = {
        {IP_GENERATE, "ipgenerate"},
        {ANALYSIS, "analyze"},
        {ANALYSIS_CLEAN, "analyze clean"},
        {SYNTHESIS, "synth"},
        {SYNTHESIS_CLEAN, "synth clean"},
        {PACKING, "packing"},
        {PACKING_CLEAN, "packing clean"},
        {PLACEMENT, "place"},
        {PLACEMENT_CLEAN, "place clean"},
        {ROUTING, "route"},
        {ROUTING_CLEAN, "route clean"},
        {TIMING_SIGN_OFF, "sta"},
        {TIMING_SIGN_OFF_CLEAN, "sta clean"},
        {POWER, "power"},
        {POWER_CLEAN, "power clean"},
        {BITSTREAM, "bitstream"},
        {BITSTREAM_COMPILE2BIT, "compile2bits"},
        {BITSTREAM_CLEAN, "bitstream clean"},
        {PLACE_AND_ROUTE_VIEW, "sta view"},
        {SIMULATE_RTL, "simulate rtl"},
        {SIMULATE_RTL_CLEAN, "simulate rtl clean"},
        {SIMULATE_GATE, "simulate gate"},
        {SIMULATE_GATE_CLEAN, "simulate gate clean"},
        {SIMULATE_PNR, "simulate pnr"},
        {SIMULATE_PNR_CLEAN, "simulate pnr clean"},
        {SIMULATE_BITSTREAM, "simulate bitstream"},
        {SIMULATE_BITSTREAM_CLEAN, "simulate bitstream clean"}};
    for (auto& [id, cmd] : commands) {
      const std::string command{cmd};
      m_taskManager->bindTaskCommand(id, [command]() {
        GlobalSession->CmdStack()->push_and_exec(new Command(command));
      });
    }
    m_taskManager->task(SIMULATE_RTL)
        ->setCustomData(
            CustomData{CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::RTL)});
    m_taskManager->task(SIMULATE_PNR)
        ->setCustomData(
            CustomData{CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::PNR)});
    m_taskManager->task(SIMULATE_GATE)
        ->setCustomData(
            CustomData{CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::Gate)});
    m_taskManager->task(SIMULATE_BITSTREAM)
        ->setCustomData(CustomData{
            CustomDataType::Sim,
            static_cast<int>(Simulator::SimulationType::BitstreamBackDoor)});
  }
  SetDeviceResources();
}

TaskManager* Compiler::GetTaskManager() const { return m_taskManager; }

void Compiler::setGuiTclSync(TclCommandIntegration* tclCommands) {
  m_tclCmdIntegration = tclCommands;
  if (m_tclCmdIntegration) {
    m_projManager = m_tclCmdIntegration->GetProjectManager();
    m_tclCmdIntegration->setIPGenerator(GetIPGenerator());
  }
}

std::vector<std::string> Compiler::helpTags() const { return {"foedag"}; }

TclCommandIntegration* Compiler::GuiTclSync() const {
  return m_tclCmdIntegration;
}

bool Compiler::IPGenerate() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
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

  CreateDummyLog(Action::IPGen, IP_GENERATE_LOG, this);
  return status;
}

bool Compiler::Packing() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  if (PackOpt() == PackingOpt::Clean) {
    Message("Cleaning packing results for " + ProjManager()->projectName());
    m_state = State::Synthesized;
    PackOpt(PackingOpt::None);
    std::filesystem::remove(ProjManager()->projectName() + "_post_synth.net");
    return true;
  }
  Message("Packing for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is packed");
  m_state = State::Packed;

  CreateDummyLog(Action::Pack, PACKING_LOG, this);
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
    std::filesystem::remove(ProjManager()->projectName() + "_post_synth.place");
    return true;
  }
  Message("Placement for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is placed");
  m_state = State::Placed;

  CreateDummyLog(Action::Placement, PLACEMENT_LOG, this);
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
    std::filesystem::remove(ProjManager()->projectName() + "_post_synth.route");
    return true;
  }

  Message("Routing for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is routed");
  m_state = State::Routed;

  CreateDummyLog(Action::Routing, ROUTING_LOG, this);
  return true;
}

bool Compiler::TimingAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Timing analysis for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is analyzed");
  CreateDummyLog(Action::STA, TIMING_ANALYSIS_LOG, this);
  return true;
}

bool Compiler::PowerAnalysis() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Power analysis for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " is analyzed");

  CreateDummyLog(Action::Power, POWER_ANALYSIS_LOG, this);
  return true;
}

bool Compiler::GenerateBitstream() {
  if (!m_projManager->HasDesign()) {
    ErrorMessage("No design specified");
    return false;
  }
  Message("Bitstream generation for design: " + m_projManager->projectName());
  Message("Design " + m_projManager->projectName() + " bitstream is generated");

  CreateDummyLog(Action::Bitstream, BITSTREAM_LOG, this);
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

void Compiler::TimingAnalysisEngineOpt(STAEngineOpt opt) {
  m_staEngineOpt = opt;
  if (m_tclCmdIntegration) m_tclCmdIntegration->updateReports();
}

std::pair<bool, std::string> Compiler::CreateDesign(const std::string& name,
                                                    const std::string& type,
                                                    bool cleanup) {
  std::string output{};
  if (m_tclCmdIntegration) {
    if (m_projManager->HasDesign()) m_tclCmdIntegration->TclCloseProject();

    std::ostringstream out;
    bool ok = m_tclCmdIntegration->TclCreateProject(name, type, cleanup, out);
    output = out.str();
    if (!ok) return {false, output};
    std::string message{"Created design: " + name};
    if (!type.empty()) message += ". Project type: " + type;
    Message(message);
  }
  return {true, output};
}

std::string Compiler::GetNetlistPath() const {
  std::string netlistFile;
  switch (GetNetlistType()) {
    case NetlistType::Verilog:
      netlistFile =
          "fabric_" + ProjManager()->projectName() + "_post_synth.eblif";
      break;
    case NetlistType::VHDL:
      netlistFile =
          "fabric_" + ProjManager()->projectName() + "_post_synth.eblif";
      break;
    case NetlistType::Edif:
      netlistFile =
          "fabric_" + ProjManager()->projectName() + "_post_synth.edif";
      break;
    case NetlistType::Blif:
      netlistFile =
          "fabric_" + ProjManager()->projectName() + "_post_synth.blif";
      break;
    case NetlistType::EBlif:
      netlistFile =
          "fabric_" + ProjManager()->projectName() + "_post_synth.eblif";
      break;
  }
  netlistFile = FilePath(Action::Synthesis, netlistFile).string();
  for (const auto& lang_file : ProjManager()->DesignFiles()) {
    switch (lang_file.first.language) {
      case Design::Language::VERILOG_NETLIST:
        netlistFile =
            "fabric_" + ProjManager()->projectName() + "_post_synth.eblif";
        netlistFile = FilePath(Action::Synthesis, netlistFile).string();
        break;
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

int Compiler::ExecuteAndMonitorSystemCommand(
    const std::string& command, const std::string logFile, bool appendLog,
    const std::filesystem::path& workingDir) {
  if (m_errorState) {
    ErrorMessage(m_errorState.message);
    ResetError();
    return -1;
  }
  auto start = Time::now();
  PERF_LOG("Command: " + command);
  (*m_out) << "Command: " << command << std::endl;
  std::error_code ec;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(
      workingDir.empty() ? fs::path{ProjManager()->projectPath()} : workingDir,
      ec);  // setting path
  // new QProcess must be created here to avoid issues related to creating
  // QObjects in different threads
  m_process = new QProcess;
  if (!workingDir.empty()) {
    m_process->setWorkingDirectory(QString::fromStdString(workingDir.string()));
    FileUtils::MkDirs(workingDir);
  }
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
  m_utils.utilization = max_utiliation;
  m_utils.duration = d.count();
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
  StringVector fileList;
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
      fileList.emplace_back(type);
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
          if (filesType == AddFilesType::Design) {
            language = Design::Language::VERILOG_2001;
            actualType = "VERILOG_2001";
          } else {
            language = Design::Language::SYSTEMVERILOG_2012;
            actualType = "SV_2012";
          }
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
      fileList.emplace_back(expandedFile);
    }
  }

  if (fileList.empty()) {
    compiler->ErrorMessage(QString{"Incorrect syntax for %1: file(s) missing."}
                               .arg((filesType == AddFilesType::Design)
                                        ? "add_design_file"
                                        : "add_simulation_file")
                               .toStdString());
    return TCL_ERROR;
  }

  compiler->Message("Adding " + actualType + " " +
                    StringUtils::join(fileList, " "));
  std::ostringstream out;
  bool ok{true};
  if (filesType == Design) {
    ok = compiler->m_tclCmdIntegration->TclAddDesignFiles(
        commandsList, libList, fileList, language, out);
  } else {
    ok = compiler->m_tclCmdIntegration->TclAddSimulationFiles(
        commandsList, libList, fileList, language, out);
  }

  if (!ok) {
    compiler->ErrorMessage(out.str());
    return TCL_ERROR;
  }
  return TCL_OK;
}

std::filesystem::path Compiler::GetBinPath() const {
  if (!GlobalSession) return {};
  if (!(GlobalSession->Context())) return {};
  return GlobalSession->Context()->BinaryPath();
}

std::filesystem::path Compiler::GetDataPath() const {
  if (!GlobalSession) return {};
  if (!(GlobalSession->Context())) return {};
  return GlobalSession->Context()->DataPath();
}

int Compiler::verifySynthPorts(Compiler* compiler, Tcl_Interp* interp, int argc,
                               const char* argv[]) {
  bool ok{true};
  std::ostringstream out;
  ok = compiler->m_tclCmdIntegration->TclVerifySynthPorts(out);
  if (!ok) {
    compiler->ErrorMessage(out.str());
    return TCL_ERROR;
  }
  compiler->Message(out.str());
  std::string rtlPortInfo =
      FilePath(Action::Analyze, "port_info.json").string();
  std::string nlPortInfo =
      FilePath(Action::Pack, "post_synth_ports.json").string();
  auto binPath = GlobalSession->Context()->BinaryPath();
  std::string command = binPath.string() + std::string("/verify_synth_ports ") +
                        "--rtlPortInfo " + rtlPortInfo + " --nlPortInfo " +
                        nlPortInfo;
  int systemRet = ExecuteAndMonitorSystemCommand(command);
  if (systemRet == -1) {
    compiler->ErrorMessage("Error in verify_synth_ports");
  }
  return TCL_OK;
}
