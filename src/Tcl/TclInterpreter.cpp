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
#include "TclInterpreter.h"

using namespace FOEDAG;

#include <tcl.h>

TclInterpreter::TclInterpreter(const char *argv0) : interp(nullptr) {
  static bool initLib;
  if (!initLib) {
    Tcl_FindExecutable(argv0);
    initLib = true;
  }
  interp = Tcl_CreateInterp();
  if (!interp) throw new std::runtime_error("failed to initialise Tcl library");
}

TclInterpreter::~TclInterpreter() {
  if (interp) Tcl_DeleteInterp(interp);
}

std::string TclInterpreter::evalFile(const std::string &filename) {
  int code = Tcl_EvalFile(interp, filename.c_str());

  if (code >= TCL_ERROR) {
    return std::string("Tcl Error: " +
                       std::string(Tcl_GetStringResult(interp)));
  }
  return std::string(Tcl_GetStringResult(interp));
}

std::string TclInterpreter::evalCmd(const std::string cmd) {
  int code = Tcl_Eval(interp, cmd.c_str());

  if (code >= TCL_ERROR) {
    return std::string("Tcl Error: " +
                       std::string(Tcl_GetStringResult(interp)));
  }
  return std::string(Tcl_GetStringResult(interp));
}

void TclInterpreter::registerCmd(const std::string &cmdName, Tcl_CmdProc proc,
                                 ClientData clientData,
                                 Tcl_CmdDeleteProc *deleteProc) {
  Tcl_CreateCommand(interp, cmdName.c_str(), proc, clientData, deleteProc);
}

std::string TclInterpreter::evalGuiTestFile(const std::string &filename) {
  return "";
}