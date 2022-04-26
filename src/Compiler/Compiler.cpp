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
#include <sstream>
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/Constraints.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "CompilerDefines.h"
#include "MainWindow/Session.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "TaskManager.h"

using namespace FOEDAG;

void Compiler::help(std::ostream* out) {
  (*out) << "-------------------------" << std::endl;
  (*out) << "-----  FOEDAG HELP  -----" << std::endl;
  (*out) << "-------------------------" << std::endl;
  (*out) << "Options:" << std::endl;
  (*out) << "   --help:  This help" << std::endl;
  (*out) << "   --noqt:  Tcl only, no GUI" << std::endl;
  (*out) << "   --batch: Tcl only, no GUI" << std::endl;
  (*out) << "   --replay <script>: Replay GUI test" << std::endl;
  (*out) << "   --script <script>: Execute a Tcl script" << std::endl;
  (*out) << "   --compiler <name>: Compiler name {openfpga...}, default is "
            "a dummy compiler"
         << std::endl;
  (*out) << "Tcl commands:" << std::endl;
  (*out) << "   help" << std::endl;
  (*out) << "   gui_start" << std::endl;
  (*out) << "   gui_stop" << std::endl;
  (*out) << "   create_design <name>" << std::endl;
  (*out) << "   add_design_file <file> <type> (VHDL_1987, VHDL_1993, "
            "VHDL_2008, V_1995, "
            "V_2001, SV_2005, SV_2009, SV_2012, SV_2017) "
         << std::endl;
  (*out) << "   set_top_module <top>" << std::endl;
  (*out) << "   add_constraint_file <file>: Sets SDC + location constraints"
         << std::endl;
  (*out) << "     Constraints: set_pin_loc, set_region_loc, all SDC commands"
         << std::endl;
  (*out) << "   batch { cmd1 ... cmdn } : Run compilation script using the "
            "commands below"
         << std::endl;
  (*out) << "   ipgenerate" << std::endl;
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
}

void Compiler::SetTclInterpreterHandler(
    TclInterpreterHandler* tclInterpreterHandler) {
  m_tclInterpreterHandler = tclInterpreterHandler;
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
}

