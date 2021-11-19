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

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Compiler/Compiler.h"
#include "Main/CommandLine.h"
#include "Tcl/TclInterpreter.h"

#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

namespace FOEDAG {

class WorkerThread {
 public:
  WorkerThread(const std::string& threadName, Compiler::Action action,
               Compiler* compiler);
  ~WorkerThread();

  const std::string& Name() { return m_threadName; }

  bool start();
  bool stop();

 private:
  std::string m_threadName;
  Compiler::Action m_action = Compiler::Action::NoAction;
  std::thread* m_thread = nullptr;
  Compiler* m_compiler = nullptr;
};

class ThreadPool {
 public:
  static std::set<WorkerThread*> threads;
};

}  // namespace FOEDAG

#endif
