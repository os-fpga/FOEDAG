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

#include "Programmer.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>  // for std::stringstream
#include <thread>
#include <unordered_set>

namespace FOEDAG {

std::string buildFpgaProgramCommand(const std::string& bitstream_file,
                                    const std::string& config_file,
                                    int pld_index) {
  std::string cmd = " -f " + config_file + " -c \"gemini load " +
                    std::to_string(pld_index) + " fpga " + bitstream_file +
                    " -p 1\"" + " -l /dev/stdout -c exit";
  return cmd;
}

std::string buildFpgaQueryStatusCommand(const std::string config_file,
                                        int pld_index) {
  std::string cmd = " -f " + config_file + " -c \"gemini status " +
                    std::to_string(pld_index) + "\"" +
                    " -l /dev/stdout -c exit";
  return cmd;
}

std::string buildListDeviceCommand(const std::string config_file) {
  std::string cmd =
      " -f " + config_file + " -c \"gemini list\"" + " -l /dev/stdout -c exit";
  return cmd;
}

std::string buildFlashProgramCommand(const std::string& bitstream_file,
                                     const std::string& config_file,
                                     int pld_index, bool doErase,
                                     bool doBlankCheck, bool doProgram,
                                     bool doVerify) {
  std::string cmd = " -f " + config_file + " -c \"gemini load " +
                    std::to_string(pld_index) + " flash " + bitstream_file +
                    " -p 1\"" + " -l /dev/stdout -c exit";
  return cmd;
}

ProgrammerCommand parseProgrammerCommand(const CFGCommon_ARG* cmdarg,
                                         std::filesystem::path configFilePath) {
  ProgrammerCommand programmerCmd;

  if (cmdarg->arg->m_help) {
    programmerCmd.name = "help";
    return programmerCmd;
  }

  if (cmdarg->arg->m_args.size() < 1) {
    CFG_POST_ERR("Not enough arguments for programmer. ");
    programmerCmd.is_error = true;
    return programmerCmd;
  }

  auto itSubcmd =
      std::find(std::begin(programmer_subcmd), std::end(programmer_subcmd),
                cmdarg->arg->m_args[0]);

  if (itSubcmd == std::end(programmer_subcmd)) {
    CFG_POST_ERR("Invalid command for %s. ", cmdarg->arg->m_args[0].c_str());
    programmerCmd.is_error = true;
    return programmerCmd;
  }

  programmerCmd.name = cmdarg->arg->m_args[0];
  const std::filesystem::path openOcdExecPath = cmdarg->toolPath;
  auto arg = std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg->arg);
  const std::string openocd = openOcdExecPath.string();
  const std::string configFile = configFilePath.string();
  if (programmerCmd.name == "fpga_config" || programmerCmd.name == "flash") {
    std::string bitstreamFile = arg->m_args[1];
    std::error_code ec;
    if (cmdarg->compilerName != "dummy") {
      if (!std::filesystem::exists(bitstreamFile, ec)) {
        CFG_POST_ERR("Cannot find bitstream file: %s. %s ",
                     bitstreamFile.c_str(), (ec ? ec.message().c_str() : ""));
        programmerCmd.is_error = true;
        return programmerCmd;
      }
    }

    if (programmerCmd.name == "fpga_config") {
      programmerCmd.executable_cmd =
          openocd +
          buildFpgaProgramCommand(bitstreamFile, configFile, arg->index);
    } else if (programmerCmd.name == "flash") {
      auto operations = parseOperationString(arg->operations);
      bool doErase = isOperationRequested("erase", operations);
      bool doBlankCheck = isOperationRequested("blankcheck", operations);
      bool doProgram = isOperationRequested("program", operations);
      bool doVerify = isOperationRequested("verify", operations);
      // TODO: doVerify, doBlankCheck, doErase set to false
      // there are not supported yet
      doErase = doBlankCheck = doVerify = false;
      programmerCmd.executable_cmd =
          openocd + buildFlashProgramCommand(bitstreamFile, configFile,
                                             arg->index, doErase, doBlankCheck,
                                             doProgram, doVerify);
    }
  } else if (programmerCmd.name == "fpga_status") {
    programmerCmd.executable_cmd =
        openocd + buildFpgaQueryStatusCommand(configFile, arg->index);
  } else if (programmerCmd.name == "list_devices") {
    programmerCmd.executable_cmd = openocd + buildListDeviceCommand(configFile);
  }
  return programmerCmd;
}

