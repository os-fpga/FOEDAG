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

#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include <atomic>
#include <functional>
#include <regex>

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

static const std::vector<std::string> programmer_subcmd{
    "fpga_config", "fpga_status", "flash", "list_devices"};

struct ProgrammerCommand {
  std::string name;
  std::string executable_cmd;
  bool is_error = false;
};

struct Device {
  int index;
  std::string name;
  std::string jtagId;
  unsigned int mask;
  unsigned int irlength;
  int flashSize;
};

enum class Operation { Erase = 1, BlankCheck = 2, Program = 4, Verify = 8 };

struct CfgStatus {
  bool cfgDone;
  bool cfgError;
};

inline Operation operator|(Operation lhs, Operation rhs) {
  using T = std::underlying_type_t<Operation>;
  return static_cast<Operation>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline Operation operator&(Operation lhs, Operation rhs) {
  using T = std::underlying_type_t<Operation>;
  return static_cast<Operation>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

using ProgressCallback = std::function<void(double)>;
using OutputCallback = std::function<void(std::string)>;

std::string buildFpgaProgramCommand(const std::string& bitstream_file,
                                    const std::string& config_file,
                                    int pld_index);
std::string buildFpgaQueryStatusCommand(const std::string config_file,
                                        int pld_index);
std::string buildListDeviceCommand(const std::string config_file);
std::string buildFlashProgramCommand(const std::string& bitstream_file,
                                     const std::string& config_file,
                                     int pld_index, bool doErase,
                                     bool doBlankCheck, bool doProgram,
                                     bool doVerify);
ProgrammerCommand parseProgrammerCommand(const CFGCommon_ARG* cmdarg,
                                         std::filesystem::path configFile);
std::vector<std::string> parseOperationString(const std::string& operation);
bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations);
void programmer_entry(const CFGCommon_ARG* cmdarg);

// Backend API
bool ListDevices(std::vector<Device>& devices, std::string& outputMsg);
bool GetFpgaStatus(const Device& device, CfgStatus& status);
int ProgramFpga(const Device& device, const std::string& bitfile,
                const std::string& cfgfile, std::ostream* outStream,
                OutputCallback callbackMsg, ProgressCallback callbackProgress,
                std::atomic<bool> stop);
bool ProgramFlash(const Device& device, const std::string& bitfile,
                  const std::string& cfgfile, Operation modes,
                  std::ostream* outStream, OutputCallback callbackMsg,
                  ProgressCallback callbackProgress, std::atomic<bool> stop);
#endif
