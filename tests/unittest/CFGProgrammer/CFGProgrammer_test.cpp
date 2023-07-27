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


#include "Configuration/Programmer/Programmer.h"
#include <fstream> // for std::ofstream

#include "Configuration/CFGCommon/CFGCommon.h"
#include "Configuration/Programmer/Programmer_helper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(ProgrammerHelper, TransportTypeToStringTest_JtagTransport) {
  EXPECT_EQ("jtag", FOEDAG::transportTypeToString(FOEDAG::TransportType::jtag));
}

TEST(ProgrammerHelper, FindStringPatternBasicTest1) {
  std::string input = "hello world";
  std::string pattern = "world";
  std::vector<std::string> expected = {"world"};
  std::vector<std::string> actual = findStringPattern(input, pattern);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, FindStringPatternBasicTest2) {
  std::string input =
      "3 auto0.tap              Y     0x20000913 0x00000000     5 0x01  0x03";
  const std::string pattern(
      R"(\s*(\d+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\d+)\s+(\S+)\s+(\S+)\s*)");
  std::vector<std::string> expected = {
      "3 auto0.tap              Y     0x20000913 0x00000000     5 0x01  0x03"};
  std::vector<std::string> actual = findStringPattern(input, pattern);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, FindStringPatternMultipleMatchesTest) {
  std::string input = "hello world, hello universe";
  std::string pattern = "hello";
  std::vector<std::string> expected = {"hello", "hello"};
  std::vector<std::string> actual = findStringPattern(input, pattern);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, FindStringPatternNoMatchesTest) {
  std::string input = "hello world";
  std::string pattern = "universe";
  std::vector<std::string> expected = {};
  std::vector<std::string> actual = findStringPattern(input, pattern);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, ExtractTapInfoListTest) {
  std::string testString =
      "Open On-Chip Debugger 0.12.0+dev-ge087524d5 (2023-07-04-13:41)\n"
      "  Licensed under GNU GPL v2\n"
      "  For bug reports, read\n"
      "  http://openocd.org/doc/doxygen/bugs.html\"\n"
      "adapter speed: 1000 kHz\n"
      "jtag\n"
      "Info : clock speed 1000 kHz\n"
      "Warn : There are no enabled taps.  AUTO PROBING MIGHT NOT WORK!!\n"
      "Error: JTAG scan chain interrogation failed: all ones\n"
      "Error: Check JTAG interface, timings, target power, etc.\n"
      "Error: Trying to use configured scan chain anyway...\n"
      "Warn : Bypassing JTAG setup events due to errors\n"
      "Warn : gdb services need one or more targets defined\n"
      "12 34 56 78 90 ab cd ef\n"
      "Info : JTAG tap: omap5912.dsp tap/device found: 0x03df1d81 (mfg: 0x017, "
      "part: 0x3df1, ver: 0x0)\n"
      "JTAG chainpos: 0 Device IDCODE = 0x03df1d81\n"
      "   TapName            Enabled IdCode     Expected   IrLen IrCap IrMask\n"
      "-- ------------------ ------- ---------- ---------- ----- ----- ------\n"
      "0  omap5912.dsp          Y    0x03df1d81 0x03df1d81 38    0x01  0x03\n"
      "1  omap5912.arm          Y    0x0692602f 0x0692602f 4     0x01  0x0f\n"
      "2  omap5912.unknown      Y    0x00000000 0x00000000 8     0x01  0x03\n"
      "3 auto0.tap              Y     0x20000913 0x00000000     5 0x01  0x03\n";

  std::vector<TapInfo> expected = {
      {0, "omap5912.dsp", true, 0x03df1d81, 0x03df1d81, 38, 0x01, 0x03},
      {1, "omap5912.arm", true, 0x0692602f, 0x0692602f, 4, 0x01, 0x0f},
      {2, "omap5912.unknown", true, 0x00000000, 0x00000000, 8, 0x01, 0x03},
      {3, "auto0.tap", true, 0x20000913, 0x00000000, 5, 0x01, 0x03}};

  std::vector<TapInfo> result = extractTapInfoList(testString);

  ASSERT_EQ(result.size(), expected.size());

  for (size_t i = 0; i < result.size(); i++) {
    ASSERT_EQ(result[i].index, expected[i].index);
    ASSERT_EQ(result[i].tapName, expected[i].tapName);
    ASSERT_EQ(result[i].enabled, expected[i].enabled);
    ASSERT_EQ(result[i].idCode, expected[i].idCode);
    ASSERT_EQ(result[i].expected, expected[i].expected);
    ASSERT_EQ(result[i].irLen, expected[i].irLen);
    ASSERT_EQ(result[i].irCap, expected[i].irCap);
    ASSERT_EQ(result[i].irMask, expected[i].irMask);
  }
}