Compiler::~Compiler() {
  delete m_taskManager;
  delete m_tclCmdIntegration;
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

bool Compiler::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  if (m_constraints == nullptr) m_constraints = new Constraints();
  m_constraints->registerCommands(interp);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    compiler->help(compiler->GetOutStream());
    return TCL_OK;
  };
  interp->registerCmd("help", help, this, 0);

  auto create_design = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc == 2) {
      name = argv[1];
    }
    compiler->m_output.clear();
    bool ok = compiler->CreateDesign(name);
    if (!compiler->m_output.empty())
      Tcl_AppendResult(interp, compiler->m_output.c_str(), nullptr);
    return ok ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("create_design", create_design, this, 0);

  auto set_active_design = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc != 2) {
      compiler->ErrorMessage("Specify a design name");
      return TCL_ERROR;
    }
    name = argv[1];
    if (compiler->GetDesign(name)) {
      compiler->SetActiveDesign(name);
      if (compiler->m_tclCmdIntegration) {
        std::ostringstream out;
        bool ok = compiler->m_tclCmdIntegration->TclSetActive(argc, argv, out);
        if (!ok) {
          compiler->ErrorMessage(out.str());
          return TCL_ERROR;
        }
      }
    } else {
      compiler->ErrorMessage(std::string(std::string("Design ") + name +
                                         std::string(" does not exist\n")));
      return TCL_ERROR;
    }
    return TCL_OK;
  };
  interp->registerCmd("set_active_design", set_active_design, this, 0);

  auto set_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc != 2) {
      compiler->ErrorMessage("Specify a top module name");
      return TCL_ERROR;
    }
    Design* design = compiler->GetActiveDesign();
    if (design == nullptr) {
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
    design->SetTopLevel(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("set_top_module", set_top_module, this, 0);

  auto add_design_file = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    Design* design = compiler->GetActiveDesign();
    if (design == nullptr) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc < 2) {
      compiler->ErrorMessage(
          "Incorrect syntax for add_design_file <file> "
          "<type (VHDL_1987, VHDL_1993, VHDL_2008, V_1995, "
          "V_2001, SV_2005, SV_2009, SV_2012, SV_2017)>");
      return TCL_ERROR;
    }
    std::string actualType = "VERILOG_2001";
    Design::Language language = Design::Language::VERILOG_2001;
    const std::string file = argv[1];
    if (strstr(file.c_str(), ".vhd")) {
      language = Design::Language::VHDL_2008;
      actualType = "VHDL_2008";
    }
    if (argc == 3) {
      const std::string type = argv[2];
      if (type == "VHDL_1987") {
        language = Design::Language::VHDL_1987;
        actualType = "VHDL_1987";
      } else if (type == "VHDL_1993") {
        language = Design::Language::VHDL_1993;
        actualType = "VHDL_1993";
      } else if (type == "VHDL_2008") {
        language = Design::Language::VHDL_2008;
        actualType = "VHDL_2008";
      } else if (type == "V_1995") {
        language = Design::Language::VERILOG_1995;
        actualType = "VERILOG_1995";
      } else if (type == "V_2001") {
        language = Design::Language::VERILOG_2001;
        actualType = "VERILOG_2001";
      } else if (type == "V_2005") {
        language = Design::Language::SYSTEMVERILOG_2005;
        actualType = "SV_2005";
      } else if (type == "SV_2009") {
        language = Design::Language::SYSTEMVERILOG_2009;
        actualType = "SV_2009";
      } else if (type == "SV_2012") {
        language = Design::Language::SYSTEMVERILOG_2012;
        actualType = "SV_2012";
      } else if (type == "SV_2017") {
        language = Design::Language::SYSTEMVERILOG_2017;
        actualType = "SV_2017";
      }
    }
    std::string expandedFile = file;
    if (!compiler->GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(file);
      expandedFile = fullPath.string();
    }

    compiler->Message(std::string("Adding ") + actualType + " " + expandedFile +
                      std::string("\n"));
    design->AddFile(language, expandedFile);
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok =
          compiler->m_tclCmdIntegration->TclAddOrCreateFiles(argc, argv, out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("add_design_file", add_design_file, this, nullptr);

  auto set_as_target = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    if (argc != 3) {
      compiler->ErrorMessage("Usage: set_as_target ?type? ?target name?");
      return TCL_ERROR;
    }
    if (compiler->m_tclCmdIntegration) {
      std::ostringstream out;
      bool ok = compiler->m_tclCmdIntegration->TclSetAsTarget(argc, argv, out);
      if (!ok) {
        compiler->ErrorMessage(out.str());
        return TCL_ERROR;
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_as_target", set_as_target, this, nullptr);

  auto add_constraint_file = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    Design* design = compiler->GetActiveDesign();
    if (design == nullptr) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc != 2) {
      compiler->ErrorMessage("Specify a constraint file name");
      return TCL_ERROR;
    }
    const std::string file = argv[1];

    std::string expandedFile = file;
    if (!compiler->GetSession()->CmdLine()->Script().empty()) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath.append(file);
      expandedFile = fullPath.string();
    }

    compiler->Message(std::string("Adding constraint file ") + expandedFile +
                      std::string("\n"));
    design->AddConstraintFile(expandedFile);
    Tcl_Eval(interp, std::string("read_sdc " + expandedFile).c_str());
    return 0;
  };
  interp->registerCmd("add_constraint_file", add_constraint_file, this, 0);

  if (batchMode) {
    auto ipgenerate = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(IPGen) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("ipgenerate", ipgenerate, this, 0);

    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Synthesis) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

    auto packing = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Pack) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("packing", packing, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Global) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("global_placement", globalplacement, this, 0);
    interp->registerCmd("globp", globalplacement, this, 0);

    auto placement = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Detailed) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("detailed_placement", placement, this, 0);
    interp->registerCmd("place", placement, this, 0);

    auto route = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Routing) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("route", route, this, 0);

    auto sta = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(STA) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("sta", sta, this, 0);

    auto power = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Power) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("power", power, this, 0);

    auto bitstream = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Bitstream) ? TCL_OK : TCL_ERROR;
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
      WorkerThread* wthread =
          new WorkerThread("ip_th", Action::IPGen, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("ipgenerate", ipgenerate, this, 0);

    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("synth_th", Action::Synthesis, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

    auto packing = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("pack_th", Action::Pack, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("packing", packing, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("glob_th", Action::Global, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("global_placement", globalplacement, this, 0);
    interp->registerCmd("globp", globalplacement, this, 0);

    auto placement = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("place_th", Action::Detailed, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("detailed_placement", placement, this, 0);
    interp->registerCmd("place", placement, this, 0);

    auto route = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("route_th", Action::Routing, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("route", route, this, 0);

    auto sta = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread = new WorkerThread("sta_th", Action::STA, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("sta", sta, this, 0);

    auto power = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("power_th", Action::Power, compiler);
      wthread->start();
      return 0;
    };
    interp->registerCmd("power", power, this, 0);

    auto bitstream = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      WorkerThread* wthread =
          new WorkerThread("bitstream_th", Action::Bitstream, compiler);
      wthread->start();
      return 0;
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
  return true;
}

bool Compiler::Compile(Action action) {
  m_stop = false;
  bool res{false};
  uint task{toTaskId(action)};
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
  if (m_taskManager) m_taskManager->stopCurrentTask();
}

Design* Compiler::GetActiveDesign() const { return m_design; }

bool Compiler::Synthesize() {
  if ((m_design == nullptr) && !CreateDesign("noname")) return false;
  (*m_out) << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
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
      (*m_out) << " File: " << (*it).path().filename().c_str()
               << " just for test";
      it++;
    }
    (*m_out) << std::endl;
    std::chrono::milliseconds dura(1000);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Synthesized;
  (*m_out) << "Design " << m_design->Name() << " is synthesized!" << std::endl;
  return true;
}

bool Compiler::GlobalPlacement() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  if (m_state != State::Packed && m_state != State::GloballyPlaced) {
    ErrorMessage("Design needs to be in packed state");
    return false;
  }
  (*m_out) << "Global Placement for design: " << m_design->Name() << "..."
           << std::endl;
  for (int i = 0; i < 100; i = i + 10) {
    (*m_out) << i << "%" << std::endl;
    std::chrono::milliseconds dura(1000);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::GloballyPlaced;
  (*m_out) << "Design " << m_design->Name() << " is globally placed!"
           << std::endl;
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
    m_taskManager->bindTaskCommand(
        IP_GENERATE, [this]() { m_interp->evalCmd("ipgenerate"); });
    m_taskManager->bindTaskCommand(SYNTHESIS,
                                   [this]() { m_interp->evalCmd("synth"); });
    m_taskManager->bindTaskCommand(PACKING,
                                   [this]() { m_interp->evalCmd("packing"); });
    m_taskManager->bindTaskCommand(GLOBAL_PLACEMENT,
                                   [this]() { m_interp->evalCmd("globp"); });
    m_taskManager->bindTaskCommand(PLACEMENT,
                                   [this]() { m_interp->evalCmd("place"); });
    m_taskManager->bindTaskCommand(ROUTING,
                                   [this]() { m_interp->evalCmd("route"); });
    m_taskManager->bindTaskCommand(TIMING_SIGN_OFF,
                                   [this]() { m_interp->evalCmd("sta"); });
    m_taskManager->bindTaskCommand(POWER,
                                   [this]() { m_interp->evalCmd("power"); });
    m_taskManager->bindTaskCommand(
        BITSTREAM, [this]() { m_interp->evalCmd("bitstream"); });
  }
}

void Compiler::setGuiTclSync(TclCommandIntegration* tclCommands) {
  m_tclCmdIntegration = tclCommands;
}

bool Compiler::IPGenerate() {
  if ((m_design == nullptr) && !CreateDesign("noname")) return false;
  (*m_out) << "IP generation for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " IPs are generated!"
           << std::endl;
  m_state = IPGenerated;
  return true;
}

bool Compiler::Packing() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Packing for design: " << m_design->Name() << "..." << std::endl;

  (*m_out) << "Design " << m_design->Name() << " is packed!" << std::endl;
  m_state = Packed;
  return true;
}

bool Compiler::Placement() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Placement for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " is placed!" << std::endl;
  m_state = Placed;
  return true;
}

bool Compiler::Route() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Routing for design: " << m_design->Name() << "..." << std::endl;

  (*m_out) << "Design " << m_design->Name() << " is routed!" << std::endl;
  m_state = Routed;
  return true;
}

bool Compiler::TimingAnalysis() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Timing analysis for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " is analyzed!" << std::endl;
  return true;
}

