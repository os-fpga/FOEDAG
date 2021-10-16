/*
Copyright 2021 The Foedag team

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "CommandStack.h"

CommandStack::CommandStack(TclInterpreter *interp) : m_interp(interp) {}

bool CommandStack::push_and_exec(Command *cmd) {
  const std::string &result = m_interp->evalCmd(cmd->do_cmd());
  m_cmds.push_back(cmd);
  return (result == "");
}

bool CommandStack::pop_and_undo() {
  if (!m_cmds.empty()) {
    Command *c = m_cmds.back();
    const std::string &result = m_interp->evalCmd(c->undo_cmd());
    m_cmds.pop_back();
    return (result == "");
  }
  return false;
}

CommandStack::~CommandStack() {}
