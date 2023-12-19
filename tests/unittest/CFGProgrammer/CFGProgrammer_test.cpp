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
#include "Configuration/Programmer/Programmer_error_code.h"
#include "Configuration/HardwareManager/Cable.h"
#include "Configuration/HardwareManager/Device.h"
#include "Configuration/HardwareManager/Tap.h"
#include "Configuration/HardwareManager/ProgrammingAdapter.h"
#include "Configuration/Programmer/ProgrammerTool.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace {
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
  // Cable cable = {0x403,
  //                0x6011,
  //                11,
  //                22,
  //                33,
  //                1,
  //                "serial_number_xyz",
  //                "description_xyz",
  //                10000,
  //                TransportType::JTAG,
  //                "RsFtdi_1_1",
  //                1};
  // Device device = {0,
  //                  "Gemini",
  //                  16384,
  //                  {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};
  // std::string bitfile = "my_bitfile.bit";
  // std::atomic<bool> stop{false};
  // EXPECT_DEATH(ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
  //                          nullptr),
  //              ".*");
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
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotFound);
}

TEST_F(ProgrammerAPI, GetFpgaStatusTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
  // expect no cable is connected in CI testing environment
  // expect no device is connected in CI testing environment
  Cable cable;
  Device device;
  CfgStatus status;
  std::string statusCmdOutput;
  int errorCode = GetFpgaStatus(cable, device, status, statusCmdOutput);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotFound);
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
  // Cable cable{0x403,
  //             0x6011,
  //             11,
  //             22,
  //             33,
  //             1,
  //             "serial_number_xyz",
  //             "description_xyz",
  //             10000,
  //             TransportType::JTAG,
  //             "RsFtdi_1_1",
  //             1};
  // Device device{0,
  //               "Gemini",
  //               16384,
  //               {99, "Gemini", true, 0x1234AABB, 0x1234AABB, 5, 0x1, 0x3}};

  ProgramFlashOperation modes = {ProgramFlashOperation::BlankCheck|
                                 ProgramFlashOperation::Erase|
                                 ProgramFlashOperation::Program|
                                 ProgramFlashOperation::Verify};
  std::atomic<bool> stop{false};
};

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaFailedExecuteCommandTest) {
  // Create a temporary file for testing
  // std::ofstream tempFile(bitfile);
  // tempFile.close();
  // int expected = ProgrammerErrorCode::FailedExecuteCommand;
  // int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr,
  //                          nullptr);
  // EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaBitfileNotFoundTest) {
  // int expected = ProgrammerErrorCode::BitfileNotFound;
  // int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr, nullptr);
  // EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashFailedExecuteCommandTest) {
  // Create a temporary file for testing
  // std::ofstream tempFile(bitfile);
  // tempFile.close();
  // int expected = ProgrammerErrorCode::FailedExecuteCommand;
  // int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr,
  //                          nullptr);
  // EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashBitfileNotFoundTest) {
  // int expected = ProgrammerErrorCode::BitfileNotFound;
  // int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr, nullptr);
  // EXPECT_EQ(expected, actual);
}
#endif // __linux__

