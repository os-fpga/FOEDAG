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
#pragma once

#include "Compiler/Compiler.h"
#include "Compiler/TaskManager.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "Configuration/CFGCommon/CFGCommon.h"
#include "Configuration/CFGCompiler/CFGCompiler.h"
#include "Main/CommandLine.h"
#include "Main/Foedag.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "gtest/gtest.h"

inline std::string compiler_tcl_common_get_current_dir(
    const std::string& filepath) {
  std::string current_dir = CFG_change_directory_to_linux_format(filepath);
  size_t index = current_dir.rfind("/");
  CFG_ASSERT(index != std::string::npos);
  current_dir = current_dir.substr(0, index);
  return current_dir;
}
#define COMPILER_TCL_COMMON_GET_CURRENT_DIR() \
  compiler_tcl_common_get_current_dir(__FILE__)

FOEDAG::Compiler* compiler_tcl_common_compiler();
FOEDAG::CFGCompiler* compiler_tcl_common_cfgcompiler();
void compiler_tcl_common_setup();
void compiler_tcl_common_run(const std::string& cmd,
                             const int expected_status = 0,
                             const std::string& expected_msg = "");
