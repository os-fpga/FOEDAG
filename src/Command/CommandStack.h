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
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/Logger.h"
#include "Tcl/TclInterpreter.h"

#ifndef COMMAND_STACK_H
#define COMMAND_STACK_H

namespace FOEDAG {

class CommandStack {
 private:
 public:
  CommandStack(TclInterpreter* interp,
               const std::string& logFile = std::string{}, bool mute = false);
  bool push_and_exec(Command* cmd);
  void push(Command* cmd);
  bool pop_and_undo();

  ~CommandStack();
  Logger* CmdLogger() { return m_logger; }
  Logger* PerfLogger() { return m_perfLogger; }
  Logger* OutLogger() { return m_outputLogger; }

 private:
  std::vector<Command*> m_cmds;
  TclInterpreter* m_interp = nullptr;
  Logger* m_logger = nullptr;
  Logger* m_perfLogger = nullptr;
  Logger* m_outputLogger = nullptr;
};

}  // namespace FOEDAG

#endif
