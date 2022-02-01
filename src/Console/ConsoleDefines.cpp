/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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

#include "ConsoleDefines.h"

namespace FOEDAG {

int TclEval(TclInterp *interp, const char *cmd) {
  return Tcl_Eval(interp, cmd);
}

const char *TclGetStringResult(TclInterp *interp) {
  return Tcl_GetStringResult(interp);
}

const char *TclGetString(Tcl_Obj *obj) { return Tcl_GetString(obj); }

void TclAppendResult(TclInterp *interp, const char *str) {
  Tcl_AppendResult(interp, str, (char *)NULL);
}

}  // namespace FOEDAG
