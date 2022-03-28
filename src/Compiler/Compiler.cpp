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
#else
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

Compiler::Compiler(TclInterpreter* interp, Design* design, std::ostream& out,
                   TclInterpreterHandler* tclInterpreterHandler)
    : m_interp(interp),
      m_design(design),
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
  if (batchMode) {
    auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      compiler->Compile(Synthesis);
      return 0;
    };
    interp->registerCmd("synthesize", synthesize, this, 0);
    interp->registerCmd("synth", synthesize, this, 0);

    auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
      Compiler* compiler = (Compiler*)clientData;
      compiler->Compile(Global);
      return 0;
    };
    interp->registerCmd("global_placement", globalplacement, this, 0);
    interp->registerCmd("globp", globalplacement, this, 0);

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
  m_out << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
  auto currentPath = std::filesystem::current_path();
  auto it = std::filesystem::directory_iterator{currentPath};
  for (int i = 0; i < 100; i = i + 10) {
    m_out << std::setw(2) << i << "%";
    if (it != std::filesystem::end(it)) {
      m_out << " File: " << (*it).path().filename().c_str() << " just for test";
      it++;
    }
    m_out << std::endl;
    std::chrono::milliseconds dura(1000);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::Synthesized;
  m_out << "Design " << m_design->Name() << " is synthesized!" << std::endl;
  return true;
}

bool Compiler::GlobalPlacement() {
  if (m_state != State::Synthesized) {
    m_out << "ERROR: Design needs to be in synthesized state" << std::endl;
    return false;
  }
  m_out << "Global Placement for design: " << m_design->Name() << "..."
        << std::endl;
  for (int i = 0; i < 100; i = i + 10) {
    m_out << i << "%" << std::endl;
    std::chrono::milliseconds dura(1000);
    std::this_thread::sleep_for(dura);
    if (m_stop) return false;
  }
  m_state = State::GloballyPlaced;
  m_out << "Design " << m_design->Name() << " is globally placed!" << std::endl;
  return true;
}

bool Compiler::RunBatch() {
  m_out << "Running batch..." << std::endl;
  TclInterpreter* batchInterp = new TclInterpreter("batchInterp");
  if (m_tclInterpreterHandler)
    m_tclInterpreterHandler->initIterpreter(batchInterp);
  RegisterCommands(batchInterp, true);
  m_out << batchInterp->evalCmd(m_batchScript);
  m_out << std::endl << "Batch Done." << std::endl;

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

bool Compiler::Placement() { return true; }

bool Compiler::Route() { return true; }

bool Compiler::TimingAnalysis() { return true; }

bool Compiler::GenerateBitstream() { return true; }
