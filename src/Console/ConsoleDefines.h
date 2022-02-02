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
using TclInterp = Tcl_Interp;

namespace FOEDAG {
static const Tcl_ChannelTypeVersion CHANNEL_VERSION_5 =
    reinterpret_cast<Tcl_ChannelTypeVersion>(TCL_CHANNEL_VERSION_5);

int TclEval(TclInterp *interp, const char *cmd);
const char *TclGetStringResult(TclInterp *interp);
const char *TclGetString(Tcl_Obj *obj);
void TclAppendResult(TclInterp *interp, const char *str);
}  // namespace FOEDAG
