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

#include "compiler_tcl_infra_common.h"
#include "gtest/gtest.h"

TEST(CFGCommon, test_replace_string) {
  std::string original = "This is ProgrammerConfiguration\t\t     \t tool";
  std::string result = CFG_replace_string(original, "Programmer", "");
  EXPECT_EQ(result, "This is Configuration\t\t     \t tool");
  result = CFG_replace_string(original, "Programmer", "FOEDAG ");
  EXPECT_EQ(result, "This is FOEDAG Configuration\t\t     \t tool");
  result = CFG_replace_string(original, "\t", " ", false);
  EXPECT_EQ(result, "This is ProgrammerConfiguration         tool");
  result = CFG_replace_string(original, "\t", " ", true);
  EXPECT_EQ(result, "This is ProgrammerConfiguration tool");
}

TEST(CFGCommon, test_change_directory_to_linux_format) {
  std::string linux_format = CFG_change_directory_to_linux_format(
      "c:\\\\\\ abc \\ efg /////xyz //123");
  EXPECT_EQ(linux_format, "c:/ abc / efg /xyz /123");
}

TEST(CFGCommon, test_configuration_relative_path) {
  std::string path =
      CFG_get_configuration_relative_path("/usr/src/ConfigurationRS/abc");
  EXPECT_EQ(path, "ConfigurationRS/abc");
}

TEST(CFGCommon, test_get_rid_whitespace) {
  std::string str = "\t  \t \r \n \tThis is Love\t \r \n     \n";
  CFG_get_rid_trailing_whitespace(str, {'\t'});
  EXPECT_EQ(str, "\t  \t \r \n \tThis is Love\t \r \n     \n");
  CFG_get_rid_trailing_whitespace(str, {' ', '\n'});
  EXPECT_EQ(str, "\t  \t \r \n \tThis is Love\t \r");
  CFG_get_rid_leading_whitespace(str, {'\t', ' '});
  EXPECT_EQ(str, "\r \n \tThis is Love\t \r");
  CFG_get_rid_whitespace(str);
  EXPECT_EQ(str, "This is Love");
}

TEST(CFGCommon, test_string_case_conversion) {
  std::string str = "thiS is me";
  EXPECT_EQ(CFG_string_toupper(str), "THIS IS ME");
  EXPECT_EQ(str, "THIS IS ME");
  EXPECT_EQ(CFG_string_tolower(str), "this is me");
  EXPECT_EQ(str, "this is me");
}

