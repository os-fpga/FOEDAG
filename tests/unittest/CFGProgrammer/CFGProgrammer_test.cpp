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
#include "Configuration/HardwareManager/HardwareManager.h"
#include "Configuration/HardwareManager/OpenocdAdapter.h"
#include "Configuration/HardwareManager/OpenocdHelper.h"

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

TEST(ErrorMessageTest, ExistingErrorCode) {
  // Test when the error code exists in the map
  std::string errorMessage = GetErrorMessage(0);
  EXPECT_EQ(errorMessage, "Success");
}

TEST(ErrorMessageTest, NonExistingErrorCode) {
  // Test when the error code exists in the map
  std::string errorMessage = GetErrorMessage(-999);
  EXPECT_EQ(errorMessage, "Unknown error code");
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
class ProgrammerAPI : public ::testing::Test {
protected:
    void SetUp() override {
      // Initialize the library before each test
      #ifndef NDEBUG
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
  Cable cable;
  std::vector<Device> devices;
  int errorCode = ListDevices(cable, devices);
  EXPECT_EQ(devices.size(), 0);
  EXPECT_EQ(errorCode, ProgrammerErrorCode::CableNotFound);
}

TEST_F(ProgrammerAPI, GetFpgaStatusTest_ReturnsCableNotSupportedWhenCableIsDefaultValue) {
  // expect no cable is connected in CI testing environment
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
#ifndef NDEBUG
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
  Cable cable;
  Device device;
  ProgramFlashOperation modes = {ProgramFlashOperation::BlankCheck|
                                 ProgramFlashOperation::Erase|
                                 ProgramFlashOperation::Program|
                                 ProgramFlashOperation::Verify};
  std::atomic<bool> stop{false};
};


TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFpgaCableNotFoundTest) {
  int expected = ProgrammerErrorCode::CableNotFound;
  int actual = ProgramFpga(cable, device, bitfile, stop, nullptr, nullptr, nullptr);
  EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramFlashCableNotFoundTest) {
  int expected = ProgrammerErrorCode::CableNotFound;
  int actual = ProgramFlash(cable, device, bitfile, stop, modes, nullptr, nullptr, nullptr);
  EXPECT_EQ(expected, actual);
}

TEST_F(ProgrammerAPI_ProgramFlashAndFpga, ProgramOtpCableNotFoundTest) {
  int expected = ProgrammerErrorCode::CableNotFound;
  int actual = ProgramOTP(cable, device, bitfile, stop, nullptr, nullptr, nullptr);
  EXPECT_EQ(expected, actual);
}

TEST(InitLibraryTest, EmptyPath) {
  std::string emptyPath = "";
  int result = InitLibrary(emptyPath);
  EXPECT_EQ(result, ProgrammerErrorCode::OpenOCDExecutableNotFound);
}

TEST(InitLibraryTest, NonExistentPath) {
  std::string nonExistentPath = "path/to/nonexistent/openocd";
  int result = InitLibrary(nonExistentPath);
  EXPECT_EQ(result, ProgrammerErrorCode::OpenOCDExecutableNotFound);
}

TEST(InitLibraryTest, ValidPath) {
#ifndef NDEBUG
    std::string validPath = "dbuild/bin/openocd";
#else
    std::string validPath = "build/bin/openocd";
#endif
  int result = InitLibrary(validPath);
  EXPECT_EQ(result, ProgrammerErrorCode::NoError);
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
// Mocking the JtagAdapter class for testing HardwareManager
class MockJtagAdapter : public JtagAdapter {
 public:
  MOCK_METHOD1(scan, std::vector<uint32_t>(const Cable& cable));
};

class HardwareManagerTest : public ::testing::Test {
 protected:
  MockJtagAdapter mockAdapter;
  HardwareManager hardwareManager{&mockAdapter};
};

TEST_F(HardwareManagerTest, ConstructorNullAdapter) {
  // Construct ProgrammerTool with a null adapter
  JtagAdapter* nullAdapter = nullptr;
  // Expect throw exception
  EXPECT_ANY_THROW(HardwareManager hardwareManager(nullAdapter));
}

TEST_F(HardwareManagerTest, GetCablesTest) {
  std::vector<Cable> cables = hardwareManager.get_cables();
  EXPECT_EQ(cables.size(), 0);
}

TEST_F(HardwareManagerTest, IsCableExistsTest) {
  Cable cable;
  bool exists = hardwareManager.is_cable_exists(1);
  EXPECT_FALSE(exists);
  exists = hardwareManager.is_cable_exists("2", true);
  EXPECT_FALSE(exists);
  exists = hardwareManager.is_cable_exists("RsFtdi_1_8", false);
  EXPECT_FALSE(exists);
  exists = hardwareManager.is_cable_exists("RsFtdi_1_9", false, cable);
  EXPECT_FALSE(exists);
}

TEST_F(HardwareManagerTest, GetTapsTest) {
  Cable cable;
  EXPECT_CALL(mockAdapter, scan(_)).WillOnce(Return(std::vector<uint32_t>{0x1000563d, 0x10000db3}));
  std::vector<Tap> taps = hardwareManager.get_taps(cable);
  EXPECT_EQ(taps.size(), 2);
}

TEST_F(HardwareManagerTest, GetDevices) {
  std::vector<Device> devices;
  ON_CALL(mockAdapter, scan(_)).WillByDefault(Return(std::vector<uint32_t>{}));
  devices = hardwareManager.get_devices();
  EXPECT_EQ(devices.size(), 0);

  ON_CALL(mockAdapter, scan(_)).WillByDefault(Return(std::vector<uint32_t>{}));
  devices.clear();
  devices = hardwareManager.get_devices(1);
  EXPECT_EQ(devices.size(), 0);

  ON_CALL(mockAdapter, scan(_)).WillByDefault(Return(std::vector<uint32_t>{}));
  devices.clear();
  devices = hardwareManager.get_devices("RsFtdi_1_8", false);
  EXPECT_EQ(devices.size(), 0);
}

TEST_F(HardwareManagerTest, GetDevicesWithCableInput) {
  Cable cable;
  std::vector<Device> devices;
  EXPECT_CALL(mockAdapter, scan(_)).WillRepeatedly(Return(std::vector<uint32_t>{0x1000563d, 0x10000db3}));
  devices.clear();
  devices = hardwareManager.get_devices(cable);
  EXPECT_EQ(devices.size(), 2);
}

TEST_F(HardwareManagerTest, FindDevice) {
  Device device;
  std::vector<Tap> taplist;
  bool found = hardwareManager.find_device("RsFtdi", 1, device, taplist, true);
  EXPECT_FALSE(found);
}

TEST_F(HardwareManagerTest, GetDeviceDB) {
  auto db = hardwareManager.get_device_db();
  EXPECT_EQ(db.size(), 2);
  EXPECT_EQ(db[0].name, "Gemini");
  EXPECT_EQ(db[0].idcode, 0x1000563d);
  EXPECT_EQ(db[0].irlength, 5);
  EXPECT_EQ(db[0].irmask, 0xffffffff);
  EXPECT_EQ(db[0].type, GEMINI);
}

// TEST(OpenocdAdapterTest, ScanTest) {
//   // Arrange
//   FOEDAG::Cable mockCable;
//   // MockJtagAdapter mockJtagAdapter;
//   std::vector<uint32_t> expectedIdcodes = {0x03df1d81, 0x0692602f};

//   // // Expectations
//   // EXPECT_CALL(mockJtagAdapter, scan(_))
//   //     .WillOnce(testing::Return(expectedIdcodes));

//   // Create OpenocdAdapter instance with mock JtagAdapter
//   FOEDAG::OpenocdAdapter adapter("openocd");
//   // adapter.set_jtag_adapter(&mockJtagAdapter);

//   // Act
//   std::vector<uint32_t> idcodes = adapter.scan(mockCable);

//   // Assert
//   EXPECT_EQ(idcodes, expectedIdcodes);
// }

TEST(CheckRegexTest, MatchCaseInsensitive) {
  // Test when the regex pattern matches, case-insensitive
  std::vector<std::string> output;
  bool result = OpenocdAdapter::check_regex("[Rs] command Error 101", R"(\[RS\] Command error (\d+)\.*)", output);

  EXPECT_TRUE(result);
  EXPECT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "101");
}

TEST(CheckRegexTest, NoMatch) {
  std::vector<std::string> output;
  bool result = OpenocdAdapter::check_regex("Testing 123", "pattern", output);

  EXPECT_FALSE(result);
  EXPECT_TRUE(output.empty());
}

TEST(CheckOutputTest, CmdProgressMatch) {
  // Test when the input string matches CMD_PROGRESS
  std::string input = "Progress 50.0% (5/10 bytes)";
  std::vector<std::string> output;
  CommandOutputType result = OpenocdAdapter::check_output(input, output);

  EXPECT_EQ(result, CMD_PROGRESS);
  ASSERT_EQ(output.size(), 3);
  EXPECT_EQ(output[0], "50.0");
  EXPECT_EQ(output[1], "5");
  EXPECT_EQ(output[2], "10");
}

TEST(CheckOutputTest, CmdErrorMatch) {
  // Test when the input string matches CMD_ERROR
  std::string input = "[RS] Command error 123 - Error message";
  std::vector<std::string> output;
  CommandOutputType result = OpenocdAdapter::check_output(input, output);

  EXPECT_EQ(result, CMD_ERROR);
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "123");
}

