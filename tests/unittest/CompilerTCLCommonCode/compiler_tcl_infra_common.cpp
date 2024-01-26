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

static FOEDAG::TclInterpreter m_compiler_tcl_common_interpreter("batchInterp");
static FOEDAG::TaskManager* m_compiler_tcl_common_taskManager =
    new FOEDAG::TaskManager(nullptr);
static FOEDAG::Compiler m_compiler_tcl_common_compiler(
    &m_compiler_tcl_common_interpreter, &std::cout);
static FOEDAG::CFGCompiler m_compiler_tcl_common_cfgcompiler(
    &m_compiler_tcl_common_compiler);
static int64_t m_compiler_tcl_common_counter = 0;

static QWidget* mainWindowBuilder(FOEDAG::Session* session) {
  return new FOEDAG::MainWindow{session};
}

static void registerExampleCommands(QWidget* widget, FOEDAG::Session* session) {
}

FOEDAG::Compiler* compiler_tcl_common_compiler() {
  compiler_tcl_common_setup();
  return &m_compiler_tcl_common_compiler;
}

FOEDAG::CFGCompiler* compiler_tcl_common_cfgcompiler() {
  compiler_tcl_common_setup();
  return &m_compiler_tcl_common_cfgcompiler;
}

void compiler_tcl_common_setup() {
  if (m_compiler_tcl_common_counter == 0) {
    printf("************************************\n");
    printf("*  compiler_tcl_common_setup\n");
    printf("************************************\n");

    // Minimum setup that I am aware of
    // Add more stuff if neccessary
    m_compiler_tcl_common_compiler.setTaskManager(
        m_compiler_tcl_common_taskManager);
    m_compiler_tcl_common_compiler.RegisterCommands(
        m_compiler_tcl_common_compiler.TclInterp(), true);
  }
  m_compiler_tcl_common_counter++;
}

void compiler_tcl_common_run(const std::string& cmd, const int expected_status,
                             const std::string& expected_msg) {
  CFG_ASSERT(m_compiler_tcl_common_counter > 0);
  int status = -1;
  std::string msg = m_compiler_tcl_common_interpreter.evalCmd(cmd, &status);
  if (status != expected_status) {
    printf("TCL Return MSG: %s\n", msg.c_str());
  }
  ASSERT_EQ(status, expected_status);
#if 0
  ASSERT_EQ(msg, expected_msg);
#endif
}