TEST(ProgrammerHelper, printCableListTest)
{
  std::vector<Cable> cableList {{1, 0x403, 0x6011, 2, 8, 33, 1, 10000, "serial_number_xyz", "description_xyz", "RsFtdi_2_8", TransportType::JTAG, CableType::FTDI}};
  testing::internal::CaptureStdout();
  printCableList(cableList, true);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected = 
  "INFO: Cable            \n"
  "INFO: -----------------\n"
  "INFO: (1) RsFtdi_2_8\n";

  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, PrintDeviceListNoDeviceTest) {
  Cable cable {1, 0x403, 0x6011, 1, 2, 33, 1, 10000, "serial_number_xyz", "description_xyz", "RsFtdi_1_2", TransportType::JTAG, CableType::FTDI};
  testing::internal::CaptureStdout();
  std::vector<Device> deviceList{};
  printDeviceList(cable, deviceList, true);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected = 
  "INFO:   No device is detected.\n";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, PrintDeviceListSimpleTest) {
  Cable cable{1, 0x403, 0x6011, 1, 2, 33, 1, 10000, "serial_number_xyz", "description_xyz", "RsFtdi_1_2", TransportType::JTAG, CableType::FTDI};
  std::vector<Device> deviceList{
    {1, "Device1", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABB, 5}},
    {2, "Gemini2", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABC, 5}}};
  testing::internal::CaptureStdout();
  printDeviceList(cable, deviceList, true);
  std::string output = testing::internal::GetCapturedStdout();
  std::string expected =
  "INFO: Cable                       | Device            | Flash Size\n"
  "INFO: -------------------------------------------------------------\n"
  "INFO: (1) RsFtdi_1_2                (1) Device1         16K               \n"
  "INFO: (1) RsFtdi_1_2                (2) Gemini2         16K               \n";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, BuildCableDeviceAliasNameTest) {
  Cable cable{1, 0x403, 0x6011, 1, 2, 33, 1, 10000, "serial_number_xyz", "description_xyz", "RsFtdi_1_2", TransportType::JTAG, CableType::FTDI};
  std::vector<Device> deviceList{
    {1, "Device1", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABB, 5}},
    {2, "Gemini2", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABC, 5}}};
  std::string output = buildCableDeviceAliasName(cable, deviceList[0]);
  std::string expected = "RsFtdi_1_2-" + deviceList[0].name + "<" + std::to_string(deviceList[0].index) + ">-" + "16KB";
  EXPECT_EQ(output, expected);
  output = buildCableDeviceAliasName(cable, deviceList[1]);
  expected = "RsFtdi_1_2-" + deviceList[1].name + "<" + std::to_string(deviceList[1].index) + ">-" + "16KB";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelper, buildCableDevicesAliasNameWithSpaceSeparatedString) {
  Cable cable{1, 0x403, 0x6011, 1, 2, 33, 1, 10000, "serial_number_xyz", "description_xyz", "RsFtdi_1_2", TransportType::JTAG, CableType::FTDI};
  std::vector<Device> deviceList{
    {1, "Device1", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABB, 5}},
    {2, "Gemini2", 16384, DeviceType::GEMINI, cable, {1, 0x1234AABC, 5}}};
  std::string output = buildCableDevicesAliasNameWithSpaceSeparatedString(cable, deviceList);
  std::string expected = "RsFtdi_1_2-" + deviceList[0].name + "<" + std::to_string(deviceList[0].index) + ">-" + "16KB"
   + " RsFtdi_1_2-" + deviceList[1].name + "<" + std::to_string(deviceList[1].index) + ">-" + "16KB";
  EXPECT_EQ(output, expected);
}

TEST(ProgrammerHelperTest, RemoveInfoAndNewlineTest) {
  EXPECT_EQ(removeInfoAndNewline("Info : Hello, world!\n"), " Hello, world!");
  EXPECT_EQ(removeInfoAndNewline("Info : \n"), " ");
  EXPECT_EQ(removeInfoAndNewline("Hello, world!\n"), "Hello, world!");
  EXPECT_EQ(removeInfoAndNewline(""), "");
}

class CableComparisonTest : public ::testing::Test {
protected:
    Cable cable1, cable2, cable3;

    void SetUp() override {
        // Initialize cables with different properties
        cable1 = {1, 100, 200, 1, 1, 1, 1, 5000, "SN1", "Cable1", "Alpha", JTAG, FTDI};
        cable2 = {2, 100, 200, 1, 1, 1, 1, 5000, "SN2", "Cable2", "Beta", JTAG, FTDI};
        cable3 = {1, 100, 200, 1, 1, 1, 1, 5000, "SN3", "Cable3", "Alpha", JTAG, FTDI};
    }
};

TEST_F(CableComparisonTest, CompareByName) {
  CompareCable compare;
  // cable1 and cable3 have the same name, but different indexes
  EXPECT_FALSE(compare(cable1, cable3));
  EXPECT_FALSE(compare(cable3, cable1));
}

TEST_F(CableComparisonTest, CompareByIndex) {
  CompareCable compare;
  // cable1 and cable2 have different names
  EXPECT_TRUE(compare(cable1, cable2));
  EXPECT_FALSE(compare(cable2, cable1));
}

TEST_F(CableComparisonTest, CompareIdenticalCables) {
  CompareCable compare;
  Cable cable4 = cable1; // Identical to cable1
  EXPECT_FALSE(compare(cable1, cable4));
  EXPECT_FALSE(compare(cable4, cable1));
}

class MockProgrammingAdapter : public ProgrammingAdapter {
public:
    MOCK_METHOD6(program_fpga, int(const Device&, const std::string&, std::atomic<bool>&, std::ostream*, OutputMessageCallback, ProgressCallback));
    MOCK_METHOD7(program_flash, int(const Device&, const std::string&, std::atomic<bool>&, ProgramFlashOperation, std::ostream*, OutputMessageCallback, ProgressCallback));
    MOCK_METHOD6(program_otp, int(const Device&, const std::string&, std::atomic<bool>&, std::ostream*, OutputMessageCallback, ProgressCallback));
    MOCK_METHOD3(query_fpga_status, int(const Device&, CfgStatus&, std::string&));
};

// Test fixture for ProgrammerTool
class ProgrammerToolTest : public ::testing::Test {
protected:
  MockProgrammingAdapter mockAdapter;
  ProgrammerTool programmerTool{&mockAdapter};
  std::string bitfile; // Store the dummy test bitfile path
  // This function creates a test bitfile for the test
  void CreateTestBitfile(const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    // You can write some content to the file if needed
    // For simplicity, we'll just create an empty file here
  }