TEST(CheckOutputTest, NoMatch) {
  std::string input = "Some random text";
  std::vector<std::string> output;
  CommandOutputType result = OpenocdAdapter::check_output(input, output);

  EXPECT_EQ(result, NOT_OUTPUT);
  EXPECT_TRUE(output.empty());
}

TEST(TransportToStringTest, JTAGConversion) {
  // Test converting JTAG transport type
  TransportType transport = TransportType::JTAG;
  std::string result = convert_transport_to_string(transport, "default");
  EXPECT_EQ(result, "jtag");
}

TEST(TransportToStringTest, DefaultConversion) {
  // Test converting an unknown transport type (should return default)
  TransportType transport = TransportType(99);
  std::string result = convert_transport_to_string(transport, "default");
  EXPECT_EQ(result, "default");
}

TEST(CableConfigTest, FTDIConfig) {
  // Test FTDI cable configuration
  Cable cable;
  cable.cable_type = CableType::FTDI;
  cable.vendor_id = 0x1234;
  cable.product_id = 0x5678;
  cable.serial_number = "FT12345";
  cable.speed = 1000;  // 1 Mbps
  cable.transport = TransportType::JTAG;

  std::string result = build_cable_config(cable);

  // Define the expected FTDI configuration string
  std::string expected = " -c \"adapter driver ftdi;"
      "ftdi vid_pid 0x1234 0x5678;"
      "ftdi layout_init 0x0c08 0x0f1b;"
      "adapter serial FT12345;"
      "adapter speed 1000;"
      "transport select jtag;"
      "telnet_port disabled;"
      "gdb_port disabled;\"";
  
  EXPECT_EQ(result, expected);
}