std::vector<std::string> parseOperationString(const std::string& operations) {
  std::vector<std::string> result;
  std::unordered_set<std::string> uniqueValues;  // To store unique values
  std::stringstream ss(operations);
  std::string token;
  // the operations string is a comma separated list or | separated list of
  // operations
  while (std::getline(ss, token, ',')) {
    std::stringstream ss2(token);
    std::string subToken;

    while (std::getline(ss2, subToken, '|')) {
      if (uniqueValues.find(subToken) ==
          uniqueValues.end()) {  // Check if value already exists
        // remove whitespace
        subToken.erase(
            std::remove_if(std::begin(subToken), std::end(subToken),
                           [](unsigned char ch) { return std::isspace(ch); }),
            std::end(subToken));
        // convert to lowercase
        std::transform(std::begin(subToken), std::end(subToken),
                       std::begin(subToken), ::tolower);
        result.push_back(subToken);
        uniqueValues.insert(subToken);  // Add value to the unique set
      }
    }
  }

  return result;
}

bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations) {
  return std::find(supportedOperations.begin(), supportedOperations.end(),
                   operation) != supportedOperations.end();
}

void programmer_entry(const CFGCommon_ARG* cmdarg) {
  if (cmdarg->compilerName == "dummy") {
    auto arg = std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg->arg);
    std::filesystem::path configFile = arg->config;
    auto programmerCmd = parseProgrammerCommand(cmdarg, configFile);
    if (programmerCmd.name == "fpga_config") {
      for (int i = 10; i <= 100; i += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CFG_POST_MSG("<test> program fpga - %d %%", i);
      }
    } else if (programmerCmd.name == "fpga_status") {
      CFG_POST_MSG("<test> FPGA configuration status : Done");
    } else if (programmerCmd.name == "list_devices") {
      CFG_POST_MSG("<test>        Device               ID           IRLen ");
      CFG_POST_MSG(
          "<test> -----  -------------------- ------------ ----------");
      CFG_POST_MSG("<test> Found  0 Gemini             0x1000AABB   5");
      CFG_POST_MSG("<test> Found  1 Gemini             0x2000CCDD   5");
    } else if (programmerCmd.name == "flash") {
      auto operations = parseOperationString(arg->operations);
      if (isOperationRequested("erase", operations)) {
        CFG_POST_MSG("<test> Erasing flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> erase flash - %d %% ", i);
        }
      }
      if (isOperationRequested("blankcheck", operations)) {
        CFG_POST_MSG("<test> Flash blank check start ...");
        CFG_POST_MSG("<test> Flash blank check complete.");
      }
      if (isOperationRequested("program", operations)) {
        CFG_POST_MSG("<test> Programming flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> program flash - %d %% ", i);
        }
      }
      if (isOperationRequested("verify", operations)) {
        CFG_POST_MSG("<test> Flash verification start ...");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> flash verified- %d %% ", i);
        }
      }
    } else {
      CFG_POST_ERR("Invalid subcommand. ");
      return;
    }
  } else {
    const std::filesystem::path openOcdExecPath = cmdarg->toolPath;
    const std::filesystem::path configFileSearchPath = cmdarg->searchPath;
    std::error_code ec;
    if (!std::filesystem::exists(openOcdExecPath, ec)) {
      CFG_POST_ERR("Cannot find openocd executable: %s. %s. ",
                   openOcdExecPath.string().c_str(),
                   (ec ? ec.message().c_str() : ""));
      return;
    }
    auto arg = std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg->arg);
    auto configFile = CFG_find_file(arg->config, configFileSearchPath);
    if (configFile.empty()) {
      CFG_POST_ERR("Cannot find config file: %s. ", configFile.c_str());
      return;
    }

    auto programmerCmd = parseProgrammerCommand(cmdarg, configFile);
    if (programmerCmd.name == "help") {
      return;
    } else if (programmerCmd.name.empty() && programmerCmd.is_error) {
      CFG_POST_ERR("Subcommand not provided. ");
      return;
    } else if (programmerCmd.is_error) {
      return;
    }
    int return_code =
        CFG_compiler_execute_cmd(programmerCmd.executable_cmd, "", false);
    if (return_code) {
      CFG_POST_ERR("Failed to execute following command %s. Error code: %d ",
                   programmerCmd.name.c_str(), return_code);
    }
  }
}

