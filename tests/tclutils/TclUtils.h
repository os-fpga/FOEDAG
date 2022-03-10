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
#pragma once

extern "C" {
#include <tcl.h>
}
#include <vector>

namespace FOEDAG::utils {

#define DISABLE_COPY_AND_ASSIGN(className)          \
 private:                                           \
  className() = default;                            \
  className(const className &) = delete;            \
  className(className &&) = delete;                 \
  className &operator=(const className &) = delete; \
  className &operator=(className &&) = delete;

class Command {
 public:
  Command(const char *name);
  virtual int command(void *clientData, Tcl_Interp *interp, int argc,
                      const char *argv[]) = 0;
  const char *name() const;
  static void registerAllcommands(Tcl_Interp *interp,
                                  void *clientData = nullptr);

 public:
  void *clientData;

 protected:
  const char *m_name;
  DISABLE_COPY_AND_ASSIGN(Command)
};

class CommandRegister {
 public:
  static CommandRegister &instance();
  std::vector<Command *> registeredCommands;

  DISABLE_COPY_AND_ASSIGN(CommandRegister)
};

void initCommandRegister();

#define COMMAND_NAME(name) COMMAND_##name
#define NAME_TO_STRING(n) #n

#define TCL_COMMAND(name)                                               \
  class COMMAND_NAME(name) : public FOEDAG::utils::Command {            \
   public:                                                              \
    COMMAND_NAME(name)() : Command(NAME_TO_STRING(name)) {}             \
    int command(void *clientData, Tcl_Interp *interp, int argc,         \
                const char *argv[]) override;                           \
    static COMMAND_NAME(name) * instance;                               \
  };                                                                    \
  COMMAND_NAME(name) * COMMAND_NAME(name)::instance =                   \
      new COMMAND_NAME(name)();                                         \
  int COMMAND_NAME(name)::command(void *clientData, Tcl_Interp *interp, \
                                  int argc, const char *argv[])

namespace internal {

void INIT_COMMAND(const char *name, Command *cmdPtr, Tcl_Interp *interpreter,
                  void *clientDataPtr);

}  // namespace internal

}  // namespace FOEDAG::utils
