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

void test_get_rid_whitespace() {
  CFG_POST_MSG("test_get_rid_whitespace");
  std::string str = "\t  \t \r \n \tThis is Love\t \r \n     \n";
  CFG_get_rid_trailing_whitespace(str, {'\t'});
  CFG_ASSERT(str == "\t  \t \r \n \tThis is Love\t \r \n     \n");
  CFG_get_rid_trailing_whitespace(str, {' ', '\n'});
  CFG_ASSERT(str == "\t  \t \r \n \tThis is Love\t \r");
  CFG_get_rid_leading_whitespace(str, {'\t', ' '});
  CFG_ASSERT(str == "\r \n \tThis is Love\t \r");
  CFG_get_rid_whitespace(str);
  CFG_ASSERT(str == "This is Love");
}

void test_string_case_conversion() {
  CFG_POST_MSG("test_string_case_conversion");
  std::string str = "thiS is me";
  CFG_ASSERT(CFG_string_toupper(str) == "THIS IS ME");
  CFG_ASSERT(str == "THIS IS ME");
  CFG_ASSERT(CFG_string_tolower(str) == "this is me");
  CFG_ASSERT(str == "this is me");
}

void test_string_to_u64_conversion() {
  CFG_POST_MSG("test_string_to_u64_conversion");
  bool status = true;
  uint64_t init = 123;
  CFG_ASSERT(CFG_convert_string_to_u64("", false, &status) == 0);
  CFG_ASSERT(status == true);
  CFG_ASSERT(CFG_convert_string_to_u64("", false, nullptr, &init) == 123);
  init = 789;
  CFG_ASSERT(CFG_convert_string_to_u64("joqekdvmpq", false, &status, &init) ==
             789);
  CFG_ASSERT(status == false);
  CFG_ASSERT(CFG_convert_string_to_u64("0x123") == 0x123);
  status = true;
  CFG_ASSERT(CFG_convert_string_to_u64("0x123 << 1", true, &status, nullptr,
                                       false) == 0);
  CFG_ASSERT(status == false);
  status = true;
  CFG_ASSERT(CFG_convert_string_to_u64("0x123 << 1", true, &status, nullptr,
                                       true) == 0x246);
  CFG_ASSERT(status == true);
  CFG_ASSERT(CFG_convert_string_to_u64("0x123 >> 4", true, &status, nullptr,
                                       true) == 0x12);
  CFG_ASSERT(status == true);
}

void test_find_item_in_vector() {
  CFG_POST_MSG("test_find_item_in_vector");
  CFG_ASSERT(
      CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "she") == -1);
  CFG_ASSERT(
      CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "you") == 1);
  CFG_ASSERT(
      CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "he") == 4);
  CFG_ASSERT(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 5) == -1);
  CFG_ASSERT(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 3) == 3);
  CFG_ASSERT(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 0) == 0);
}

void test_string_split() {
  CFG_POST_MSG("test_string_split");
  std::string string = "I am*#You*#*#*#HEis*#";
  std::vector<std::string> results = CFG_split_string(string, "*#");
  CFG_ASSERT(results.size() == 6);
  CFG_ASSERT(results[0] == "I am");
  CFG_ASSERT(results[1] == "You");
  CFG_ASSERT(results[2] == "");
  CFG_ASSERT(results[3] == "");
  CFG_ASSERT(results[4] == "HEis");
  CFG_ASSERT(results[5] == "");
  results = CFG_split_string(string, "*#", 0, false);
  CFG_ASSERT(results.size() == 3);
  CFG_ASSERT(results[0] == "I am");
  CFG_ASSERT(results[1] == "You");
  CFG_ASSERT(results[2] == "HEis");
  results = CFG_split_string(string, "*#", 3, true);
  CFG_ASSERT(results.size() == 4);
  CFG_ASSERT(results[0] == "I am");
  CFG_ASSERT(results[1] == "You");
  CFG_ASSERT(results[2] == "");
  CFG_ASSERT(results[3] == "*#HEis*#");
  results = CFG_split_string(string, "*#", 2, false);
  CFG_ASSERT(results.size() == 3);
  CFG_ASSERT(results[0] == "I am");
  CFG_ASSERT(results[1] == "You");
  CFG_ASSERT(results[2] == "*#*#HEis*#");
  results = CFG_split_string(string, "*#", 3, false);
  CFG_ASSERT(results.size() == 3);
  CFG_ASSERT(results[0] == "I am");
  CFG_ASSERT(results[1] == "You");
  CFG_ASSERT(results[2] == "HEis");
  results = CFG_split_string(string, "!");
  CFG_ASSERT(results.size() == 1);
  CFG_ASSERT(results[0] == "I am*#You*#*#*#HEis*#");
}

int main(int argc, const char** argv) {
  CFG_POST_MSG("This is CFGCommon unit test");
  test_arg();
  test_programmer_short_option_ok();
  test_program_device_long_option_ok();
  test_get_rid_whitespace();
  test_string_case_conversion();
  test_string_to_u64_conversion();
  test_find_item_in_vector();
  test_string_split();
  return 0;
}
