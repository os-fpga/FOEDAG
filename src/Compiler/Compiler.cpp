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
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "CompilerDefines.h"

using namespace FOEDAG;

Compiler::Compiler(TclInterpreter* interp, std::ostream* out,
                   TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_out(out),
      m_tclInterpreterHandler(tclInterpreterHandler) {
  if (m_tclInterpreterHandler) m_tclInterpreterHandler->setCompiler(this);
}

Compiler::~Compiler() { delete m_taskManager; }

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
  auto create_design = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc == 2) {
      name = argv[1];
    }
    if (compiler->GetDesign(name)) {
      Tcl_AppendResult(interp, "ERROR: design already exists", (char*)NULL);
      return TCL_ERROR;
    } else {
      Design* design = new Design(name);
      compiler->SetDesign(design);
      compiler->Message(std::string("Created design: ") + name +
                        std::string("\n"));
    }
    return 0;
  };
  interp->registerCmd("create_design", create_design, this, 0);

  auto set_active_design = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc != 2) {
      Tcl_AppendResult(interp, "ERROR: Specify a design name", (char*)NULL);
      return TCL_ERROR;
    }
    if (compiler->GetDesign(argv[1])) {
      compiler->SetActiveDesign(argv[1]);
    } else {
      Tcl_AppendResult(interp,
                       std::string(std::string("ERROR: design ") + name +
                                   std::string(" does not exist\n"))
                           .c_str(),
                       (char*)NULL);
      return TCL_ERROR;
    }
    return 0;
  };
  interp->registerCmd("set_active_design", set_active_design, this, 0);

  auto set_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    std::string name = "noname";
    if (argc != 2) {
      Tcl_AppendResult(interp, "ERROR: Specify a top module name", (char*)NULL);
      return TCL_ERROR;
    }
    Design* design = compiler->GetActiveDesign();
    if (design == nullptr) {
      Tcl_AppendResult(interp,
                       "ERROR: create a design first: create_design <name>",
                       (char*)NULL);
      return TCL_ERROR;
    }
    design->TopLevel(argv[1]);
    return 0;
  };
  interp->registerCmd("set_top_module", set_top_module, this, 0);

  auto add_design_file = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*)clientData;
    Design* design = compiler->GetActiveDesign();
    if (design == nullptr) {
      Tcl_AppendResult(interp,
                       "ERROR: create a design first: create_design <name>",
                       (char*)NULL);
      return TCL_ERROR;
    }
    if (argc < 2 || argc > 3) {
      Tcl_AppendResult(interp,
                       "ERROR: Incorrect syntax for add_design_file <file> "
                       "<type (VHDL_1987, VHDL_1993, VHDL_2008, V_1995, "
                       "V_2001, SV_2005, SV_2009, SV_2012, SV_2017)>",
                       (char*)NULL);
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
    compiler->Message(std::string("Adding ") + actualType + " " + file +
                      std::string("\n"));
    design->AddFile(language, file);
    return 0;
  };
  interp->registerCmd("add_design_file", add_design_file, this, 0);

  if (batchMode) {
    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      return compiler->Compile(Synthesis) ? TCL_OK : TCL_ERROR;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

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
    interp->registerCmd("placement", placement, this, 0);

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
    interp->registerCmd("placement", placement, this, 0);

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
          new WorkerThread("bitstream_th", Action::STA, compiler);
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
  uint task{TaskManager::invalid_id};
  switch (action) {
    case Action::Synthesis:
      task = SYNTHESIS;
      break;
    case Action::Global:
      task = PLACEMENT;
      break;
    case Action::Detailed:
      task = PLACEMENT;
      break;
    case Action::Routing:
      task = ROUTING;
      break;
    case Action::STA:
      task = TIMING_SIGN_OFF;
      break;
    case Action::Power:
      task = POWER;
      break;
    case Action::Bitstream:
      task = BITSTREAM;
      break;
    case Action::Batch:
      break;
    default:
      break;
  }
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

bool Compiler::Synthesize() {
  if (m_design == nullptr) {
    std::string name = "noname";
    Design* design = new Design(name);
    SetDesign(design);
    Message(std::string("Created design: ") + name + std::string("\n"));
  }
  (*m_out) << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
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
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  if (m_state != State::Synthesized) {
    (*m_out) << "ERROR: Design needs to be in synthesized state" << std::endl;
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
    case Action::Synthesis:
      return Synthesize();
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
    m_taskManager->bindTaskCommand(m_taskManager->task(SYNTHESIS), [this]() {
      Tcl_Eval(m_interp->getInterp(), "synth");
    });
    m_taskManager->bindTaskCommand(m_taskManager->task(PLACEMENT), [this]() {
      Tcl_Eval(m_interp->getInterp(), "globp");
    });
  }
}

bool Compiler::Placement() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool Compiler::Route() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool Compiler::TimingAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool Compiler::PowerAnalysis() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool Compiler::GenerateBitstream() {
  if (m_design == nullptr) {
    (*m_out) << "ERROR: No design specified" << std::endl;
    return false;
  }
  return true;
}

bool Compiler::SetActiveDesign(const std::string name) {
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
  const char* cmd = command.c_str();
  char buf[BUFSIZ];
  FILE* ptr;

  if ((ptr = popen(cmd, "r")) != nullptr) {
    while (fgets(buf, BUFSIZ, ptr) != nullptr) {
      (*m_out) << buf << std::flush;
    }
    pclose(ptr);
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