TEST(CableConfigTest, JLINKConfig) {
  // Test JLINK cable configuration
  Cable cable;
  cable.cable_type = CableType::JLINK;
  cable.speed = 500;  // 500 Kbps
  cable.transport = TransportType::JTAG;

  std::string result = build_cable_config(cable);

  // Define the expected JLINK configuration string
  std::string expected = " -c \"adapter driver jlink;"
      "adapter speed 500;"
      "transport select jtag;"
      "telnet_port disabled;"
      "gdb_port disabled;\"";
  
  EXPECT_EQ(result, expected);
}

TEST(TapConfigTest, EmptyTapList) {
  // Test with an empty taplist
  std::vector<Tap> taplist;
  std::string result = build_tap_config(taplist);

  // The result should be an empty string
  EXPECT_EQ(result, "");
}

TEST(TapConfigTest, NonEmptyTapList) {
  // Test with a non-empty taplist
  std::vector<Tap> taplist;
  taplist.push_back({1, 0xabcd, 5});
  taplist.push_back({2, 0x1234, 3});
  taplist.push_back({3, 0x5678, 6});

  std::string result = build_tap_config(taplist);

  // Define the expected tap configuration string
  std::string expected = " -c \"jtag newtap tap1 tap -irlen 5 -expected-id 0xabcd;"
      "jtag newtap tap2 tap -irlen 3 -expected-id 0x1234;"
      "jtag newtap tap3 tap -irlen 6 -expected-id 0x5678;\"";
  
  EXPECT_EQ(result, expected);
}

TEST(TargetConfigTest, GeminiDevice) {
  // Test with a Gemini device
  Device device;
  device.type = DeviceType::GEMINI;
  device.index = 1;
  device.tap.index = 2;
  std::string result = build_target_config(device);

  // Define the expected target configuration string for Gemini
  std::string expected = " -c \"target create gemini1 riscv -endian little -chain-position tap2.tap;\""
      " -c \"pld device gemini gemini1\"";
  
  EXPECT_EQ(result, expected);
}

TEST(TargetConfigTest, VirgoDevice) {
  // Test with a Virgo device
  Device device;
  device.type = DeviceType::VIRGO;
  device.index = 4;
  device.tap.index = 1;
  std::string result = build_target_config(device);

  // Define the expected target configuration string for Virgo
  std::string expected = " -c \"target create gemini4 riscv -endian little -chain-position tap1.tap;\""
      " -c \"pld device gemini gemini4\"";
  
  EXPECT_EQ(result, expected);
}

TEST(TargetConfigTest, OCLADevice) {
  // Test with an OCLA device
  Device device;
  device.type = DeviceType::OCLA;
  device.index = 5;
  device.tap.index = 5;
  std::string result = build_target_config(device);

  // Define the expected target configuration string for OCLA
  std::string expected = " -c \"target create gemini5 testee -chain-position tap5.tap;\"";
  
  EXPECT_EQ(result, expected);
}


} // end anoymous namespace