  // This is called before each test case
  void SetUp() override {
    // Generate a unique test bitfile name for each test
    bitfile = std::string("test_bitfile_") + ::testing::UnitTest::GetInstance()->current_test_info()->name() + std::string(".bit");
    CreateTestBitfile(bitfile);
  }
  // This is called after each test case
  void TearDown() override {
    if (!bitfile.empty()) {
      std::remove(bitfile.c_str());
    }
  }
};

TEST_F(ProgrammerToolTest, ConstructorNullAdapter) {
  // Construct ProgrammerTool with a null adapter
  ProgrammingAdapter* nullAdapter = nullptr;
  // Expect throw exception
  EXPECT_ANY_THROW(ProgrammerTool programmerTool(nullAdapter));
}


TEST_F(ProgrammerToolTest, ProgramFpgaBitfileNotFound) {
  Device device;
  const std::string bitfile = "non_existent.bit";
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;

  EXPECT_CALL(mockAdapter, program_fpga(_, _, _, _, _, _))
    .Times(0); // Expect no calls to program_fpga

  int statusCode = programmerTool.program_fpga(device, bitfile, stop, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::BitfileNotFound);
}

TEST_F(ProgrammerToolTest, ProgramFpgaFileExistsAndReturnNoError) {
  Device device;
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;

  // Mock the behavior of the ProgrammingAdapter to return a specific status code
  EXPECT_CALL(mockAdapter, program_fpga(_, _, _, _, _, _))
    .WillOnce(Return(ProgrammerErrorCode::NoError));

  int statusCode = programmerTool.program_fpga(device, bitfile, stop, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::NoError);
}

TEST_F(ProgrammerToolTest, ProgramFlashBitfileNotFound) {
  Device device;
  const std::string bitfile = "non_existent.bit";
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;
  ProgramFlashOperation modes = ProgramFlashOperation::Program;

  EXPECT_CALL(mockAdapter, program_flash(_, _, _, _, _, _, _))
    .Times(0); // Expect no calls to program_fpga

  int statusCode = programmerTool.program_flash(device, bitfile, stop, modes, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::BitfileNotFound);
}

TEST_F(ProgrammerToolTest, ProgramFlashFileExistsAndReturnNoError) {
  Device device;
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;
  ProgramFlashOperation modes = ProgramFlashOperation::Program;

  // Mock the behavior of the ProgrammingAdapter to return a specific status code
  EXPECT_CALL(mockAdapter, program_flash(_, _, _, _, _, _, _))
    .WillOnce(Return(ProgrammerErrorCode::NoError));

  int statusCode = programmerTool.program_flash(device, bitfile, stop, modes, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::NoError);
}

TEST_F(ProgrammerToolTest, ProgramOtpBitfileNotFound) {
  Device device;
  const std::string bitfile = "non_existent.bit";
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;

  EXPECT_CALL(mockAdapter, program_otp(_, _, _, _, _, _))
    .Times(0); // Expect no calls to program_fpga

  int statusCode = programmerTool.program_otp(device, bitfile, stop, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::BitfileNotFound);
}

TEST_F(ProgrammerToolTest, ProgramOtpFileExistsAndReturnNoError) {
  Device device;
  std::atomic<bool> stop{false};
  std::ostringstream outputStream;

  // Mock the behavior of the ProgrammingAdapter to return a specific status code
  EXPECT_CALL(mockAdapter, program_otp(_, _, _, _, _, _))
    .WillOnce(Return(ProgrammerErrorCode::NoError));

  int statusCode = programmerTool.program_otp(device, bitfile, stop, &outputStream, nullptr, nullptr);
  EXPECT_EQ(statusCode, ProgrammerErrorCode::NoError);
}

TEST_F(ProgrammerToolTest, QueryFpgaStatusSuccess) {
  Device device;
  CfgStatus cfgStatus;
  std::string outputMessage;

  // Set up expectations for the mock ProgrammingAdapter
  EXPECT_CALL(mockAdapter, query_fpga_status(_, _, _))
    .WillOnce(DoAll(
      SetArgReferee<1>(CfgStatus{ true, false}),
      SetArgReferee<2>(std::string("Test output message")),
      Return(ProgrammerErrorCode::NoError)
    ));

  int statusCode = programmerTool.query_fpga_status(device, cfgStatus, outputMessage);

  EXPECT_EQ(statusCode, ProgrammerErrorCode::NoError);
  EXPECT_EQ(cfgStatus.cfgDone, true);
  EXPECT_EQ(cfgStatus.cfgError, false);
}

} // end anoymous namespace