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

#include "TclUtils.h"

namespace FOEDAG::utils {

Command::Command(const char *name) : m_name(name) {
  CommandRegister::instance().registeredCommands.push_back(this);
}

const char *Command::name() const { return m_name; }

void Command::registerAllcommands(Tcl_Interp *interp, void *clientData) {
  for (auto cmd : CommandRegister::instance().registeredCommands) {
    internal::INIT_COMMAND(cmd->name(), cmd, interp, clientData);
  }
}

CommandRegister &CommandRegister::instance() {
  static CommandRegister runner;
  return runner;
}

void initCommandRegister() { CommandRegister::instance(); }

namespace internal {

void INIT_COMMAND(const char *name, Command *cmdPtr, Tcl_Interp *interpreter,
               void *clientDataPtr) {
  auto lambda = [](void *clientData, Tcl_Interp *interp, int argc,
                   const char *argv[]) -> int {
    Command *t = static_cast<Command *>(clientData);
    return t->command(t->clientData, interp, argc, argv);
  };
  cmdPtr->clientData = clientDataPtr;
  Tcl_CreateCommand(interpreter, name, lambda, reinterpret_cast<void *>(cmdPtr),
                    nullptr);
}

}  // namespace internal

}  // namespace FOEDAG::utils