TEST(CFGCommon, test_string_to_u64_conversion) {
  bool status = true;
  uint64_t init = 123;
  // Empty
  EXPECT_EQ(CFG_convert_string_to_u64("", false, &status), 0);
  EXPECT_EQ(status, true);
  EXPECT_EQ(CFG_convert_string_to_u64("", false, nullptr, &init), 123);
  // Decimal
  EXPECT_EQ(CFG_convert_string_to_u64("123456789"), 123456789);
  EXPECT_EQ((int)(CFG_convert_string_to_u64("-12")), -12);
  // Binary
  EXPECT_EQ(CFG_convert_string_to_u64("b000111100"), 0x3C);
  EXPECT_EQ((int)(CFG_convert_string_to_u64("-b11")), -3);
  // Hex
  EXPECT_EQ(CFG_convert_string_to_u64("0x123"), 0x123);
  EXPECT_EQ((int)(CFG_convert_string_to_u64("-0xFF")), -255);
  // HDL Binary
  EXPECT_EQ(CFG_convert_string_to_u64("'b000111100"), 0x3C);
  EXPECT_EQ(CFG_convert_string_to_u64("4'b000111100"), 0xC);
  // HDL Decimal
  EXPECT_EQ(CFG_convert_string_to_u64("'d12345"), 12345);
  EXPECT_EQ(CFG_convert_string_to_u64("4'd12345"), 9);
  // HDL Hex
  EXPECT_EQ(CFG_convert_string_to_u64("'hABCDEF"), 0xABCDEF);
  EXPECT_EQ(CFG_convert_string_to_u64("12'hABCDEF"), 0xDEF);
  // Shift Left
  EXPECT_EQ(CFG_convert_string_to_u64("0x123 << 1"), 0x246);
  // Shift Right
  EXPECT_EQ(CFG_convert_string_to_u64("0x123 >> 4"), 0x12);
  // Mix Shift
  EXPECT_EQ(CFG_convert_string_to_u64("0x123 << 5 >> 3 << 1 >> 2"), 0x246);
  // Mix Shift
  EXPECT_EQ(CFG_convert_string_to_u64("0x123 >> 4 << 2"), 0x48);
  /*
    Invalid Test
  */
  // Decimal
  init = 789;
  EXPECT_EQ(CFG_convert_string_to_u64("joqekdvmpq", false, &status, &init),
            789);
  EXPECT_EQ(status, false);
  // Binary
  EXPECT_EQ(CFG_convert_string_to_u64("b000111102", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("b", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("b11111111111111111111111111111111111111"
                                      "111111111111111111111111111111111111111"
                                      "1111111111111111111111111111111111",
                                      false, &status),
            0);
  EXPECT_EQ(status, false);
  status = true;
  // Hex
  EXPECT_EQ(CFG_convert_string_to_u64("0x123z", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("0x", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("0x111111111111111111111111111111111",
                                      false, &status),
            0);
  EXPECT_EQ(status, false);
  status = true;
  // HDL Binary
  EXPECT_EQ(CFG_convert_string_to_u64("'b00011110a", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("'b", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("'b11111111111111111111111111111111111111"
                                      "111111111111111111111111111111111111111"
                                      "1111111111111111111111111111111111",
                                      false, &status),
            0);
  EXPECT_EQ(status, false);
  status = true;
  // HDL Decimal
  EXPECT_EQ(CFG_convert_string_to_u64("'dab", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("'d", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  // HDL Hex
  EXPECT_EQ(CFG_convert_string_to_u64("'hxy", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("'h", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("'hCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", false,
                                      &status),
            0);
  EXPECT_EQ(status, false);
  status = true;
  // Shift
  EXPECT_EQ(CFG_convert_string_to_u64(">>", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64("<<", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" >>", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" <<", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" >> ", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" << ", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" ab >> 1 ", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  EXPECT_EQ(CFG_convert_string_to_u64(" 1 << ab", false, &status), 0);
  EXPECT_EQ(status, false);
  status = true;
  // CFG_INTERNAL_ERROR("stop");
}

TEST(CFGCommon, test_find_item_in_vector) {
  EXPECT_EQ(CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "she"),
            -1);
  EXPECT_EQ(CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "you"),
            1);
  EXPECT_EQ(CFG_find_string_in_vector({"I", "you", "we", "they", "he"}, "he"),
            4);
  EXPECT_EQ(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 5), -1);
  EXPECT_EQ(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 3), 3);
  EXPECT_EQ(CFG_find_u32_in_vector({0, 1, 2, 3, 4}, 0), 0);
}

TEST(CFGCommon, test_string_split) {
  std::string string = "I am*#You*#*#*#HEis*#";
  std::vector<std::string> results = CFG_split_string(string, "*#");
  EXPECT_EQ(results.size(), 6);
  EXPECT_EQ(results[0], "I am");
  EXPECT_EQ(results[1], "You");
  EXPECT_EQ(results[2], "");
  EXPECT_EQ(results[3], "");
  EXPECT_EQ(results[4], "HEis");
  EXPECT_EQ(results[5], "");
  results = CFG_split_string(string, "*#", 0, false);
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], "I am");
  EXPECT_EQ(results[1], "You");
  EXPECT_EQ(results[2], "HEis");
  results = CFG_split_string(string, "*#", 3, true);
  EXPECT_EQ(results.size(), 4);
  EXPECT_EQ(results[0], "I am");
  EXPECT_EQ(results[1], "You");
  EXPECT_EQ(results[2], "");
  EXPECT_EQ(results[3], "*#HEis*#");
  results = CFG_split_string(string, "*#", 2, false);
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], "I am");
  EXPECT_EQ(results[1], "You");
  EXPECT_EQ(results[2], "*#*#HEis*#");
  results = CFG_split_string(string, "*#", 3, false);
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], "I am");
  EXPECT_EQ(results[1], "You");
  EXPECT_EQ(results[2], "HEis");
  results = CFG_split_string(string, "!");
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0], "I am*#You*#*#*#HEis*#");

  results.clear();
  string = "aa,bb,cc,dd, ee";
  results = CFG_split_string(string, ",");
  EXPECT_EQ(results.size(), 5);
  EXPECT_EQ(results[0], "aa");
  EXPECT_EQ(results[1], "bb");
  EXPECT_EQ(results[2], "cc");
  EXPECT_EQ(results[3], "dd");
  EXPECT_EQ(results[4], " ee");
}

TEST(CFGCommon, test_join_strings) {
  std::vector<std::string> strings = {"", "", "a", "", "b", "c"};
  std::string result = CFG_join_strings(strings, ";", true);
  EXPECT_EQ(result, ";;a;;b;c");
  result = CFG_join_strings(strings, " : ", false);
  EXPECT_EQ(result, "a : b : c");
}

TEST(CFGCommon, test_exception) {
  int a = 10;
  int exception_count = 0;
  try {
    CFG_ASSERT(0 > a);
    FAIL() << "CFG_ASSERT() should throw an error\n";
  } catch (std::exception& e) {
    EXPECT_EQ("0 > a", CFG_print("%s", e.what()));
    exception_count++;
  }
  try {
    CFG_ASSERT_MSG(0 > 10, "This is invalid comparision - %d > %d", 0, a);
    FAIL() << "CFG_ASSERT_MSG() should throw an error\n";
  } catch (std::exception& e) {
    EXPECT_EQ("This is invalid comparision - 0 > 10",
              CFG_print("%s", e.what()));
    exception_count++;
  }
  try {
    CFG_INTERNAL_ERROR("This is internal error %s", "from CFGCommon UnitTest");
    FAIL() << "CFG_INTERNAL_ERROR() should throw an error\n";
  } catch (std::exception& e) {
    EXPECT_EQ("This is internal error from CFGCommon UnitTest",
              CFG_print("%s", e.what()));
    exception_count++;
  }
  EXPECT_EQ(exception_count, 3);
}

TEST(CFGCommon, test_number_to_unit_string) {
  EXPECT_EQ(CFG_convert_number_to_unit_string(0), "0");
  EXPECT_EQ(CFG_convert_number_to_unit_string(1023), "1023");
  EXPECT_EQ(CFG_convert_number_to_unit_string(2048), "2K");
  EXPECT_EQ(CFG_convert_number_to_unit_string(1024 * 1024), "1M");
  EXPECT_EQ(CFG_convert_number_to_unit_string(1024ULL * 1024ULL * 1024ULL),
            "1G");
  EXPECT_EQ(
      CFG_convert_number_to_unit_string(1024ULL * 1024ULL * 1024ULL * 1024ULL),
      "1T");
  EXPECT_EQ(CFG_convert_number_to_unit_string(123456789), "123456789");
}

TEST(CFGCommon, test_python) {
  std::map<std::string, CFG_Python_OBJ> pobjs = CFG_Python(
      {"a=1", "b=3", "c=a+b", "d='%d'%(c*b)"}, {"a", "b", "c", "d", "e", "f"});
  EXPECT_EQ(pobjs.size(), 4);
  // Existence
  EXPECT_NE(pobjs.find("a"), pobjs.end());
  EXPECT_NE(pobjs.find("b"), pobjs.end());
  EXPECT_NE(pobjs.find("c"), pobjs.end());
  EXPECT_NE(pobjs.find("d"), pobjs.end());
  // Check value
  EXPECT_EQ(pobjs["a"].get_u32("a"), 1);
  EXPECT_EQ(pobjs["b"].get_u32("b"), 3);
  EXPECT_EQ(pobjs["c"].get_u32("c"), 4);
  EXPECT_EQ(pobjs["d"].get_str("d"), "12");
}

TEST(CFGCommon, test_python_mgr) {
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  CFG_Python_MGR mgr;
  mgr.run({"pins = []"}, {});
  EXPECT_EQ(mgr.results().size(), 0);
  mgr.run({"exist = 1 if 'a' in pins else 0", "pins.append('a')"}, {"exist"});
  EXPECT_EQ(mgr.results().size(), 1);
  EXPECT_EQ(mgr.result_u32("exist"), 0);
  mgr.run({"exist = 1 if 'a' in pins else 0", "pins.append('b')",
           "count = len(pins)"},
          {"exist", "count"});
  EXPECT_EQ(mgr.results().size(), 2);
  EXPECT_EQ(mgr.result_u32("exist"), 1);
  EXPECT_EQ(mgr.result_u32("count"), 2);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  mgr.run({"bytes = bytearray([0, 1, 2])", "u32s = [4, 5, 6]",
           "strs = ['abc', 'efg']", "str='xyz'", "bool0=3 in u32s",
           "bool1=4 in u32s"},
          {"exist", "count", "bytes", "u32s", "strs", "str", "hello", "bool0",
           "bool1"});
  EXPECT_EQ(mgr.results().size(), 8);
  EXPECT_EQ(mgr.result_u32("exist"), 1);
  EXPECT_EQ(mgr.result_u32("count"), 2);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::vector<uint8_t> bytes = mgr.result_bytes("bytes");
  EXPECT_EQ(bytes.size(), 3);
  EXPECT_EQ(bytes[0], 0);
  EXPECT_EQ(bytes[1], 1);
  EXPECT_EQ(bytes[2], 2);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::vector<uint32_t> u32s = mgr.result_u32s("u32s");
  EXPECT_EQ(u32s.size(), 3);
  EXPECT_EQ(u32s[0], 4);
  EXPECT_EQ(u32s[1], 5);
  EXPECT_EQ(u32s[2], 6);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::vector<std::string> strs = mgr.result_strs("strs");
  EXPECT_EQ(strs.size(), 2);
  EXPECT_EQ(strs[0], "abc");
  EXPECT_EQ(strs[1], "efg");
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(mgr.result_str("str"), "xyz");
  EXPECT_EQ(mgr.result_bool("bool0"), false);
  EXPECT_EQ(mgr.result_bool("bool1"), true);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(
      mgr.set_file(CFG_print("%s/python_file_test.py", current_dir.c_str())),
      "python_file_test");
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  std::vector<CFG_Python_OBJ> results = mgr.run_file(
      "python_file_test", "func1",
      std::vector<CFG_Python_OBJ>(
          {CFG_Python_OBJ(bool(1)), CFG_Python_OBJ(uint32_t(-1)),
           CFG_Python_OBJ(std::string("abc")),
           CFG_Python_OBJ(std::vector<uint8_t>({0, 1, 2})),
           CFG_Python_OBJ(std::vector<uint32_t>({10, (uint32_t)(-10)})),
           CFG_Python_OBJ(std::vector<std::string>({"X", "y", "Z"}))}));
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(results.size(), 7);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(results[0].type, CFG_Python_OBJ::TYPE::BOOL);
  EXPECT_EQ(results[0].get_bool(), false);
  EXPECT_EQ(results[1].type, CFG_Python_OBJ::TYPE::INT);
  EXPECT_EQ(results[1].get_u32(), uint32_t(-11));
  EXPECT_EQ(results[2].type, CFG_Python_OBJ::TYPE::STR);
  EXPECT_EQ(results[2].get_str(), "ABC");
  EXPECT_EQ(results[3].type, CFG_Python_OBJ::TYPE::BYTES);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  bytes = results[3].get_bytes();
  EXPECT_EQ(bytes.size(), 3);
  EXPECT_EQ(bytes[0], 10);
  EXPECT_EQ(bytes[1], 11);
  EXPECT_EQ(bytes[2], 12);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(results[4].type, CFG_Python_OBJ::TYPE::INTS);
  u32s = results[4].get_u32s();
  EXPECT_EQ(u32s.size(), 4);
  EXPECT_EQ((int)(u32s[0]), 10);
  EXPECT_EQ((int)(u32s[1]), -10);
  EXPECT_EQ((int)(u32s[2]), -5);
  EXPECT_EQ((int)(u32s[3]), 5);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(results[5].type, CFG_Python_OBJ::TYPE::STRS);
  strs = results[5].get_strs();
  EXPECT_EQ(strs.size(), 3);
  EXPECT_EQ(strs[0], "x");
  EXPECT_EQ(strs[1], "y");
  EXPECT_EQ(strs[2], "z");
  EXPECT_EQ(results[6].type, CFG_Python_OBJ::TYPE::NONE);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  results = mgr.run_file("python_file_test", "func2",
                         std::vector<CFG_Python_OBJ>({}));
  EXPECT_EQ(results.size(), 0);
  // Run file
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  mgr.run_file(CFG_print("%s/python_file_test.py", current_dir.c_str()),
               std::vector<std::string>({"abc", "xyz"}));
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(mgr.results().size(), 1);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
  EXPECT_EQ(mgr.result_u32("abc"), 101);
  std::cout << "test_python_mgr: " << __LINE__ << "\n" << std::flush;
}