TEST(ProgrammerHelper, ExtractDeviceListBasicTest) {
  std::string input =
      "Found  0   Device1   0x1234abcd   4   16384\n"
      "Found 1   Device2   0x5678efgh   5   1024000\n"
      "Found  2   Device3   0x90abcdef   6   1612312384\n";
  std::vector<Device> expected = {
      {0,
       "Device1",
       16384,
       {11, "Device1.tap", true, 0x1234abcd, 0x1234abcd, 4, 0x1, 0x3}},
      {1,
       "Device2",
       1024000,
       {22, "Device2.tap", true, 0x5678deff, 0x5678deff, 5, 0x1, 0x3}},
      {2,
       "Device3",
       1612312384,
       {33, "Device3.tap", true, 0x90abcdef, 0x90abcdef, 6, 0x1, 0x3}}};
  std::vector<Device> actual = extractDeviceList(input);
  for (size_t i = 0; i < actual.size(); i++) {
    EXPECT_EQ(expected[i].index, actual[i].index);
    EXPECT_EQ(expected[i].name, actual[i].name);
    EXPECT_EQ(expected[i].tapInfo.idCode, actual[i].tapInfo.idCode);
    EXPECT_EQ(expected[i].tapInfo.irMask, actual[i].tapInfo.irMask);
    EXPECT_EQ(expected[i].tapInfo.irLen, actual[i].tapInfo.irLen);
    EXPECT_EQ(expected[i].flashSize, actual[i].flashSize);
  }
}

TEST(ProgrammerHelper, ExtractDeviceListNoMatchesTest) {
  std::string input = "This is not a valid device list";
  std::vector<Device> expected = {};
  std::vector<Device> actual = extractDeviceList(input);
  EXPECT_EQ(expected.size(), actual.size());
}

TEST(ProgrammerHelper, ExtractStatusValidInputTest) {
  std::string statusString =
      "         Device               cfg_done   cfg_error\n"
      "-------- -------------------- ---------- ----------\n"
      "Found  0 Gemini               1          0        \n";
  CfgStatus expectedStatus = {1, 0};
  bool expectedStatusFound = false;
  CfgStatus actualStatus = extractStatus(statusString, expectedStatusFound);
  EXPECT_EQ(expectedStatusFound, true);
  EXPECT_EQ(expectedStatus.cfgDone, actualStatus.cfgDone);
  EXPECT_EQ(expectedStatus.cfgError, actualStatus.cfgError);
}

TEST(ProgrammerHelper, ExtractStatusInvalidInputTest) {
  std::string statusString = "This is not a valid status string";
  CfgStatus expectedStatus = {0, 0};
  bool expectedStatusFound = true;
  CfgStatus actualStatus = extractStatus(statusString, expectedStatusFound);
  EXPECT_EQ(expectedStatusFound, false);
  EXPECT_EQ(expectedStatus.cfgDone, actualStatus.cfgDone);
  EXPECT_EQ(expectedStatus.cfgError, actualStatus.cfgError);
}

TEST(ProgrammerHelper, IsCableSupportedTest_ReturnsNoErrorWhenCableIsSupported) {
  Cable cable1{0x0403, 0x6011};
  Cable cable2{0x0403, 0x6010};
  int errorCode = isCableSupported(cable1);
  errorCode = isCableSupported(cable2);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::NoError);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::NoError);
}

TEST(ProgrammerHelper, IsCableSupportedTest_ReturnsCableNotSupportedErrorWhenCableIsNotSupported) {
  Cable cable{0x1111, 0x2222};
  int errorCode = isCableSupported(cable);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotSupported);
}

