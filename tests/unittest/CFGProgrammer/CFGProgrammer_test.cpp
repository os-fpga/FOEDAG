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
      "Found 1   Device2   0x5678deff   5   2097152\n"
      "Found  2   Device3   0x90abcdef   6    134217728\n"
      "Found  1 gemini               0x1000563d   5          2097152\n";
  std::vector<Device> expected = {
      {1,
       "gemini",
       2097152,
       {44, "gemini.tap", true, 0x1000563d, 0x1000563d, 5, 0x1, 0x3}}
    };    
  std::vector<Device> actual;
  int status = extractDeviceList(input, actual);
  EXPECT_EQ(status, ProgrammerErrorCode::NoError);
  EXPECT_EQ(expected.size(), actual.size());
  EXPECT_EQ(expected[0].name, actual[0].name);
  EXPECT_EQ(expected[0].tapInfo.idCode, actual[0].tapInfo.idCode);
  EXPECT_EQ(expected[0].tapInfo.irMask, actual[0].tapInfo.irMask);
  EXPECT_EQ(expected[0].tapInfo.irLen, actual[0].tapInfo.irLen);
  EXPECT_EQ(expected[0].flashSize, actual[0].flashSize);
  EXPECT_EQ(expected[0].index, actual[0].index);
}

TEST(ProgrammerHelper, ExtractDeviceListBasicInvalidFlashSize) {
  std::string input =
      "Found  0   Device1   0x1234abcd   4   1111\n";
  std::vector<Device> expected = {
      {1,
       "gemini",
       16384,
       {44, "gemini.tap", true, 0x1000563d, 0x1000563d, 5, 0x1, 0x3}}
    };    
  std::vector<Device> actual;
  int status = extractDeviceList(input, actual);
  EXPECT_EQ(status, ProgrammerErrorCode::InvalidFlashSize);
}

TEST(ProgrammerHelper, ExtractDeviceListNoMatchesTest) {
  std::string input = "This is not a valid device list";
  std::vector<Device> expected = {};
  std::vector<Device> actual;
  int status = extractDeviceList(input, actual);
  EXPECT_EQ(status, ProgrammerErrorCode::NoError);
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

TEST(ProgrammerHelper, BuildFpgaCableStringStreamSerialNumEmptyTest) {
  Cable cable = {0x403, 0x6011,    45,         1,    88,
                 3,     "", "lopopolo", 2000, TransportType::jtag};
  std::stringstream expected;
  expected << std::hex << std::showbase;
  expected << " -c \"adapter driver ftdi\""
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
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
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
                 TransportType::jtag, 
                 "RsFtdi_1_1",
                 1};
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
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
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
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
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

TEST(ProgrammerHelper, BuildOTPProgramCommandBasicTest) {
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
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
      " -c \"gemini load 0 otp my_bitstreamfile.bit -p 1\" -l /dev/stdout -c "
      "\"exit\"";
  std::string actual = buildOTPProgramCommand(cable, device, bitfile);
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
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
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

#if defined(__linux__)
// API testing
#ifdef DEBUG_BUILD
TEST(ProgrammerAPI, ProgramFpgaDeathTest) {
  // Create a temporary file for testing
  Cable cable = {0x403,
                 0x6011,
                 11,
                 22,
                 33,
                 1,
                 "serial_number_xyz",
                 "description_xyz",
                 10000,
                 TransportType::jtag,
                 "RsFtdi_1_1",
                 1};
  Device device = {0,
                   "Gemini",
                   16384,
                   {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  std::string bitfile = "my_bitfile.bit";
  std::atomic<bool> stop{false};
  EXPECT_DEATH(ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
                           nullptr),
               ".*");
}
#endif // end DEBUG_BUILD
class ProgrammerAPI : public ::testing::Test {
protected:
    void SetUp() override {
      // Initialize the library before each test
      #ifdef DEBUG_BUILD
          std::cout << "ProgrammerAPI dbuild/bin/openocd" << std::endl;
          InitLibrary("dbuild/bin/openocd");
      #else
          std::cout << "ProgrammerAPI build/bin/openocd" << std::endl;
          InitLibrary("build/bin/openocd");
      #endif
    }

    void TearDown() override {
        // Tear down the test fixture.
    }
};

TEST_F(ProgrammerAPI, GetAvailableCables_ReturnsNoErrorWhenSuccessful) {
  // expect no cable is connected in CI testing environment
  std::vector<Cable> cables;
  int errorCode = GetAvailableCables(cables);
  EXPECT_EQ(cables.size(), 0);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::NoError);
}

TEST_F(ProgrammerAPI, ListDevicesTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
  // expect no cable is connected in CI testing environment
  // expect no device is connected in CI testing environment
  Cable cable;
  std::vector<Device> devices;
  int errorCode = ListDevices(cable, devices);
  EXPECT_EQ(devices.size(), 0);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotSupported);
}

TEST_F(ProgrammerAPI, GetFpgaStatusTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
  // expect no cable is connected in CI testing environment
  // expect no device is connected in CI testing environment
  Cable cable;
  Device device;
  CfgStatus status;
  std::string statusCmdOutput;
  int errorCode = GetFpgaStatus(cable, device, status, statusCmdOutput);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotSupported);
}
class ProgrammerAPI_ProgramFlashAndFpga : public ::testing::Test {
 protected:
  void SetUp() override {
// Initialize the library before each test
#ifdef DEBUG_BUILD
    InitLibrary("dbuild/bin/openocd");
#else
    InitLibrary("build/bin/openocd");
#endif
  }

