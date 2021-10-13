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

#include <string.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <tcl.h>

class TclInterpreter {
private:
  Tcl_Interp *interp;
public:
  TclInterpreter(const char *argv0 = nullptr) : interp(nullptr) {
    static bool initLib;
    if (!initLib) {
      Tcl_FindExecutable(argv0);
      initLib = true;
    }
    interp = Tcl_CreateInterp();
    if (!interp) throw new std::runtime_error("failed to initialise Tcl library");
  }
  
  ~TclInterpreter() {
    if (interp) Tcl_DeleteInterp(interp);
  }
  
  std::string evalFile(const std::string &filename) {
    int code = Tcl_EvalFile(interp, filename.c_str());
    
    if (code >= TCL_ERROR) {
      
      throw Tcl_GetStringResult(interp);
    }
    return std::string(Tcl_GetStringResult(interp));
  }
  
  std::string evalCmd(const std::string cmd) {
    int code = Tcl_Eval(interp, cmd.c_str());
    
    if (code >= TCL_ERROR) {
      
      throw Tcl_GetStringResult(interp);
    }
    return std::string(Tcl_GetStringResult(interp));
  }
};


int main(int argc, const char** argv) {
  
  TclInterpreter interpreter(argv[0]);
  std::string result = interpreter.evalCmd("puts \"Hello Foedag, you have Tcl!\"");
  std::cout << result << '\n';
  
}
