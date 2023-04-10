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

#include "CFGCommon.h"

#include <gtest/gtest.h>

#include "CFGArg_auto.h"
#include "gmock/gmock.h"

namespace {
TEST(CFGCommon, test_arg_default) {
  CFGArg_UTST arg;
  EXPECT_TRUE(arg.debug);
  EXPECT_EQ(arg.index.size(), 1);
  EXPECT_EQ(arg.index[0], 3);
  EXPECT_EQ(arg.cable, "usb");
  EXPECT_EQ(arg.operation.size(), 0);
  EXPECT_EQ(arg.target, "fpga");
  EXPECT_EQ(arg.m_args.size(), 0);
}

TEST(CFGCommon, test_arg_with_dummy_args) {
  CFGArg_UTST arg;
  const char* argv[] = {"-d",          "in1",         "--index",
                        "-2x",         "in2",         "--cable=ethernet",
                        "-o",          "erase",       "-oprogram",
                        "--operation", "verify",      "--operation=exam",
                        "in3",         "--target",    "flash",
                        "in4",         "--help=o",    "-h",
                        "--help",      "--help=many", "-i"};
  std::vector<std::string> errors;
  bool status = arg.parse(int(sizeof(argv) / sizeof(argv[0])), argv, &errors);
  EXPECT_FALSE(status);
  EXPECT_FALSE(arg.debug);
  EXPECT_EQ(arg.index.size(), 1);
  EXPECT_EQ(arg.index[0], 3);
  EXPECT_EQ(arg.cable, "ethernet");
  EXPECT_EQ(arg.operation.size(), 4);
  EXPECT_EQ(arg.operation[0], "erase");
  EXPECT_EQ(arg.operation[1], "program");
  EXPECT_EQ(arg.operation[2], "verify");
  EXPECT_EQ(arg.operation[3], "exam");
  EXPECT_EQ(arg.target, "flash");
  EXPECT_EQ(arg.m_args.size(), 4);
  EXPECT_EQ(arg.m_args[0], "in1");
  EXPECT_EQ(arg.m_args[1], "in2");
  EXPECT_EQ(arg.m_args[2], "in3");
  EXPECT_EQ(arg.m_args[3], "in4");
  EXPECT_EQ(errors.size(), 4);
  EXPECT_EQ(errors[0],
            "Fail to assign value -2x to option index because of invalid "
            "integer conversion");
  EXPECT_EQ(errors[1], "Not able to print help for invalid option many");
  EXPECT_EQ(errors[2], "Not enough input to assign option index");
  EXPECT_EQ(errors[3],
            "Can only specify maximum of 2 argument(s), but found 4 "
            "argument(s) is specified");
}

TEST(CFGCommon, test_program_device_arg) {
  CFGArg_PROGRAM_DEVICE arg;
  std::vector<std::string> errors;

  const char* argv[] = {
      "programmer", "-b", "test.bit", "-c", "gemini.cfg", "--index", "2",
  };
  bool status = arg.parse(int(sizeof(argv) / sizeof(argv[0])), argv, &errors);
  EXPECT_TRUE(status);
  EXPECT_EQ(arg.config, "gemini.cfg");
  EXPECT_EQ(arg.index, 2);
  EXPECT_EQ(arg.bitstream, "test.bit");
}

TEST(CFGCommon, test_program_device_arg_default) {
  CFGArg_PROGRAM_DEVICE arg;
  EXPECT_EQ(arg.config, "");
  EXPECT_EQ(arg.index, 0);
  EXPECT_EQ(arg.bitstream, "");
  EXPECT_EQ(arg.m_args.size(), 0);
}

// <TODO> Add more tests

}  // end of anonymous namespace
