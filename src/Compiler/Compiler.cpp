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

#include "Compiler/Compiler.h"
#include "Compiler/WorkerThread.h"

using namespace FOEDAG;

Compiler::~Compiler() {}

bool Compiler::registerCommands() {
  auto synthesize = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int { 
    Compiler* compiler = (Compiler*) clientData;
    WorkerThread* wthread = new WorkerThread("synth_th", Action::Synthesis, compiler);
    wthread->start();
    return 0; 
  };
  m_interp->registerCmd("synthesize", synthesize, this, 0);

  auto globalplacement = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    Compiler* compiler = (Compiler*) clientData;
    WorkerThread* wthread = new WorkerThread("glob_th", Action::Global, compiler);
    wthread->start();
    return 0; 
  };
  m_interp->registerCmd("global_placement", globalplacement, this, 0);

  auto stop = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    for (auto th : ThreadPool::threads) {
      th->stop();
    }
    ThreadPool::threads.clear();
    return 0;
  };
  m_interp->registerCmd("stop", stop, 0, 0);

  return true;
}

bool Compiler::compile(Action action) {
  m_stop = false;
  switch (action) {
    case Action::Synthesis: 
      return Synthesize();
    case Action::Global:
      return GlobalPlacement();
    default:
      break;
  }
  return false;
}

bool Compiler::Synthesize() {
  m_out << "Synthesizing design: " << m_design->Name() << "..." << std::endl;
  for (int i = 0; i < 100; i = i + 10) {
    m_out << i << "%" << std::endl;
    sleep(1);
    if (m_stop) 
      return false;
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
  m_out << "Global Placement for design: " << m_design->Name() << "..." << std::endl;
  for (int i = 0; i < 100; i = i + 10) {
    m_out << i << "%" << std::endl;
    sleep(1);
    if (m_stop) 
      return false;
  }
  m_state = State::GloballyPlaced;
  m_out << "Design " << m_design->Name() << " is globally placed!" << std::endl;
  return true;
}

bool Compiler::Placement() { return true; }

bool Compiler::Route() { return true; }

bool Compiler::TimingAnalysis() { return true; }

bool Compiler::GenerateBitstream() { return true; }