bool ListDevices(std::vector<Device>& devices, std::string& outputMsg) {
  devices.clear();
  outputMsg.clear();
  devices.push_back(Device{0, "Gemini", "0x1000563d", 0xffffff, 5, 16384});
  devices.push_back(Device{1, "Gemini+", "0x1000564d", 0xffffff, 5, 16384});

  std::stringstream ss;
  ss << std::left << std::setw(8) << "Device" << std::setw(22) << "ID"
     << std::setw(13) << "IRLen" << std::setw(14) << "Flash Size" << std::endl;

  ss << "------ ---------------------- ------------ ----------" << std::endl;
  ss << std::left << std::setw(8) << "Found" << std::setw(10) << "0 Gemini"
     << std::setw(14) << "0x1000563d" << std::setw(13) << " 5" << std::setw(13)
     << "16384" << std::endl;

  ss << std::left << std::setw(8) << "Found" << std::setw(10) << "1 Gemini+"
     << std::setw(14) << "0x1000564d" << std::setw(13) << " 5" << std::setw(13)
     << "16384" << std::endl;

  outputMsg = ss.str();
  return true;
}

bool GetFpgaStatus(const Device& device, CfgStatus& status) {
  status.cfgDone = true;
  status.cfgError = false;
  return true;
}
int ProgramFpga(const Device& device, const std::string& bitfile,
                const std::string& cfgfile, std::ostream* outStream,
                OutputCallback callbackMsg, ProgressCallback callbackProgress,
                std::atomic<bool>& stop) {
  std::vector<std::string> runMessages{
      "Open On-Chip Debugger 0.12.0+dev-g7c8f503b4 (2023-06-12-08:16)\n",
      "Licensed under GNU GPL v2\n",
      "...\n",
      "Info : [RS] Configuring FPGA fabric...\n",
  };

  for (auto s : runMessages) {
    if (stop) {
      return -1;
    }

    if (outStream != nullptr) {
      *outStream << s;
    }
    if (callbackMsg != nullptr) {
      callbackMsg(s);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  for (int i = 0; i <= 100; i += 10) {
    if (stop) {
      return -1;
    }
    std::string runProgress("Info : [RS] Progress " + std::to_string(i) +
                            ".00% (34816/34816 bytes)\n");
    if (callbackMsg != nullptr) {
      callbackMsg(runProgress);
    }
    if (callbackProgress != nullptr) {
      callbackProgress(double(i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::string endMessage = "Info : [RS] Configured FPGA fabric successfully\n";
  std::string loadedFile = "loaded file" + bitfile + " to pld device " +
                           device.name + " " + std::to_string(device.index) +
                           " in 5s 90381us\n";

  if (outStream != nullptr) {
    *outStream << endMessage << loadedFile;
  }

  if (callbackMsg != nullptr) {
    callbackMsg(endMessage);
    callbackMsg(loadedFile);
  }

  return 0;
}
bool ProgramFlash(const Device& device, const std::string& bitfile,
                  const std::string& cfgfile, Operation modes,
                  std::ostream* outStream, OutputCallback callbackMsg,
                  ProgressCallback callbackProgress, std::atomic<bool>& stop) {
  return 0;
}

}  // namespace FOEDAG
