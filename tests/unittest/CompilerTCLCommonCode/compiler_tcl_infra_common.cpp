/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

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

#include "compiler_tcl_infra_common.h"

static FOEDAG::TclInterpreter* m_compiler_tcl_common_interpreter = nullptr;
static FOEDAG::Compiler* m_compiler_tcl_common_compiler = nullptr;

FOEDAG::TclInterpreter*& compiler_tcl_common_interpreter() {
  return m_compiler_tcl_common_interpreter;
}

FOEDAG::Compiler*& compiler_tcl_common_compiler() {
  return m_compiler_tcl_common_compiler;
}

void compiler_tcl_common_run(const std::string& cmd, const int expected_status,
                             const std::string& expected_msg) {
  int status = -1;
  std::string msg = m_compiler_tcl_common_interpreter->evalCmd(cmd, &status);
  if (status != expected_status) {
    printf("TCL Return MSG: %s\n", msg.c_str());
  }
  ASSERT_EQ(status, expected_status);
#if 0
  ASSERT_EQ(msg, expected_msg);
#endif
}