  void TearDown() override {
    // Clean up any temporary files after each test
    std::remove(bitfile.c_str());
  }
  const std::string bitfile = "my_bitfile.bit";
  Cable cable{0x403,
              0x6011,
              11,
              22,
              33,
              1,
              "serial_number_xyz",
              "description_xyz",
              10000,
              TransportType::jtag,
              "RsFtdi_1_1",
              1};
  Device device{0,
                "Gemini",
                16384,
                {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};

  ProgramFlashOperation modes = {ProgramFlashOperation::BlankCheck|
                                 ProgramFlashOperation::Erase|
                                 ProgramFlashOperation::Program|
                                 ProgramFlashOperation::Verify};
  std::atomic<bool> stop{false};
};

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaFailedExecuteCommandTest) {
  // Create a temporary file for testing
  std::ofstream tempFile(bitfile);
  tempFile.close();
  int expected = ProgrammerErrorCode::FailedExecuteCommand;
  int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
                           nullptr);
  EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaBitfileNotFoundTest) {
  int expected = ProgrammerErrorCode::BitfileNotFound;
  int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr, nullptr);
  EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashFailedExecuteCommandTest) {
  // Create a temporary file for testing
  std::ofstream tempFile(bitfile);
  tempFile.close();
  int expected = ProgrammerErrorCode::FailedExecuteCommand;
  int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr,
                           nullptr);
  EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashBitfileNotFoundTest) {
  int expected = ProgrammerErrorCode::BitfileNotFound;
  int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr, nullptr);
  EXPECT_EQ(expected, actual);
}

TEST(ProgrammerHelper, InitializeHwDb) {
  // in CI, no cable is connected
  std::vector<HwDevices> cableDeviceDb;
  std::map<std::string, Cable> cableMap;
  testing::internal::CaptureStdout();
  InitializeHwDb(cableDeviceDb, cableMap);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected = "INFO: No cable found.\n";
  EXPECT_EQ(output, expected);
}
#endif // __linux__

