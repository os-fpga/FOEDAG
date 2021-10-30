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

#ifndef TCL_INTERPRETER_H
#define TCL_INTERPRETER_H

struct Tcl_Interp;

namespace FOEDAG {

class TclInterpreter {
 private:
  Tcl_Interp* interp;

 public:
  TclInterpreter(const char* argv0 = nullptr);

  ~TclInterpreter();

  std::string evalFile(const std::string& filename);

  std::string evalGuiTestFile(const std::string& filename);

  std::string evalCmd(const std::string cmd);

  typedef std::function<void()> TclCallback0;
  typedef std::function<void(const std::string& arg1)> TclCallback1;
  typedef std::function<void(const std::string& arg1, const std::string& arg2)>
      TclCallback2;

  void registerCmd(const std::string& cmdName, Tcl_CmdProc proc,
                   ClientData clientData, Tcl_CmdDeleteProc* deleteProc);

  Tcl_Interp* getInterp() { return interp; }
};

}  // namespace FOEDAG

#endif
