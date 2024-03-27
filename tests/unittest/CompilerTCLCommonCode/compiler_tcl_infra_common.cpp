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

void create_unittest_directory(const std::string& feature) {
  std::string directory = CFG_print("utst/%s", feature.c_str());
  if (!std::filesystem::is_directory("utst")) {
    std::filesystem::create_directory("utst");
  }
  if (!std::filesystem::is_directory(directory)) {
    std::filesystem::create_directory(directory);
  }
}

void compare_unittest_file(bool binary, const std::string& file,
                           const std::string& feature,
                           const std::string& golden_dir, bool equal) {
  std::string test_file =
      CFG_print("utst/%s/%s", feature.c_str(), file.c_str());
  std::string golden_file =
      CFG_print("%s/%s", golden_dir.c_str(), file.c_str());
  bool status = false;
  if (binary) {
    status = CFG_compare_two_binary_files(test_file, golden_file);
  } else {
    status = CFG_compare_two_text_files(test_file, golden_file);
  }
  if (status != equal) {
    printf("Comparing unit test file failed: %s vs %s\n", test_file.c_str(),
           golden_file.c_str());
  }
  ASSERT_EQ(status, equal);
}