TEST(ProgrammerHelper, BuildFpgaCableStringStreamBasicTest) {
  Cable cable = {0x403, 0x6011,    45,         1,    88,
                 3,     "F-M14D5", "lopopolo", 2000, TransportType::jtag};
  std::stringstream expected;
  expected << std::hex << std::showbase;
  expected << " -c \"adapter driver ftdi\""
           << " -c \"adapter serial " << cable.serialNumber << "\""
           << " -c \"ftdi vid_pid " << cable.vendorId << " " << cable.productId
           << "\""
           << " -c \"ftdi layout_init 0x0c08 0x0f1b\"";
  expected << std::dec << std::noshowbase;
  expected << " -c \"adapter speed " << cable.speed << "\""
           << " -c \"transport select "
           << transportTypeToString(cable.transport) << "\"";

  std::stringstream actual = buildFpgaCableStringStream(cable);

  EXPECT_EQ(expected.str(), actual.str());
}

TEST(ProgrammerHelper, BuildFpgaTargetStringStreamBasicTest) {
  Device device = {0,
                   "DeviceXZ",
                   16384,
                   {11, "DeviceXZ", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  std::stringstream expected;
  expected << " -c \"jtag newtap " << device.name << device.index
           << " tap -irlen " << device.tapInfo.irLen << " -expected-id 0x"
           << std::hex << device.tapInfo.expected << "\"";
  expected << std::dec;
  expected << " -c \"target create " << device.name << device.index
           << " riscv -endian little -chain-position " << device.name
           << device.index << ".tap\"";
  expected << " -c \"pld device gemini " << device.name << device.index << "\"";
  std::stringstream actual = buildFpgaTargetStringStream(device);

  EXPECT_EQ(expected.str(), actual.str());
}

TEST(ProgrammerHelper, BuildInitEndStringWithCommandBasicTest) {
  std::string command = "programming-x";
  std::string expected =
      " -c \"init\" -c \"programming-x\" -l /dev/stdout -c \"exit\"";
  std::string actual = buildInitEndStringWithCommand(command);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, BuildFpgaCableCommandBasicTest) {
  Cable cable = {0x403, 0x6011,       0,          1,    2,
                 3,     "CSJ01222DM", "kikilala", 9600, TransportType::jtag};
  std::string command = "test_command";
  std::string expected = buildFpgaCableStringStream(cable).str() +
                         " -c \"init\""
                         " -c \"" +
                         command +
                         "\""
                         " -l /dev/stdout"
                         " -c \"exit\"";

  std::string actual = buildFpgaCableCommand(cable, command);

  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, BuildScanChainCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag};
  std::string expected =
      " -c \"adapter driver ftdi\""
      " -c \"adapter serial serial_number_xyz\""
      " -c \"ftdi vid_pid 0x403 0x6011\""
      " -c \"ftdi layout_init 0x0c08 0x0f1b\""
      " -c \"adapter speed 10000\""
      " -c \"transport select jtag\""
      " -c \"init\" -c \"scan_chain\" -l /dev/stdout -c \"exit\"";
  std::string actual = buildScanChainCommand(cable);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, BuildListDeviceCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag};
  std::vector<TapInfo> foundTapList = {
      {0, "Gemini", true, 0x20000913, 0x20000913, 0x5, 0x1, 0x3}};
  std::string expected =
      " -c \"adapter driver ftdi\""
      " -c \"adapter serial serial_number_xyz\""
      " -c \"ftdi vid_pid 0x403 0x6011\""
      " -c \"ftdi layout_init 0x0c08 0x0f1b\""
      " -c \"adapter speed 10000\" -c \"transport select jtag\""
      " -c \"jtag newtap Gemini0 tap -irlen 5 -expected-id 0x20000913\""
      " -c \"target create Gemini0 riscv -endian little -chain-position "
      "Gemini0.tap\""
      " -c \"pld device gemini Gemini0\""
      " -c \"init\" -c \"gemini list\" -l /dev/stdout -c \"exit\"";
  std::string actual = buildListDeviceCommand(cable, foundTapList);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, BuildListDeviceCommandNoTapsTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag};
  std::vector<TapInfo> foundTapList = {};
  std::string expected = "";
  std::string actual = buildListDeviceCommand(cable, foundTapList);
  EXPECT_EQ(expected, actual);
}

class BuildFlashProgramCommandTest
    : public testing::TestWithParam<std::tuple<
          std::string, std::string, int, bool, bool, bool, bool, std::string>> {
};

