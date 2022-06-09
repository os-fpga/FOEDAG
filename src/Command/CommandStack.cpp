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

CommandStack::CommandStack(TclInterpreter *interp, const std::string &logFile,
                           bool mute)
    : m_interp(interp) {
  if (!mute) {
    m_logger = new Logger(logFile.empty() ? "cmd.tcl" : logFile + "_cmd.tcl");
    m_logger->open();
    std::time_t result = std::time(nullptr);
    (*m_logger) << "# Command log file\n";
    (*m_logger) << "# Created: " << std::ctime(&result) << "\n";

    m_perfLogger =
        new Logger(logFile.empty() ? "perf.log" : logFile + "_perf.log");
    m_perfLogger->open();
    (*m_perfLogger) << "# Perf log file\n";
    (*m_perfLogger) << "# Created: " << std::ctime(&result) << "\n";

    m_outputLogger = new Logger(logFile.empty() ? "out.log" : logFile + ".log");
    m_outputLogger->open();
    (*m_outputLogger) << "# Out log file\n";
    (*m_outputLogger) << "# Created: " << std::ctime(&result) << "\n";
  }
}

bool CommandStack::push_and_exec(Command *cmd) {
  if (m_logger) m_logger->log(cmd->do_cmd());
  const std::string &result = m_interp->evalCmd(cmd->do_cmd());
  m_cmds.push_back(cmd);
  return (result == "");
}

void CommandStack::push(Command *cmd) {
  if (m_logger) m_logger->log(cmd->do_cmd());
  m_cmds.push_back(cmd);
}

bool CommandStack::pop_and_undo() {
  if (!m_cmds.empty()) {
    Command *c = m_cmds.back();
    if (m_logger) m_logger->log(c->undo_cmd());
    const std::string &result = m_interp->evalCmd(c->undo_cmd());
    m_cmds.pop_back();
    return (result == "");
  }
  return false;
}

CommandStack::~CommandStack() {
  delete m_logger;
  delete m_perfLogger;
  delete m_outputLogger;
  for (auto cmd : m_cmds) delete cmd;
}
