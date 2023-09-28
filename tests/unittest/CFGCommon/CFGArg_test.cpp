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

#include "Configuration/CFGCommon/CFGCommon.h"
#include "CFGArg_auto.h"
#include "gtest/gtest.h"

TEST(CFGArg, test_arg) {
  CFG_unset_callback_message_function();
  CFGArg_UTST arg;
  CFGArg_UTST_SUB0 sub0arg;
  EXPECT_EQ(sub0arg.debug, true);
  EXPECT_EQ(sub0arg.index.size(), 1);
  EXPECT_EQ(sub0arg.index[0], 3);
  EXPECT_EQ(sub0arg.cable, "usb");
  EXPECT_EQ(sub0arg.operation.size(), 0);
  EXPECT_EQ(sub0arg.target, "fpga");
  EXPECT_EQ(sub0arg.m_args.size(), 0);
  const char* argv[] = {"sub0",
                        "-d",
                        "in1",
                        "--index",
                        "-2x",
                        "in2",
                        "--cable=ethernet",
                        "-o",
                        "erase",
                        "-oprogram",
                        "--operation",
                        "verify",
                        "--operation=exam",
                        "in3",
                        "--target",
                        "flash",
                        "in4",
                        "--help=o",
                        "-h",
                        "--help",
                        "--help=many",
                        "-i"};
  std::vector<std::string> errors;
  bool status = arg.parse(int(sizeof(argv) / sizeof(argv[0])), argv, &errors);
  EXPECT_EQ(status, false);
  const CFGArg_UTST_SUB0* sub0arg_ptr =
      static_cast<const CFGArg_UTST_SUB0*>(arg.get_sub_arg());
  EXPECT_EQ(sub0arg_ptr->debug, false);
  EXPECT_EQ(sub0arg_ptr->index.size(), 1);
  EXPECT_EQ(sub0arg_ptr->index[0], 3);
  EXPECT_EQ(sub0arg_ptr->cable, "ethernet");
  EXPECT_EQ(sub0arg_ptr->operation.size(), 4);
  EXPECT_EQ(sub0arg_ptr->operation[0], "erase");
  EXPECT_EQ(sub0arg_ptr->operation[1], "program");
  EXPECT_EQ(sub0arg_ptr->operation[2], "verify");
  EXPECT_EQ(sub0arg_ptr->operation[3], "exam");
  EXPECT_EQ(sub0arg_ptr->target, "flash");
  EXPECT_EQ(sub0arg_ptr->m_args.size(), 4);
  EXPECT_EQ(sub0arg_ptr->m_args[0], "in1");
  EXPECT_EQ(sub0arg_ptr->m_args[1], "in2");
  EXPECT_EQ(sub0arg_ptr->m_args[2], "in3");
  EXPECT_EQ(sub0arg_ptr->m_args[3], "in4");
  EXPECT_EQ(errors.size(), 4);
  EXPECT_EQ(errors[0],
             "Fail to assign value -2x to option index because of invalid "
             "integer conversion");
  EXPECT_EQ(errors[1], "Not able to print help for invalid option many");
  EXPECT_EQ(errors[2], "Not enough input to assign option index");
  EXPECT_EQ(errors[3],
             "Can only specify maximum of 2 argument(s), but found 4 "
             "argument(s) is specified");
  arg.print();
}

