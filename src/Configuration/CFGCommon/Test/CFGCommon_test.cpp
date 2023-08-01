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

#include "CFGCommon.h"

#include "CFGArg_auto.h"

// this test is not meant for automated. It is for manual testing
void test_programmer_short_option_ok() {
  CFG_POST_MSG("test_programmer_short_option_ok");
  CFGArg_PROGRAMMER arg;
  std::vector<std::string> errors;
  CFG_ASSERT(arg.config == "gemini.cfg");
  CFG_ASSERT(arg.index == 0);
  CFG_ASSERT(arg.m_args.size() == 0);
  const char* argv[] = {"flash", "test.bit", "-o", "erase|program"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  CFG_ASSERT(status);
  CFG_ASSERT(arg.config == "gemini.cfg");
  CFG_ASSERT(arg.index == 0);
  CFG_ASSERT(arg.operations == "erase|program");
  CFG_ASSERT(arg.m_args[0] == "flash");
  CFG_ASSERT(arg.m_args[1] == "test.bit");
  arg.print();
}

void test_program_device_long_option_ok() {
  CFG_POST_MSG("test_program_device_long_option_ok");
  CFGArg_PROGRAMMER arg;
  std::vector<std::string> errors;
  CFG_ASSERT(arg.config == "gemini.cfg");
  CFG_ASSERT(arg.index == 0);
  CFG_ASSERT(arg.m_args.size() == 0);
  const char* argv[] = {"flash",        "test.bit",
                        "--operations", "erase,blankcheck,program",
                        "-c",           "gemini.cfg"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  CFG_ASSERT(status);
  CFG_ASSERT(arg.config == "gemini.cfg");
  CFG_ASSERT(arg.index == 0);
  CFG_ASSERT(arg.operations == "erase,blankcheck,program");
  CFG_ASSERT(arg.m_args[0] == "flash");
  CFG_ASSERT(arg.m_args[1] == "test.bit");
  arg.print();
}

int main(int argc, const char** argv) {
  CFG_POST_MSG("This is CFGCommon unit test");
  test_programmer_short_option_ok();
  test_program_device_long_option_ok();
  return 0;
}
