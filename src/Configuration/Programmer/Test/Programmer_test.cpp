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

#include "Programmer/Programmer.h"

#include <gmock/gmock.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <regex>

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

namespace {
TEST(CFGCommon, test_programmer_compiler_dummy) {
  testing::internal::CaptureStdout();
  CFGCommon_ARG arg;
  arg.projectName = "Foo";
  arg.projectPath = "/foo/bar";
  arg.device = "ptech";
  arg.command = "programmer";
  arg.compilerName = "dummy";
  programmer_entry(&arg);
  std::string output = testing::internal::GetCapturedStdout();
  std::smatch match;
  std::regex patternProjectName(R"(ProjectName\s*:\s*Foo)");
  std::regex patternProjectPath(R"(ProjectPath\s*:\s*/foo/bar)");
  std::regex patternDevice(R"(Device\s*:\s*ptech)");
  std::regex patternCommand(R"(command\s*:\s*programmer)");
  std::regex patternLooping(R"(Looping to test multithread - \d+)");
  EXPECT_TRUE(std::regex_search(output, match, patternProjectName));
  EXPECT_TRUE(std::regex_search(output, match, patternProjectPath));
  EXPECT_TRUE(std::regex_search(output, match, patternDevice));
  EXPECT_TRUE(std::regex_search(output, match, patternCommand));
  EXPECT_TRUE(std::regex_search(output, match, patternLooping));
}

TEST(CFGCommon, test_programmer_not_dummy_openocd_not_exist) {
  testing::internal::CaptureStdout();
  CFGCommon_ARG cmdarg;
  cmdarg.compilerName = "";
  cmdarg.toolPath = "/usr/bin/notexist";
  programmer_entry(&cmdarg);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output,
            "ERROR: Cannot find openocd executable: /usr/bin/notexist. \n");
}

TEST(CFGCommon, test_programmer_not_dummy_bitstream_not_exist) {
  const std::string fake_openocd_exec = "openocd123";
  {
    std::ofstream fake_openocd(fake_openocd_exec);
    fake_openocd << "fake_opencd\n";
  }

  testing::internal::CaptureStdout();
  CFGCommon_ARG cmdarg;
  cmdarg.compilerName = "";
  cmdarg.toolPath = fake_openocd_exec;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAM_DEVICE>();
  auto programmer_arg =
      std::static_pointer_cast<CFGArg_PROGRAM_DEVICE>(cmdarg.arg);
  programmer_arg->bitstream = "fake_bitstream";
  programmer_arg->config = "fake_config";
  programmer_arg->index = 1;
  programmer_entry(&cmdarg);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "ERROR: Cannot find bitstream file: fake_bitstream. \n");

  std::filesystem::remove(fake_openocd_exec);
}

TEST(CFGCommon, test_programmer_not_dummy_config_not_exist) {
  const std::string fake_openocd_exec = "openocd123";
  {
    std::ofstream fake_openocd(fake_openocd_exec);
    fake_openocd << "fake_opencd\n";
  }
  const std::string fake_bitstream_file = "dummy_bit";
  {
    std::ofstream fake_bitstream(fake_bitstream_file);
    fake_bitstream << "0x1 0x2 0x3\n";
  }

  testing::internal::CaptureStdout();
  CFGCommon_ARG cmdarg;
  cmdarg.compilerName = "";
  cmdarg.toolPath = fake_openocd_exec;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAM_DEVICE>();
  auto programmer_arg =
      std::static_pointer_cast<CFGArg_PROGRAM_DEVICE>(cmdarg.arg);
  programmer_arg->bitstream = fake_bitstream_file;
  programmer_arg->config = "fake_config";
  programmer_arg->index = 1;
  programmer_entry(&cmdarg);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "ERROR: Cannot find config file: \n");

  std::filesystem::remove(fake_openocd_exec);
  std::filesystem::remove(fake_bitstream_file);
}

TEST(CFGCommon, test_programmer_not_dummy_fail_programming) {
  const std::string fake_openocd_exec = "openocd123";
  {
    std::ofstream fake_openocd(fake_openocd_exec);
    fake_openocd << "fake_opencd\n";
  }
  const std::string fake_bitstream_file = "dummy_bit";
  {
    std::ofstream fake_bitstream(fake_bitstream_file);
    fake_bitstream << "0x1 0x2 0x3\n";
  }
  const std::string fake_config_file = "dummy_cfg";
  {
    std::ofstream fake_config(fake_config_file);
    fake_config << "make up config data\n";
  }

  testing::internal::CaptureStdout();
  CFGCommon_ARG cmdarg;
  cmdarg.compilerName = "";
  cmdarg.toolPath = fake_openocd_exec;
  cmdarg.arg = std::make_shared<CFGArg_PROGRAM_DEVICE>();
  auto programmer_arg =
      std::static_pointer_cast<CFGArg_PROGRAM_DEVICE>(cmdarg.arg);
  programmer_arg->bitstream = fake_bitstream_file;
  programmer_arg->config = fake_config_file;
  programmer_arg->index = 1;
  programmer_entry(&cmdarg);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(
      output.find("ERROR: Bitstream programming failed. Error code: 127") !=
      std::string::npos);

  std::filesystem::remove(fake_openocd_exec);
  std::filesystem::remove(fake_bitstream_file);
  std::filesystem::remove(fake_config_file);
}

}  // end anonymous namespace