bool Compiler::PowerAnalysis() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Timing analysis for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " is analyzed!" << std::endl;
  return true;
}

bool Compiler::GenerateBitstream() {
  if (m_design == nullptr) {
    ErrorMessage("No design specified");
    return false;
  }
  (*m_out) << "Bitstream generation for design: " << m_design->Name() << "..."
           << std::endl;

  (*m_out) << "Design " << m_design->Name() << " bitstream is generated!"
           << std::endl;
  return true;
}

bool Compiler::CreateDesign(const std::string& name) {
  if (m_tclCmdIntegration) {
    std::ostringstream out;
    bool ok{true};
    if (m_tclCmdIntegration->getActiveDesign().isEmpty()) {
      ok = m_tclCmdIntegration->TclCreateProject(name.c_str(), out);
    } else {
      ok = m_tclCmdIntegration->TclCreateFileSet(name.c_str(), out);
    }
    if (!out.str().empty()) m_output = out.str();
    if (!ok) return false;
  }
  if (GetDesign(name)) {
    ErrorMessage("Design already exists");
    return false;
  }

  Design* design = new Design(name);
  design->setConstraints(getConstraints());
  SetDesign(design);
  Message(std::string("Created design source: ") + name + std::string("\n"));
  return true;
}

bool Compiler::SetActiveDesign(const std::string& name) {
  for (Design* design : m_designs) {
    if (design->Name() == name) {
      m_design = design;
      return true;
    }
  }
  return false;
}

