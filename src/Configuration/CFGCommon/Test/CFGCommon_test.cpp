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

void test_arg() {
  CFG_POST_MSG("This is CFGArg unit test");
  CFGArg_UTST arg;
  CFG_ASSERT(arg.debug == true);
  CFG_ASSERT(arg.index.size() == 1);
  CFG_ASSERT(arg.index[0] == 3);
  CFG_ASSERT(arg.cable == "usb");
  CFG_ASSERT(arg.operation.size() == 0);
  CFG_ASSERT(arg.target == "fpga");
  CFG_ASSERT(arg.m_args.size() == 0);
  const char* argv[] = {"-d",          "in1",         "--index",
                        "-2x",         "in2",         "--cable=ethernet",
                        "-o",          "erase",       "-oprogram",
                        "--operation", "verify",      "--operation=exam",
                        "in3",         "--target",    "flash",
                        "in4",         "--help=o",    "-h",
                        "--help",      "--help=many", "-i"};
  std::vector<std::string> errors;
  bool status = arg.parse(int(sizeof(argv) / sizeof(argv[0])), argv, &errors);
  CFG_ASSERT(!status);
  CFG_ASSERT(arg.debug == false);
  CFG_ASSERT(arg.index.size() == 1);
  CFG_ASSERT(arg.index[0] == 3);
  CFG_ASSERT(arg.cable == "ethernet");
  CFG_ASSERT(arg.operation.size() == 4);
  CFG_ASSERT(arg.operation[0] == "erase");
  CFG_ASSERT(arg.operation[1] == "program");
  CFG_ASSERT(arg.operation[2] == "verify");
  CFG_ASSERT(arg.operation[3] == "exam");
  CFG_ASSERT(arg.target == "flash");
  CFG_ASSERT(arg.m_args.size() == 4);
  CFG_ASSERT(arg.m_args[0] == "in1");
  CFG_ASSERT(arg.m_args[1] == "in2");
  CFG_ASSERT(arg.m_args[2] == "in3");
  CFG_ASSERT(arg.m_args[3] == "in4");
  CFG_ASSERT(errors.size() == 4);
  CFG_ASSERT(errors[0] ==
             "Fail to assign value -2x to option index because of invalid "
             "integer conversion");
  CFG_ASSERT(errors[1] == "Not able to print help for invalid option many");
  CFG_ASSERT(errors[2] == "Not enough input to assign option index");
  CFG_ASSERT(errors[3] ==
             "Can only specify maximum of 2 argument(s), but found 4 "
             "argument(s) is specified");
  arg.print();
}

void test_program_device_arg() {
  CFG_POST_MSG("test_program_device_arg unit test");
  CFGArg_PROGRAM_DEVICE arg;
  std::vector<std::string> errors;
  CFG_ASSERT(arg.config == "");
  CFG_ASSERT(arg.index == 0);
  CFG_ASSERT(arg.bitstream == "");
  CFG_ASSERT(arg.m_args.size() == 0);
  const char* argv[] = {
      "programmer", "-b", "test.bit", "-c", "gemini.cfg", "--index", "2",
  };
  bool status = arg.parse(int(sizeof(argv) / sizeof(argv[0])), argv, &errors);
  CFG_ASSERT(status);
  CFG_ASSERT(arg.config == "gemini.cfg");
  CFG_ASSERT(arg.index == 2);
  CFG_ASSERT(arg.bitstream == "test.bit");
  arg.print();
}

int main(int argc, const char** argv) {
  CFG_POST_MSG("This is CFGCommon unit test");
  test_arg();
  test_program_device_arg();
  return 0;
}