TEST(ProgrammerHelper, printCableListTest)
{
  std::vector<Cable> cableList{{0x403, 0x6011, 2, 8, 33, 1, "serial_number_xyz", "description_xyz", 10000, TransportType::jtag, "RsFtdi_2_8", 1}};
  testing::internal::CaptureStdout();
  printCableList(cableList);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected = 
  "INFO: Cable            \n"
  "INFO: -----------------\n"
  "INFO: (1) RsFtdi_2_8\n";

  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, PrintDeviceListNoDeviceTest) {
  Cable cable{0x403, 0x6011, 1, 2, 33, 1, "serial_number_xyz", "description_xyz", 10000, TransportType::jtag, "RsFtdi_1_2", 1};
  testing::internal::CaptureStdout();
  std::vector<Device> deviceList{};
  printDeviceList(cable, deviceList);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected = 
  "INFO: Cable               | Device\n"
  "INFO: -----------------------------------------------\n"
  "INFO:   No device detected.\n";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, PrintDeviceListSimpleTest) {
  Cable cable{0x403, 0x6011, 1, 2, 33, 1, "serial_number_xyz", "description_xyz", 10000, TransportType::jtag, "RsFtdi_1_2", 1};
  std::vector<Device> deviceList{
    {1, "Device1", 16384, {1, "Device1.Tap", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2, "Gemini2", 16384, {2, "Device2.Tap", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}};
  testing::internal::CaptureStdout();
  printDeviceList(cable, deviceList);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected =
  "INFO: Cable               | Device\n"
  "INFO: -----------------------------------------------\n"
  "INFO: (1) RsFtdi_1_2        (1) Device1\n"
  "INFO: (1) RsFtdi_1_2        (2) Gemini2\n";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelperTest, RemoveInfoAndNewlineTest) {
  EXPECT_EQ(removeInfoAndNewline("Info : Hello, world!\n"), " Hello, world!");
  EXPECT_EQ(removeInfoAndNewline("Info : \n"), " ");
  EXPECT_EQ(removeInfoAndNewline("Hello, world!\n"), "Hello, world!");
  EXPECT_EQ(removeInfoAndNewline(""), "");
}

class HwDevicesTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    cable = Cable{0x403, 0x6011, 1, 2, 33, 1, "serial_number_xyz", "description_xyz", 10000, TransportType::jtag, "RsFtdi_1_2", 1};
    hwDevices = HwDevices(cable);
  }
  void TearDown() override {
    // Clean up any temporary files after each test
  }
  Cable cable;
  HwDevices hwDevices;
};

TEST_F(HwDevicesTestFixture, ConstructorTest) {
  Cable output = hwDevices.getCable();
  EXPECT_EQ(output.vendorId, cable.vendorId);
  EXPECT_EQ(output.productId, cable.productId);
  EXPECT_EQ(output.busAddr, cable.busAddr);
  EXPECT_EQ(output.portAddr, cable.portAddr);
  EXPECT_EQ(output.deviceAddr, cable.deviceAddr);
  EXPECT_EQ(output.serialNumber, cable.serialNumber);
  EXPECT_EQ(output.description, cable.description);
  EXPECT_EQ(output.speed, cable.speed);
  EXPECT_EQ(output.transport, cable.transport);
  EXPECT_EQ(output.name, cable.name);
  EXPECT_EQ(output.index, cable.index);
}

