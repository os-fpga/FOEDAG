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
#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <QEventLoop>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Compiler/Compiler.h"
#include "Main/CommandLine.h"
#include "Tcl/TclInterpreter.h"

namespace FOEDAG {

class WorkerThread {
 public:
  WorkerThread(const std::string& threadName, Compiler::Action action,
               Compiler* compiler,
               const std::function<void(int)>& postRunTask = {});
  ~WorkerThread();

  const std::string& Name() { return m_threadName; }

  bool start();
  bool stop();

  /*!
   * \brief Start
   * Run any callback in thread
   * \param fn - callback function
   * \param args - callback arguments
   * \return value from the callback
   */
  template <typename Func, typename... Args>
  bool Start(const Func& fn, Args&&... args) {
    bool result = true;
    m_compiler->start();
    QEventLoop* eventLoop{nullptr};
    const bool processEvents = isGui();
    if (processEvents) eventLoop = new QEventLoop;
    m_thread =
        // pack args as tuple for capturing
        new std::thread([&, args = std::make_tuple(std::forward<Args>(args)...),
                         eventLoop]() mutable {
          // pass arguments to callback
          std::apply([&result, fn](auto&&... args) { result = fn(args...); },
                     std::move(args));
          if (eventLoop) eventLoop->quit();
        });
    if (eventLoop)
      eventLoop->exec();
    else
      m_thread->join();  // batch mode
    m_compiler->finish();
    delete eventLoop;
    return result;
  }

 private:
  bool isGui() const;

 private:
  std::string m_threadName;
  Compiler::Action m_action = Compiler::Action::NoAction;
  std::thread* m_thread = nullptr;
  Compiler* m_compiler = nullptr;
  const std::function<void(int)>& m_postRunTask{};
};

class ThreadPool {
 public:
  static std::set<WorkerThread*> threads;
};

}  // namespace FOEDAG

#endif
