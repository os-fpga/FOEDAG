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

static QWidget* mainWindowBuilder(FOEDAG::Session* session) {
  return new FOEDAG::MainWindow{session};
}

static void registerExampleCommands(QWidget* widget, FOEDAG::Session* session) {
}

TEST(CompilerTCLCommonCodeUP, set_up) {
  FOEDAG::TclInterpreter*& interpreter = compiler_tcl_common_interpreter();
  FOEDAG::Compiler*& compiler = compiler_tcl_common_compiler();
  // Do it once
  ASSERT_EQ(interpreter, nullptr);
  ASSERT_EQ(compiler, nullptr);

  const int argc = 2;
  const char* argv[argc] = {"compiler_tcl_infra_common_code", "--batch"};
  FOEDAG::CommandLine* cmd =
      new FOEDAG::CommandLine(argc, const_cast<char**>(&argv[0]));
  cmd->processArgs();

  // Minimum setup that I am aware of
  // Add more stuff if neccessary
  interpreter = new FOEDAG::TclInterpreter("batchInterp");
  ASSERT_NE(interpreter, nullptr);

  FOEDAG::TaskManager* taskManager = new FOEDAG::TaskManager(nullptr);
  compiler = new FOEDAG::Compiler(interpreter, &std::cout);
  ASSERT_NE(compiler, nullptr);

  compiler->setTaskManager(taskManager);
  compiler->RegisterCommands(compiler->TclInterp(), true);
}