TEST_P(BuildFlashProgramCommandTest, BuildFlashProgramCommandTest) {
  std::string bitstream_file = std::get<0>(GetParam());
  std::string config_file = std::get<1>(GetParam());
  int pld_index = std::get<2>(GetParam());
  bool doErase = std::get<3>(GetParam());
  bool doBlankCheck = std::get<4>(GetParam());
  bool doProgram = std::get<5>(GetParam());
  bool doVerify = std::get<6>(GetParam());
  std::string expected = std::get<7>(GetParam());
  std::string actual =
      buildFlashProgramCommand(bitstream_file, config_file, pld_index, doErase,
                               doBlankCheck, doProgram, doVerify);
  EXPECT_EQ(expected, actual);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, BuildFlashProgramCommandTest,
    testing::Values(
        std::make_tuple("bitstream.bin", "config.cfg", 0, true, true, true,
                        true,
                        " -f config.cfg -c \"gemini load 0 flash bitstream.bin "
                        "-p 1\" -l /dev/stdout -c exit"),
        std::make_tuple("bitstream2.bin", "config2.cfg", 1, false, false, false,
                        false,
                        " -f config2.cfg -c \"gemini load 1 flash "
                        "bitstream2.bin -p 1\" -l /dev/stdout -c exit"),
        std::make_tuple("bitstream3.bin", "config3.cfg", 2, true, false, true,
                        false,
                        " -f config3.cfg -c \"gemini load 2 flash "
                        "bitstream3.bin -p 1\" -l /dev/stdout -c exit")));

TEST(ProgrammerHelper, ParseProgrammerCommandHelpCommandTest) {
  CFGCommon_ARG cmdarg;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAMMER>();
  cmdarg.arg->m_help = true;
  ProgrammerCommand expected;
  expected.name = "help";
  ProgrammerCommand actual = parseProgrammerCommand(&cmdarg, "");
  EXPECT_EQ(expected.name, actual.name);
  EXPECT_EQ(expected.is_error, actual.is_error);
}

TEST(ProgrammerHelper, ParseProgrammerCommandNotEnoughArgumentsTest) {
  CFGCommon_ARG cmdarg;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAMMER>();
  ProgrammerCommand expected;
  expected.is_error = true;
  ProgrammerCommand actual = parseProgrammerCommand(&cmdarg, "");
  EXPECT_EQ(expected.is_error, actual.is_error);
}

TEST(ProgrammerHelper, ParseProgrammerCommandInvalidCommandTest) {
  CFGCommon_ARG cmdarg;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAMMER>();
  cmdarg.arg->m_args.push_back("invalid_command");
  ProgrammerCommand expected;
  expected.is_error = true;
  ProgrammerCommand actual = parseProgrammerCommand(&cmdarg, "");
  EXPECT_EQ(expected.is_error, actual.is_error);
}

