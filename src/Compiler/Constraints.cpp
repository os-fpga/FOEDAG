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

#include "Compiler/Constraints.h"

using namespace FOEDAG;

Constraints::Constraints() {
  m_interp = new TclInterpreter("");
  registerCommands(m_interp);
}

Constraints::~Constraints() {}

bool Constraints::evaluateConstraints(const std::filesystem::path& path) {
  m_interp->evalFile(path);
  return true;
}

bool Constraints::evaluateConstraint(const std::string& constraint) {
  m_interp->evalCmd(constraint);
  return true;
}

void Constraints::reset() {
  m_constraints.erase(m_constraints.begin(), m_constraints.end());
  m_keeps.erase(m_keeps.begin(), m_keeps.end());
}

static std::string getConstraint(uint64_t argc, const char* argv[]) {
  std::string command;
  for (uint64_t i = 0; i < argc; i++) {
    command += std::string(argv[i]) + " ";
  }
  return command;
}

void Constraints::registerCommands(TclInterpreter* interp) {
  // https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/tcl/Sdc.tcl
  // TODO: register all SDC commands, extract the "keeps"
  auto create_clock = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;

    if (argc < 2) {
      Tcl_AppendResult(interp, "ERROR: invalid syntax for create_clock",
                       (char*)NULL);
      return TCL_ERROR;
    }
    constraints->addConstraint(getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        constraints->addKeep(argv[i]);
      }
    }
    return 0;
  };
  interp->registerCmd("create_clock", create_clock, this, 0);

  // TODO: register all location constraints
}