TEST_F(HwDevicesTestFixture, AddDevicesTest) {
  EXPECT_EQ(hwDevices.getDevicesCount(), 0);
  hwDevices.addDevices({
    {1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2,"Gemini2",16384,{2, "Tap2.Gemini2", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}
  });
  std::vector<Device> devices = {
    {1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2,"Gemini2",16384,{2, "Tap2.Gemini2", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}
  };
  EXPECT_EQ(hwDevices.getDevices().size(), devices.size());
  for(size_t i = 0; i < hwDevices.getDevicesCount(); i++) {
    EXPECT_EQ(hwDevices.getDevices()[i].name, devices[i].name);
    EXPECT_EQ(hwDevices.getDevices()[i].index, devices[i].index);
    EXPECT_EQ(hwDevices.getDevices()[i].flashSize, devices[i].flashSize);
  }
}

TEST_F(HwDevicesTestFixture, AddDeviceTest124) {
  EXPECT_EQ(hwDevices.getDevicesCount(), 0);
  Device device{1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  hwDevices.addDevice(device);
  EXPECT_EQ(hwDevices.getDevicesCount(), 1);
}

TEST_F(HwDevicesTestFixture, ClearDevicesTest) {
  EXPECT_EQ(hwDevices.getDevicesCount(), 0);
  hwDevices.addDevices({
    {1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2,"Gemini2",16384,{2, "Tap2.Gemini2", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}
  });
  EXPECT_EQ(hwDevices.getDevicesCount(), 2);
  hwDevices.clearDevices();
  EXPECT_EQ(hwDevices.getDevicesCount(), 0);
}

TEST_F(HwDevicesTestFixture, FindDeviceByIndexTest) {
  hwDevices.addDevices({
    {1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2,"Gemini2",32768,{2, "Tap2.Gemini2", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}
  });
  Device device;
  bool found = hwDevices.findDevice(1, device);
  EXPECT_TRUE(found);
  EXPECT_EQ(device.name, hwDevices.getDevices()[0].name);
  EXPECT_EQ(device.index, hwDevices.getDevices()[0].index);
  EXPECT_EQ(device.flashSize, hwDevices.getDevices()[0].flashSize);
}

TEST_F(HwDevicesTestFixture, FindDeviceByNameTest) {
  hwDevices.addDevices({
    {1,"Gemini1",16384,{1, "Tap1.Gemini1", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}},
    {2,"Gemini2",32768,{2, "Tap2.Gemini2", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}}
  });
  Device device;
  bool found = hwDevices.findDevice("Gemini2", device);
  EXPECT_TRUE(found);
  EXPECT_EQ(device.name, hwDevices.getDevices()[1].name);
  EXPECT_EQ(device.index, hwDevices.getDevices()[1].index);
  EXPECT_EQ(device.flashSize, hwDevices.getDevices()[1].flashSize);
}

class ProgramerTestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    cmdarg.command = "programmer";
    cmdarg.toolPath = "build/bin/openocd";
    original_cin = std::cin.rdbuf(input_stream.rdbuf());
  }
  void TearDown() override {
    std::cin.rdbuf(original_cin);
  }
  CFGCommon_ARG cmdarg;
  std::istringstream input_stream;
  std::streambuf *original_cin = nullptr;
};

TEST_F(ProgramerTestFixture, ExecuteOTPCommandWithDummyCompiler) {
  const char *argv[] = { "otp", "-c", "1", "bitstream.cfgbit" };
  int argc = 4;

  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  arg->parse(argc, argv, nullptr);
  cmdarg.arg = arg;
  cmdarg.compilerName = "dummy";
  programmer_entry(&cmdarg);

  // just for code coverage but nothing to be observed for now
}

TEST_F(ProgramerTestFixture, ExecuteOTPCommandWithOpenFPGACompilerAndYFlag) {
  const char *argv[] = { "otp", "-c", "1", "bitstream.cfgbit", "-y" };
  int argc = 5;

  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  arg->parse(argc, argv, nullptr);
  cmdarg.arg = arg;
  cmdarg.compilerName = "openfpga";
  programmer_entry(&cmdarg);

  // just for code coverage but nothing to be observed for now
}

TEST_F(ProgramerTestFixture, ExecuteOTPCommandWithPromptingAnd_Y_Anwser) {
  const char *argv[] = { "otp", "-c", "1", "bitstream.cfgbit" };
  int argc = 4;

  input_stream.str("Y");

  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  arg->parse(argc, argv, nullptr);
  cmdarg.arg = arg;
  cmdarg.compilerName = "openfpga";
  programmer_entry(&cmdarg);

  // just for code coverage but nothing to be observed for now
}

TEST_F(ProgramerTestFixture, ExecuteOTPCommandWithPromptingAnd_N_Anwser) {
  const char *argv[] = { "otp", "-c", "1", "bitstream.cfgbit" };
  int argc = 4;

  input_stream.str("N");

  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  arg->parse(argc, argv, nullptr);
  cmdarg.arg = arg;
  cmdarg.compilerName = "openfpga";
  programmer_entry(&cmdarg);

  // just for code coverage but nothing to be observed for now
}

TEST_F(ProgramerTestFixture, ExecuteOTPCommandWithPromptingAnd_Garbage_Anwser) {
  const char *argv[] = { "otp", "-c", "2", "bitstream.cfgbit" };
  int argc = 4;

  input_stream.str("%^@sadsad");

  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  arg->parse(argc, argv, nullptr);
  cmdarg.arg = arg;
  cmdarg.compilerName = "openfpga";
  programmer_entry(&cmdarg);

  // just for code coverage but nothing to be observed for now
}