Design* Compiler::GetDesign(const std::string name) {
  for (Design* design : m_designs) {
    if (design->Name() == name) {
      return design;
    }
  }
  return nullptr;
}

void Compiler::SetDesign(Design* design) {
  m_design = design;
  m_designs.push_back(design);
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

bool Compiler::ExecuteAndMonitorSystemCommand(const std::string& command) {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  // TODO: Windows System call
  return false;
#else
  (*m_out) << "Command: " << command << std::endl;
  const char* cmd = command.c_str();
  char buf[BUFSIZ];
  FILE* ptr;

  if ((ptr = popen(cmd, "r")) != nullptr) {
    while (fgets(buf, BUFSIZ, ptr) != nullptr) {
      if (m_stop == true) {
        break;
      }
      (*m_out) << buf << std::flush;
    }
    pclose(ptr);
    if (m_stop == true) {
      Message("Execution interrupted by user!\n");
      return false;
    }
  } else {
    return false;
  }

  return true;
#endif
}

std::string Compiler::replaceAll(std::string_view str, std::string_view from,
                                 std::string_view to) {
  size_t start_pos = 0;
  std::string result(str);
  while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
    result.replace(start_pos, from.length(), to);
    start_pos += to.length();  // Handles case where 'to' is a substr of 'from'
  }
  return result;
}
