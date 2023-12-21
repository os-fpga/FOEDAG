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

#include "OpenocdAdapter.h"

#include <filesystem>
#include <map>
#include <regex>
#include <sstream>

#include "Configuration/CFGCommon/CFGCommon.h"
#include "Configuration/HardwareManager/HardwareManager.h"
#include "Configuration/HardwareManager/OpenocdHelper.h"
#include "Configuration/Programmer/Programmer_error_code.h"
#include "Configuration/Programmer/Programmer_helper.h"
namespace FOEDAG {

OpenocdAdapter::OpenocdAdapter(std::string openocd) : m_openocd(openocd) {}

OpenocdAdapter::~OpenocdAdapter() {}

std::vector<uint32_t> OpenocdAdapter::scan(const Cable& cable) {
  std::vector<uint32_t> idcode_array;
  std::string line;
  std::cmatch matches;
  std::string output;

  const std::string pattern(
      R"((\d+) +(\w+.\w+) +([YN]) +(0x[0-9a-f]+) +(0x[0-9a-f]+) +(\d+) +(0x[0-9a-f]+) +(0x[0-9a-f]+))");

  // OpenOCD "scan_chain" command output text example:-
  //    TapName            Enabled IdCode     Expected   IrLen IrCap IrMask
  // -- ------------------ ------- ---------- ---------- ----- ----- ------
  //  0 omap5912.dsp          Y    0x03df1d81 0x03df1d81    38 0x01  0x03
  //  1 omap5912.arm          Y    0x0692602f 0x0692602f     4 0x01  0x0f
  //  2 omap5912.unknown      Y    0x00000000 0x00000000     8 0x01  0x03
  //  3 auto0.tap             Y    0x20000913 0x00000000     5 0x01  0x03

  // use "scan_chain" cmd to collect tap ids
  CFG_ASSERT_MSG(execute(cable, "scan_chain", output) == 0, "cmdexec error: %s",
                 output.c_str());
  std::stringstream ss(output);

  while (std::getline(ss, line)) {
    if (std::regex_search(line.c_str(), matches,
                          std::regex(pattern, std::regex::icase)) == true) {
      uint32_t idcode = (uint32_t)CFG_convert_string_to_u64(matches[4]);
      idcode_array.push_back(idcode);
    }
  }

  return idcode_array;
}

bool OpenocdAdapter::check_regex(std::string str, std::string pattern,
                                 std::vector<std::string>& output) {
  std::smatch m;
  int i = 0;

  if (std::regex_search(str, m, std::regex{pattern, std::regex::icase})) {
    output.clear();
    for (auto& s : m) {
      if (i++ > 0) {
        output.push_back(s);
      }
    }
    return true;
  }

  return false;
}

CommandOutputType OpenocdAdapter::check_output(
    std::string str, std::vector<std::string>& output) {
  static std::map<CommandOutputType, std::string> patterns = {
      {CMD_PROGRESS, R"(Progress +(\d+.\d+)% +\((\d+)\/(\d+) +bytes\))"},
      {CMD_ERROR, R"(\[RS\] Command error (\d+)\.*)"},
      {CMD_TIMEOUT, R"(\[RS\] Timed out waiting for task to complete\.)"},
      {CBUFFER_TIMEOUT, R"(\[RS\] Circular buffer timed out\.)"},
      {FSBL_BOOT_FAILURE, R"(\[RS\] Failed to load FSBL firmware)"},
      {UNKNOWN_FIRMWARE, R"(\[RS\] Unknown firmware)"},
      {CONFIG_ERROR,
       R"(\[RS\] FPGA fabric configuration error \(cfg_done *= *(\d+), *cfg_error *= *(\d+)\))"},
      {CONFIG_SUCCESS, R"(\[RS\] Configured FPGA fabric successfully)"},
      {INVALID_BITSTREAM,
       R"(\[RS\] Unsupported UBI header version ([0-9a-f]+))"},
  };

  for (auto const& [key, pat] : patterns) {
    if (check_regex(str, pat, output)) {
      return key;
    }
  }

  return NOT_OUTPUT;
}

void OpenocdAdapter::update_taplist(const std::vector<Tap>& taplist) {
  m_taplist = taplist;
}

int OpenocdAdapter::program_fpga(const Device& device,
                                 const std::string& bitfile,
                                 std::atomic<bool>& stop,
                                 std::ostream* outStream,
                                 OutputMessageCallback callbackMsg,
                                 ProgressCallback callbackProgress) {
  int statusCode = ProgrammerErrorCode::NoError;
  CFG_ASSERT(std::filesystem::exists(m_openocd));
  std::string openocd_command =
      create_openocd_command("fpga", device, m_taplist, bitfile, m_openocd);

  // run the command
  int res = CFG_execute_cmd_with_callback(
      openocd_command, m_last_output, outStream, std::regex{}, stop, nullptr,
      [&](const std::string& line) {
        std::vector<std::string> data{};
        switch (check_output(line, data)) {
          case CMD_PROGRESS: {
            double percent = std::strtod(data[0].c_str(), nullptr);
            std::ostringstream stream;
            if (callbackMsg != nullptr) {
              if (percent < 100) {
                callbackMsg(data[0]);
              } else {
                callbackMsg("99.99");
              }
            }
            if (callbackProgress != nullptr) {
              if (percent < 100) {
                callbackProgress(data[0]);
              } else {
                callbackProgress("99.99");
              }
            }
            break;
          }
          case CMD_ERROR:
            statusCode = std::stoi(data[0]);
            break;
          case CMD_TIMEOUT:
            statusCode = ProgrammerErrorCode::CmdTimeout;
            break;
          case CBUFFER_TIMEOUT:
            statusCode = ProgrammerErrorCode::BufferTimeout;
            break;
          case CONFIG_ERROR:
            statusCode = ProgrammerErrorCode::ConfigError;
            break;
          case CONFIG_SUCCESS:
            if (callbackMsg != nullptr) callbackMsg("100.00");
            if (callbackProgress != nullptr) callbackProgress("100.00");
            break;
          case UNKNOWN_FIRMWARE:
            statusCode = ProgrammerErrorCode::UnknownFirmware;
            break;
          case FSBL_BOOT_FAILURE:
            statusCode = ProgrammerErrorCode::FsblBootFail;
            break;
          default:
            // callbackMsg(line);
            break;
        }
      });

  if (res != 0) {
    statusCode = ProgrammerErrorCode::GeneralCmdError;
  }
  return statusCode;
}
int OpenocdAdapter::program_flash(
    const Device& device, const std::string& bitfile, std::atomic<bool>& stop,
    ProgramFlashOperation modes, std::ostream* outStream,
    OutputMessageCallback callbackMsg, ProgressCallback callbackProgress) {
  int statusCode = ProgrammerErrorCode::NoError;
  CFG_ASSERT(std::filesystem::exists(m_openocd));
  std::string openocd_command =
      create_openocd_command("flash", device, m_taplist, bitfile, m_openocd);
  ;
  // run the command
  int res = CFG_execute_cmd_with_callback(
      openocd_command, m_last_output, outStream, std::regex{}, stop, nullptr,
      [&](const std::string& line) {
        std::vector<std::string> data{};
        switch (check_output(line, data)) {
          case CMD_PROGRESS: {
            double percent = std::strtod(data[0].c_str(), nullptr);
            std::ostringstream stream;
            if (callbackMsg != nullptr) {
              if (percent < 100) {
                callbackMsg(data[0]);
              } else {
                callbackMsg("99.99");
              }
            }
            if (callbackProgress != nullptr) {
              if (percent < 100) {
                callbackProgress(data[0]);
              } else {
                callbackProgress("99.99");
              }
            }
            break;
          }
          case CMD_ERROR:
            statusCode = std::stoi(data[0]);
            break;
          case CMD_TIMEOUT:
            statusCode = ProgrammerErrorCode::CmdTimeout;
            break;
          case CBUFFER_TIMEOUT:
            statusCode = ProgrammerErrorCode::BufferTimeout;
            break;
          case CONFIG_ERROR:
            statusCode = ProgrammerErrorCode::ConfigError;
            break;
          case CONFIG_SUCCESS:
            if (callbackMsg != nullptr) callbackMsg("100.00");
            if (callbackProgress != nullptr) callbackProgress("100.00");
            break;
          case UNKNOWN_FIRMWARE:
            statusCode = ProgrammerErrorCode::UnknownFirmware;
            break;
          case FSBL_BOOT_FAILURE:
            statusCode = ProgrammerErrorCode::FsblBootFail;
            break;
          default:
            // callbackMsg(line);
            break;
        }
      });

  if (res != 0) {
    statusCode = ProgrammerErrorCode::GeneralCmdError;
  }
  return statusCode;
}
int OpenocdAdapter::program_otp(const Device& device,
                                const std::string& bitfile,
                                std::atomic<bool>& stop,
                                std::ostream* outStream,
                                OutputMessageCallback callbackMsg,
                                ProgressCallback callbackProgress) {
  int statusCode = ProgrammerErrorCode::NoError;
  CFG_ASSERT(std::filesystem::exists(m_openocd));
  std::string openocd_command =
      create_openocd_command("otp", device, m_taplist, bitfile, m_openocd);
  // run the openocd_command
  int res = CFG_execute_cmd_with_callback(
      openocd_command, m_last_output, outStream, std::regex{}, stop, nullptr,
      [&](const std::string& line) {
        std::vector<std::string> data{};
        switch (check_output(line, data)) {
          case CMD_PROGRESS: {
            double percent = std::strtod(data[0].c_str(), nullptr);
            std::ostringstream stream;
            if (callbackMsg != nullptr) {
              if (percent < 100) {
                callbackMsg(data[0]);
              } else {
                callbackMsg("99.99");
              }
            }
            if (callbackProgress != nullptr) {
              if (percent < 100) {
                callbackProgress(data[0]);
              } else {
                callbackProgress("99.99");
              }
            }
            break;
          }
          case CMD_ERROR:
            statusCode = std::stoi(data[0]);
            break;
          case CMD_TIMEOUT:
            statusCode = ProgrammerErrorCode::CmdTimeout;
            break;
          case CBUFFER_TIMEOUT:
            statusCode = ProgrammerErrorCode::BufferTimeout;
            break;
          case CONFIG_ERROR:
            statusCode = ProgrammerErrorCode::ConfigError;
            break;
          case CONFIG_SUCCESS:
            if (callbackMsg != nullptr) callbackMsg("100.00");
            if (callbackProgress != nullptr) callbackProgress("100.00");
            break;
          case UNKNOWN_FIRMWARE:
            statusCode = ProgrammerErrorCode::UnknownFirmware;
            break;
          case FSBL_BOOT_FAILURE:
            statusCode = ProgrammerErrorCode::FsblBootFail;
            break;
          default:
            // callbackMsg(line);
            break;
        }
      });

  if (res != 0) {
    statusCode = ProgrammerErrorCode::GeneralCmdError;
  }
  return statusCode;
}

int OpenocdAdapter::query_fpga_status(const Device& device,
                                      CfgStatus& cfgStatus,
                                      std::string& outputString) {
  std::ostringstream ss;
  std::atomic<bool> stopCommand{false};
  std::string cmdOutput, outputMsg;
  CFG_ASSERT(std::filesystem::exists(m_openocd));

  ss << " -l /dev/stdout"  //<-- not windows friendly
     << " -d2";

  ss << build_cable_config(device.cable) << build_tap_config(m_taplist)
     << build_target_config(device);

  std::string cmd = "gemini status 1 fpga ";

  ss << " -c \"init\"";
  ss << " -c \"" << cmd << "\"";
  ss << " -c \"exit\"";

  int result = CFG_execute_cmd("OPENOCD_DEBUG_LEVEL=-3 " + m_openocd + ss.str(),
                               cmdOutput, nullptr, stopCommand);
  outputString = cmdOutput;
  if (result != 0) {
    return ProgrammerErrorCode::GeneralCmdError;  // general cmdline error
  } else {
    bool statusFound = false;
    cfgStatus = extractStatus(cmdOutput, statusFound);
    if (!statusFound) {
      return ProgrammerErrorCode::ParseFpgaStatusError;  // fail to parse status
    }
  }
  return result;
}

int OpenocdAdapter::execute(const Cable& cable, std::string cmd,
                            std::string& output) {
  std::atomic<bool> stop = false;
  std::ostringstream ss;

  CFG_ASSERT(std::filesystem::exists(m_openocd));

  ss << " -l /dev/stdout"  //<-- not windows friendly
     << " -d2";
  ss << build_cable_config(cable);
  ss << " -c \"init\"";
  ss << " -c \"" << cmd << "\"";
  ss << " -c \"exit\"";

  // run the command
  int res = CFG_execute_cmd("OPENOCD_DEBUG_LEVEL=-3 " + m_openocd + ss.str(),
                            output, nullptr, stop);
  return res;
}

}  // namespace FOEDAG