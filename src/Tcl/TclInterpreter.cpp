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

#include <QString>
#include <QSysInfo>

using namespace FOEDAG;

#include <tcl.h>

TclInterpreter::TclInterpreter(const char *argv0) : interp(nullptr) {
  static bool initLib;
  if (!initLib) {
    Tcl_FindExecutable(argv0);
    initLib = true;
  }
  interp = Tcl_CreateInterp();
  Tcl_Init(interp);
  if (!interp) throw new std::runtime_error("failed to initialise Tcl library");
  evalCmd(TclHistoryScript());
}

TclInterpreter::~TclInterpreter() {
  if (interp) Tcl_DeleteInterp(interp);
}

std::string TclInterpreter::evalFile(const std::string &filename, int *ret) {
  int code = Tcl_EvalFile(interp, filename.c_str());
  if (ret) *ret = code;

  if (code >= TCL_ERROR) {
    return TclStackTrace(code);
  }
  return std::string(Tcl_GetStringResult(interp));
}

std::string TclInterpreter::evalCmd(const std::string cmd, int *ret) {
  int code = Tcl_Eval(interp, cmd.c_str());
  if (ret) *ret = code;

  if (code >= TCL_ERROR) {
    return TclStackTrace(code);
  }
  return std::string(Tcl_GetStringResult(interp));
}

void TclInterpreter::registerCmd(const std::string &cmdName, Tcl_CmdProc proc,
                                 ClientData clientData,
                                 Tcl_CmdDeleteProc *deleteProc) {
  Tcl_CreateCommand(interp, cmdName.c_str(), proc, clientData, deleteProc);
}

std::string TclInterpreter::evalGuiTestFile(const std::string &filename) {
  QString testHarness = R"(
  proc test_harness { gui_script } {
    global CONT errorInfo
    set fid [open $gui_script]
    set content [read $fid]
    close $fid
    set errorInfo ""

    catch {
        
        # Schedule commands
        set lines [split $content "\n"]
        set time 500
        foreach line $lines {
            if {[regexp {^#} $line]} {
                continue
            }
            if {$line == ""} {
                continue
            }
            after $time $line 
            
            
            set time [expr $time + 500]
        }
    }
    
    # Schedule GUI exit
    set time [expr $time + 500]
    after $time "puts \"GUI EXIT\" ; flush stdout; set CONT 0"
    
    # Enter loop
    set CONT 1 
    puts TEST_LOOP_ENTERED
    flush stdout
    while {$CONT} {
        set a 0
        after 10 set a 1
        %1
        vwait a
        if {$errorInfo != ""} {
          puts $errorInfo
          exit 1
        }
    }
    puts TEST_LOOP_EXITED
    flush stdout
    if {$errorInfo != ""} {
        puts $errorInfo
        exit 1
    }
    
    puts "Tcl Exit" ; flush stdout
    tcl_exit
  }

  )";
  testHarness =
      testHarness.arg((QSysInfo::productType() == "centos") ? R"(
  # this delay for some reason fixes tests on CentOS but stuck on MAC
  after 20
)"
                                                            : QString());

  std::string call_test = "proc call_test { } {\n";
  call_test += "test_harness " + filename + "\n";
  call_test += "}\n";

  std::string completeScript = testHarness.toStdString() + "\n" + call_test;

  int code = Tcl_Eval(interp, completeScript.c_str());

  if (code >= TCL_ERROR) {
    return std::string("Tcl Error: " +
                       std::string(Tcl_GetStringResult(interp)));
  }
  return std::string(Tcl_GetStringResult(interp));
}

std::string TclInterpreter::TclStackTrace(int code) const {
  std::string output;
  Tcl_Obj *options = Tcl_GetReturnOptions(interp, code);
  Tcl_Obj *key = Tcl_NewStringObj("-errorinfo", -1);
  Tcl_Obj *stackTrace;
  Tcl_IncrRefCount(key);
  Tcl_DictObjGet(NULL, options, key, &stackTrace);
  Tcl_DecrRefCount(key);
  output = Tcl_GetString(stackTrace);
  Tcl_DecrRefCount(options);
  return output;
}