TEST(CFGArg, test_list_device_ok) {
  CFGArg_PROGRAMMER_LIST_DEVICE arg;
  std::vector<std::string> errors;
  const char* argv[] = {"1", "-v"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_EQ(arg.m_args[0], "1");
}

TEST(CFGArg, test_list_device_exceed_max_arg) {
  CFGArg_PROGRAMMER_LIST_DEVICE arg;
  std::vector<std::string> errors;  
  const char* argv[] = {"Cable1", "Cable2"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_list_cable_ok) {
  CFGArg_PROGRAMMER_LIST_CABLE arg;
  std::vector<std::string> errors;
  const char* argv[] = {"-v"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
}

TEST(CFGArg, test_list_cable_exceed_max_arg) {
  CFGArg_PROGRAMMER_LIST_CABLE arg;
  std::vector<std::string> errors;
  const char* argv[] = {"dummy_arg, dummy_arg2"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_fpga_status_ok) {
  CFGArg_PROGRAMMER_FPGA_STATUS arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable1", "-d", "1"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_EQ(arg.cable, "cable1");
  EXPECT_EQ(arg.index, 1);
}

TEST(CFGArg, test_fpga_status_one_option_input) {
  CFGArg_PROGRAMMER_FPGA_STATUS arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-d", "2"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_EQ(errors.size(), 1);
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 2);
}

TEST(CFGArg, test_fpga_status_invalid_index) {
  CFGArg_PROGRAMMER_FPGA_STATUS arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable1", "-d", "not_a_number"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
  EXPECT_EQ(arg.cable, "cable1");
  EXPECT_EQ(arg.index, 1); // if error, it should return the default value
}

TEST(CFGArg, test_fpga_config_ok) {
  CFGArg_PROGRAMMER_FPGA_CONFIG arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "test.bit"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_EQ(arg.cable, "cable123");
  EXPECT_EQ(arg.m_args[0], "test.bit");
}

TEST(CFGArg, test_fpga_config_no_cable_option) {
  CFGArg_PROGRAMMER_FPGA_CONFIG arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-d", "2", "test.bit"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_fpga_config_no_arg) {
  CFGArg_PROGRAMMER_FPGA_CONFIG arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_fpga_config_empty_option_empty_arg) {
  CFGArg_PROGRAMMER_FPGA_CONFIG arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char** argv{nullptr};
  int argc = 0;
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_fpga_config_exceed_max_arg) {
  CFGArg_PROGRAMMER_FPGA_CONFIG arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "test.bit", "dummy_arg.bit"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_flash_ok) {
  CFGArg_PROGRAMMER_FLASH arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.operations, "program");
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "test.bit", "-o", "erase|program|verify"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_EQ(arg.cable, "cable123");
  EXPECT_EQ(arg.index, 2);
  EXPECT_EQ(arg.operations, "erase|program|verify");
  EXPECT_EQ(arg.m_args[0], "test.bit");
}

TEST(CFGArg, test_flash_no_cable_option) {
  CFGArg_PROGRAMMER_FLASH arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.operations, "program");
  std::vector<std::string> errors;
  const char* argv[] = {"-d", "2", "test.bit", "-o", "erase|program"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_flash_no_arg) {
  CFGArg_PROGRAMMER_FLASH arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.operations, "program");
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "-o", "erase|program|verify"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_flash_empty_option_empty_arg) {
  CFGArg_PROGRAMMER_FLASH arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.operations, "program");
  std::vector<std::string> errors;
  const char** argv{nullptr};
  int argc = 0;
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_flash_exceed_max_arg) {
  CFGArg_PROGRAMMER_FLASH arg;
  //test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.operations, "program");
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "test.bit", "extra_arg", "-o", "erase|program|verify"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, false);
  EXPECT_GE(errors.size(), 1);
}

TEST(CFGArg, test_otp_args) {
  CFGArg_PROGRAMMER_OTP arg;
  // test default cable and index value
  EXPECT_EQ(arg.cable, "1");
  EXPECT_EQ(arg.index, 1);
  EXPECT_EQ(arg.confirm, false);
  std::vector<std::string> errors;
  const char* argv[] = {"-c", "cable123", "-d", "2", "-y", "test.bit"};
  int argc = int(sizeof(argv) / sizeof(argv[0]));
  bool status = arg.parse(argc, argv, &errors);
  EXPECT_EQ(status, true);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_EQ(arg.cable, "cable123");
  EXPECT_EQ(arg.index, 2);
  EXPECT_EQ(arg.m_args[0], "test.bit");
  EXPECT_EQ(arg.confirm, true);
}