TEST(ProgrammerHelper, BuildFpgaQueryStatusCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag};
  Device device = {0,
                   "DeviceXZ",
                   16384,
                   {99, "DeviceXZ", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  std::string expected =
      " -c \"adapter driver ftdi\""
      " -c \"adapter serial serial_number_xyz\""
      " -c \"ftdi vid_pid 0x403 0x6011\""
      " -c \"ftdi layout_init 0x0c08 0x0f1b\""
      " -c \"adapter speed 10000\" -c \"transport select jtag\""
      " -c \"jtag newtap DeviceXZ0 tap -irlen 5 -expected-id 0x1234aabb\""
      " -c \"target create DeviceXZ0 riscv -endian little -chain-position "
      "DeviceXZ0.tap\""
      " -c \"pld device gemini DeviceXZ0\""
      " -c \"init\" -c \"gemini status 0\" -l /dev/stdout -c \"exit\"";
  std::string actual = buildFpgaQueryStatusCommand(cable, device);
  EXPECT_EQ(expected, actual);
}

class ParseProgrammerCommandTest
    : public testing::TestWithParam<
          std::tuple<std::string, std::string, int, std::string, std::string>> {
};

TEST_P(ParseProgrammerCommandTest, Test) {
  std::string cmd_name = std::get<0>(GetParam());
  std::string bitstream_file = std::get<1>(GetParam());
  int pld_index = std::get<2>(GetParam());
  std::string compiler_name = std::get<3>(GetParam());
  std::string expected = std::get<4>(GetParam());
  std::filesystem::path config_file_path = "config.cfg";
  CFGCommon_ARG cmdarg;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAMMER>();
  std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg.arg)->index = pld_index;
  cmdarg.arg->m_args.push_back(cmd_name);
  cmdarg.arg->m_args.push_back(bitstream_file);
  cmdarg.compilerName = compiler_name;
  ProgrammerCommand actual = parseProgrammerCommand(&cmdarg, config_file_path);
  EXPECT_EQ(expected, actual.executable_cmd);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, ParseProgrammerCommandTest,
    testing::Values(
        std::make_tuple("fpga_config", "bitstream.bin", 0, "dummy",
                        " -f config.cfg -c \"gemini load 0 fpga bitstream.bin "
                        "-p 1\" -l /dev/stdout -c exit"),
        std::make_tuple("flash", "bitstream2.bin", 1, "dummy",
                        " -f config.cfg -c \"gemini load 1 flash "
                        "bitstream2.bin -p 1\" -l /dev/stdout -c exit"),
        std::make_tuple(
            "fpga_status", "bitstream3.bin", 2, "dummy",
            " -f config.cfg -c \"gemini status 2\" -l /dev/stdout -c exit"),
        std::make_tuple(
            "list_devices", "bitstream4.bin", 3, "dummy",
            " -f config.cfg -c \"gemini list\" -l /dev/stdout -c exit")));

