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

#include "CommandStack.h"

#include <chrono>
#include <ctime>

using namespace FOEDAG;

CommandStack::CommandStack(TclInterpreter *interp, const std::string &logFile)
    : m_interp(interp) {
  m_logger = new Logger(logFile.empty() ? "cmd.log" : logFile);
  m_logger->open();
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  (*m_logger) << "# Command log file\n";
  (*m_logger) << "# Created: " << std::ctime(&time) << "\n";
}

bool CommandStack::push_and_exec(Command *cmd) {
  m_logger->log(cmd->do_cmd());
  const std::string &result = m_interp->evalCmd(cmd->do_cmd());
  m_cmds.push_back(cmd);
  return (result == "");
}

void CommandStack::push(Command *cmd) {
  m_logger->log(cmd->do_cmd());
  m_cmds.push_back(cmd);
}

bool CommandStack::pop_and_undo() {
  if (!m_cmds.empty()) {
    Command *c = m_cmds.back();
    m_logger->log(c->undo_cmd());
    const std::string &result = m_interp->evalCmd(c->undo_cmd());
    m_cmds.pop_back();
    return (result == "");
  }
  return false;
}

CommandStack::~CommandStack() {
  delete m_logger;
  for (auto cmd : m_cmds) delete cmd;
}
