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
#ifndef TCL_INTERPRETER_H
#define TCL_INTERPRETER_H

#if defined(_MSC_VER)
#include <direct.h>
#include <process.h>
#else
#include <sys/param.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
extern "C" {
#include <tcl.h>
}

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct Tcl_Interp;

namespace FOEDAG {

class TclInterpreter {
 private:
  Tcl_Interp* interp;

 public:
  TclInterpreter(const char* argv0 = nullptr);

  ~TclInterpreter();

  std::string evalFile(const std::string& filename, int* ret = nullptr);

  std::string evalGuiTestFile(const std::string& filename);

  std::string evalCmd(const std::string cmd, int* ret = nullptr);

  void setResult(const std::string& result);

  typedef std::function<void()> TclCallback0;
  typedef std::function<void(const std::string& arg1)> TclCallback1;
  typedef std::function<void(const std::string& arg1, const std::string& arg2)>
      TclCallback2;

  void registerCmd(const std::string& cmdName, Tcl_CmdProc proc,
                   ClientData clientData, Tcl_CmdDeleteProc* deleteProc);

  Tcl_Interp* getInterp() { return interp; }

 private:
  std::string TclHistoryScript();
  std::string TclStackTrace(int code) const;
};

}  // namespace FOEDAG

#endif