TEST(ProgrammerHelper, BuildFpgaProgramCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag};
  Device device = {0,
                   "Gemini",
                   16384,
                   {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  std::string bitfile = "my_bitstreamfile.bit";
  std::string expected =
      " -c \"adapter driver ftdi\" -c \"adapter serial serial_number_xyz\""
      " -c \"ftdi vid_pid 0x403 0x6011\" -c \"ftdi layout_init 0x0c08 0x0f1b\""
      " -c \"adapter speed 10000\" -c \"transport select jtag\""
      " -c \"jtag newtap Gemini0 tap -irlen 5 -expected-id 0x1234aabb\""
      " -c \"target create Gemini0 riscv -endian little -chain-position "
      "Gemini0.tap\""
      " -c \"pld device gemini Gemini0\" -c \"init\""
      " -c \"gemini load 0 fpga my_bitstreamfile.bit -p 1\" -l /dev/stdout -c "
      "\"exit\"";
  std::string actual = buildFpgaProgramCommand(cable, device, bitfile);
  EXPECT_EQ(expected, actual);
}

class BuildFpgaProgramCommandTest
    : public testing::TestWithParam<
          std::tuple<std::string, std::string, int, std::string>> {};

TEST_P(BuildFpgaProgramCommandTest, Test) {
  std::string bitstream_file = std::get<0>(GetParam());
  std::string config_file = std::get<1>(GetParam());
  int pld_index = std::get<2>(GetParam());
  std::string expected = std::get<3>(GetParam());
  std::string actual =
      buildFpgaProgramCommand(bitstream_file, config_file, pld_index);
  EXPECT_EQ(expected, actual);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, BuildFpgaProgramCommandTest,
    testing::Values(
        std::make_tuple(
            "bitstream.bin", "config.cfg", 0,
            " -f config.cfg -c \"gemini load 0 fpga bitstream.bin -p 1\""
            " -l /dev/stdout -c exit"),
        std::make_tuple(
            "other_bitstream.bin", "config.cfg", 0,
            " -f config.cfg -c \"gemini load 0 fpga other_bitstream.bin -p 1\""
            " -l /dev/stdout -c exit"),
        std::make_tuple(
            "bitstream.bin", "other_config.cfg", 0,
            " -f other_config.cfg -c \"gemini load 0 fpga bitstream.bin -p 1\""
            " -l /dev/stdout -c exit"),
        std::make_tuple(
            "bitstream.bin", "config.cfg", 2,
            " -f config.cfg -c \"gemini load 2 fpga bitstream.bin -p 1\""
            " -l /dev/stdout -c exit")));

TEST(ProgrammerHelper, BuildFlashProgramCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_aaa",
                 "description_bbb",
                 10000,
                 TransportType::jtag};
  Device device = {0,
                   "Pluto",
                   16384,
                   {99, "Pluto", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  ProgramFlashOperation modes = {ProgramFlashOperation::BlankCheck|
                                 ProgramFlashOperation::Erase|
                                 ProgramFlashOperation::Program|
                                 ProgramFlashOperation::Verify};
  std::string bitfile = "my_bitstream_flash_file.bit";
  std::string expected =
      " -c \"adapter driver ftdi\" -c \"adapter serial serial_number_aaa\""
      " -c \"ftdi vid_pid 0x403 0x6011\" -c \"ftdi layout_init 0x0c08 0x0f1b\""
      " -c \"adapter speed 10000\" -c \"transport select jtag\""
      " -c \"jtag newtap Pluto0 tap -irlen 5 -expected-id 0x1234aabb\""
      " -c \"target create Pluto0 riscv -endian little -chain-position "
      "Pluto0.tap\""
      " -c \"pld device gemini Pluto0\" -c \"init\""
      " -c \"gemini load 0 flash my_bitstream_flash_file.bit -p 1\" -l /dev/stdout -c "
      "\"exit\"";
  std::string actual = buildFlashProgramCommand(cable, device, bitfile, modes);
  EXPECT_EQ(expected, actual);
}

class BuildFpgaQueryStatusCommandTest
    : public testing::TestWithParam<std::tuple<std::string, int, std::string>> {
};

TEST_P(BuildFpgaQueryStatusCommandTest, Test) {
  std::string config_file = std::get<0>(GetParam());
  int pld_index = std::get<1>(GetParam());
  std::string expected = std::get<2>(GetParam());
  std::string actual = buildFpgaQueryStatusCommand(config_file, pld_index);
  EXPECT_EQ(expected, actual);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, BuildFpgaQueryStatusCommandTest,
    testing::Values(
        std::make_tuple(
            "config.cfg", 0,
            " -f config.cfg -c \"gemini status 0\" -l /dev/stdout -c exit"),
        std::make_tuple(
            "config.cfg", 2,
            " -f config.cfg -c \"gemini status 2\" -l /dev/stdout -c exit"),
        std::make_tuple("other_config.cfg", 0,
                        " -f other_config.cfg -c \"gemini status 0\" -l "
                        "/dev/stdout -c exit")));

class ParseOperationStringTest
    : public testing::TestWithParam<
          std::tuple<std::string, std::vector<std::string>>> {};

TEST_P(ParseOperationStringTest, Test) {
  std::string operations = std::get<0>(GetParam());
  std::vector<std::string> expected = std::get<1>(GetParam());
  std::vector<std::string> actual = parseOperationString(operations);
  EXPECT_EQ(expected, actual);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, ParseOperationStringTest,
    testing::Values(
        std::make_tuple("program, verify, erase",
                        std::vector<std::string>{"program", "verify", "erase"}),
        std::make_tuple("program|verify, erase",
                        std::vector<std::string>{"program", "verify", "erase"}),
        std::make_tuple("program, verify, program",
                        std::vector<std::string>{"program", "verify"}),
        std::make_tuple("", std::vector<std::string>{})));

class IsOperationRequestedTest
    : public testing::TestWithParam<
          std::tuple<std::string, std::vector<std::string>, bool>> {};

TEST_P(IsOperationRequestedTest, Test) {
  std::string operation = std::get<0>(GetParam());
  std::vector<std::string> supported_operations = std::get<1>(GetParam());
  bool expected = std::get<2>(GetParam());
  bool actual = isOperationRequested(operation, supported_operations);
  EXPECT_EQ(expected, actual);
}

INSTANTIATE_TEST_SUITE_P(
    ProgrammerHelper, IsOperationRequestedTest,
    testing::Values(
        std::make_tuple("program",
                        std::vector<std::string>{"program", "verify", "erase"},
                        true),
        std::make_tuple("reset",
                        std::vector<std::string>{"program", "verify", "erase"},
                        false),
        std::make_tuple("program", std::vector<std::string>{}, false)));

// API testing
// #ifdef DEBUG_BUILD
// TEST(ProgrammerAPI, ProgramFpgaDeathTest) {
//   // Create a temporary file for testing
//   Cable cable = {0x403,
//                  0x6011,
//                  11,
//                  22,
//                  33,
//                  1,
//                  "serial_number_xyz",
//                  "description_xyz",
//                  10000,
//                  TransportType::jtag};
//   Device device = {0,
//                    "Gemini",
//                    16384,
//                    {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
//   std::string bitfile = "my_bitfile.bit";
//   std::atomic<bool> stop{false};
//   EXPECT_DEATH(ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
//                            nullptr),
//                ".*");
// }
// #endif
// class ProgrammerAPI : public ::testing::Test {
// protected:
//     void SetUp() override {
//       // Initialize the library before each test
//       #ifdef DEBUG_BUILD
//           std::cout << "ProgrammerAPI dbuild/bin/openocd" << std::endl;
//           InitLibrary("dbuild/bin/openocd");
//       #else
//           std::cout << "ProgrammerAPI build/bin/openocd" << std::endl;
//           InitLibrary("build/bin/openocd");
//       #endif
//     }

//     void TearDown() override {
//         // Tear down the test fixture.
//     }

//     // Declare any variables or helper functions that you need.
// };
// TEST_F(ProgrammerAPI, GetAvailableCables_ReturnsNoErrorWhenSuccessful) {
//   // expect no cable is connected in CI testing environment
//   std::vector<Cable> cables;
//   int errorCode = GetAvailableCables(cables);
//   EXPECT_EQ(cables.size(), 0);
//   EXPECT_EQ(errorCode, ProgrammerErrorCode::NoError);
// }

// TEST_F(ProgrammerAPI, ListDevicesTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
//   // expect no cable is connected in CI testing environment
//   // expect no device is connected in CI testing environment
//   Cable cable;
//   std::vector<Device> devices;
//   int errorCode = ListDevices(cable, devices);
//   EXPECT_EQ(devices.size(), 0);
//   EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotSupported);
// }

// TEST_F(ProgrammerAPI, GetFpgaStatusTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
//   // expect no cable is connected in CI testing environment
//   // expect no device is connected in CI testing environment
//   Cable cable;
//   Device device;
//   CfgStatus status;
//   int errorCode = GetFpgaStatus(cable, device, status);
//   EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotSupported);
// }

// class ProgrammerAPI_ProgramFlashAndFpga : public ::testing::Test {
//  protected:
//   void SetUp() override {
// // Initialize the library before each test
// #ifdef DEBUG_BUILD
//     InitLibrary("dbuild/bin/openocd");
// #else
//     InitLibrary("build/bin/openocd");
// #endif
//   }

//   void TearDown() override {
//     // Clean up any temporary files after each test
//     std::remove(bitfile.c_str());
//   }
//   const std::string bitfile = "my_bitfile.bit";
//   Cable cable{0x403,
//               0x6011,
//               11,
//               22,
//               33,
//               1,
//               "serial_number_xyz",
//               "description_xyz",
//               10000,
//               TransportType::jtag};
//   Device device{0,
//                 "Gemini",
//                 16384,
//                 {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  
//   ProgramFlashOperation modes = {ProgramFlashOperation::BlankCheck|
//                                  ProgramFlashOperation::Erase|
//                                  ProgramFlashOperation::Program|
//                                  ProgramFlashOperation::Verify};
//   std::atomic<bool> stop{false};
// };

// TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaFailedExecuteCommandTest) {
//   // Create a temporary file for testing
//   std::ofstream tempFile(bitfile);
//   tempFile.close();
//   int expected = ProgrammerErrorCode::FailedExecuteCommand;
//   int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
//                            nullptr);
//   EXPECT_EQ(expected, actual);
// }

// TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaBitfileNotFoundTest) {
//   int expected = ProgrammerErrorCode::BitfileNotFound;
//   int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr, nullptr);
//   EXPECT_EQ(expected, actual);
// }

// TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashFailedExecuteCommandTest) {
//   // Create a temporary file for testing
//   std::ofstream tempFile(bitfile);
//   tempFile.close();
//   int expected = ProgrammerErrorCode::FailedExecuteCommand;
//   int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr,
//                            nullptr);
//   EXPECT_EQ(expected, actual);
// }

// TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashBitfileNotFoundTest) {
//   int expected = ProgrammerErrorCode::BitfileNotFound;
//   int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr, nullptr);
//   EXPECT_EQ(expected, actual);
// }

