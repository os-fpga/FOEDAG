
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

#pragma once
#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

struct libusb_device_handle;

namespace FOEDAG {

// forward declaration
struct Cable;
struct Device;
struct TapInfo;
struct CfgStatus;
enum class ProgramFlashOperation : uint32_t;
enum class TransportType : uint32_t;

struct ProgrammerCommand {
  std::string name;
  std::string executable_cmd;
  bool is_error = false;
};

int64_t ConvertIntegerStringToInt64(const std::string& str,
                                    std::size_t* pos = nullptr, int base = 10);
int ConvertIntegerStringToInt(const std::string& str,
                              std::size_t* pos = nullptr, int base = 10);

void addOrUpdateErrorMessage(int error, const std::string& message);
std::vector<std::string> findStringPattern(const std::string& input,
                                           const std::string& pattern);
int isCableSupported(const Cable& cable);
std::stringstream buildFpgaCableStringStream(const Cable& cable);
std::stringstream buildFpgaTargetStringStream(const Device& device);
std::string buildInitEndStringWithCommand(const std::string& command);
std::string buildFpgaCableCommand(const Cable& cable,
                                  const std::string& command);
std::string buildScanChainCommand(const Cable& cable);
std::string buildFpgaQueryStatusCommand(const Cable& cable,
                                        const Device& device);
std::string buildListDeviceCommand(const Cable& cable,
                                   const std::vector<TapInfo>& foundTapList);
std::string buildFpgaProgramCommand(const Cable& cable, const Device& device,
                                    const std::string& bitstream_file);
std::string buildFlashProgramCommand(
    const Cable& cable, const Device& device, const std::string& bitstreamFile,
    ProgramFlashOperation programFlashOperation);
std::string buildFpgaProgramCommand(const std::string& bitstream_file,
                                    const std::string& config_file,
                                    int pld_index);
std::string buildFpgaQueryStatusCommand(const std::string config_file,
                                        int pld_index);
std::string buildListDeviceCommand(const std::string config_file);
std::string buildListDeviceCommand(const Cable& cable, const TapInfo& foundTap);
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

// libusb related helper function
int get_string_descriptor(struct libusb_device_handle* device,
                          uint8_t desc_index, std::string& stringDesc,
                          std::string& outputMsg);

std::vector<TapInfo> extractTapInfoList(const std::string& tapInfoString);
std::vector<Device> extractDeviceList(const std::string& deviceListString);
CfgStatus extractStatus(const std::string& statusString, bool& statusFound);
std::string transportTypeToString(TransportType transport);

}  // namespace FOEDAG