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

#include "Programmer_helper.h"
#include "libusb-1.0/libusb.h"
#include <regex>
#include <unordered_set>

#include "../../src/Utils/StringUtils.h"
#include "Programmer.h"
#include "libusb.h"

namespace FOEDAG {

static const std::vector<std::string> programmer_subcmd{
    "fpga_config", "fpga_status", "flash", "list_devices"};

std::string transportTypeToString(TransportType transport) {
  switch (transport) {
    case TransportType::jtag:
      return "jtag";
      // Handle other transport types as needed
  }
  return "";
}

int64_t ConvertIntegerStringToInt64(const std::string& str, std::size_t* pos,
                                    int base) {
  try {
    return std::stoll(str, pos, base);
  } catch (const std::invalid_argument& ia) {
    CFG_POST_WARNING("ConvertIntegerStringToInt64 Invalid argument: %s",
                     ia.what());
    return 0;
  } catch (const std::out_of_range& oor) {
    CFG_POST_WARNING("ConvertIntegerStringToInt64 Invalid argument: %s",
                     oor.what());
    return 0;
  }
}

int ConvertIntegerStringToInt(const std::string& str, std::size_t* pos,
                              int base) {
  try {
    return std::stoi(str, pos, base);
  } catch (const std::invalid_argument& ia) {
    CFG_POST_WARNING("ConvertIntegerStringToInt Invalid argument: %s",
                     ia.what());
    return 0;
  } catch (const std::out_of_range& oor) {
    CFG_POST_WARNING("ConvertIntegerStringToInt Invalid argument: %s",
                     oor.what());
    return 0;
  }
}

int get_string_descriptor(struct libusb_device_handle* device,
                          uint8_t desc_index, std::string& stringDesc,
                          std::string& outputMsg) {
  int retval = 0;
  char desc_string[256]; /* Max size of string descriptor */
  retval = libusb_get_string_descriptor_ascii(
      device, desc_index, (unsigned char*)desc_string, sizeof(desc_string));
  stringDesc = desc_string;
  if (retval < 0) {
    outputMsg += "libusb_get_string_descriptor_ascii() failed with " +
                 std::string(libusb_error_name(retval)) + "\n";
    return retval;
  }
  return retval;
}

void addOrUpdateErrorMessage(int error, const std::string& message) {
  auto it = ErrorMessages.find(error);
  if (it != ErrorMessages.end()) {
    // Error code already exists, update the error message
    it->second = message;
  } else {
    // Error code doesn't exist, insert the new error code and message
    ErrorMessages.insert({error, message});
  }
}

std::vector<std::string> findStringPattern(const std::string& input,
                                           const std::string& pattern) {
  std::vector<std::string> matches;
  std::regex re(pattern);
  std::sregex_iterator next(input.begin(), input.end(), re);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    matches.push_back(match.str());
    next++;
  }
  return matches;
}

int isCableSupported(const Cable& cable) {
  bool cableSupported = false;
  for (auto& supportedCable : supportedCableVendorIdProductId) {
    if (cable.vendorId == std::get<0>(supportedCable) &&
        cable.productId == std::get<1>(supportedCable)) {
      cableSupported = true;
      break;
    }
  }
  if (!cableSupported) {
    return ProgrammerErrorCode::CableNotSupported;
  }
  return ProgrammerErrorCode::NoError;
}

/*
This function expecting the string from openOCD scan_chain output
with example below

testString =
"   TapName            Enabled IdCode     Expected   IrLen IrCap IrMask\n"
"-- ------------------ ------- ---------- ---------- ----- ----- ------\n"
"0  omap5912.dsp          Y    0x03df1d81 0x03df1d81 38    0x01  0x03\n"
"1  omap5912.arm          Y    0x0692602f 0x0692602f 4     0x01  0x0f\n"
"2  omap5912.unknown      Y    0x00000000 0x00000000 8     0x01  0x03\n"
"3 auto0.tap              Y     0x20000913 0x00000000     5 0x01  0x03
^\d+\s+\w+\.\w+\s+\w+\s+0x[\da-fA-F]+\s+0x[\da-fA-F]+\s+\d+\s+0x[\da-fA-F]+\s+0x[\da-fA-F]+$
*/
std::vector<TapInfo> extractTapInfoList(const std::string& tapInfoString) {
  std::vector<TapInfo> tapInfoList;
  std::istringstream iss(tapInfoString);
  std::string line;
  std::string token;
  size_t pos = -1;
  std::vector<std::string> matches;
  const std::string pattern(
      R"(\s*(\d+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\d+)\s+(\S+)\s+(\S+)\s*)");
  while (std::getline(iss, line)) {
    matches = findStringPattern(line, pattern);
    if (matches.size() == 0) {
      continue;
    }
    // Replace all the spaces with a single space
    while ((pos = line.find("  ")) != std::string::npos) {
      line.replace(pos, 2, " ");
    }
    std::istringstream lineStream(line);
    std::vector<std::string> tokens;
    while (lineStream >> token) {
      tokens.push_back(token);
    }
    if (tokens.size() == 8) {
      TapInfo tapInfo;
      tapInfo.index = ConvertIntegerStringToInt(tokens[0]);
      tapInfo.tapName = tokens[1];
      tapInfo.enabled = tokens[2] == "Y" ? true : false;
      tapInfo.idCode = ConvertIntegerStringToInt(tokens[3], 0, 0);
      tapInfo.expected = ConvertIntegerStringToInt(tokens[4], 0, 0);
      tapInfo.irLen = ConvertIntegerStringToInt(tokens[5]);
      tapInfo.irCap = ConvertIntegerStringToInt(tokens[6], 0, 0);
      tapInfo.irMask = ConvertIntegerStringToInt(tokens[7], 0, 0);
      tapInfoList.push_back(tapInfo);
    }
  }

  return tapInfoList;
}
/*
This function expecting the string from openOCD gemini list output
with example below

         Device               ID           IRLen      Flash Size
-------- -------------------- ------------ ---------- ----------------
Found  0 Gemini               0x20000913   5          16384
*/
std::vector<Device> extractDeviceList(const std::string& devicesString) {
  std::vector<Device> devices;

  std::istringstream iss(devicesString);
  std::string line;
  size_t pos = -1;
  std::vector<std::string> matches;
  const std::string pattern(
      R"(Found\s+(\d+)\s+([\w\s]+)\s+([0-9a-fA-Fx]+)\s+(\d+)\s+(\d+))");

  while (std::getline(iss, line)) {
    matches = findStringPattern(line, pattern);
    if (matches.size() == 0) {
      continue;
    }
    // Replace all the spaces with a single space
    while ((pos = line.find("  ")) != std::string::npos) {
      line.replace(pos, 2, " ");
    }
    Device device;
    std::istringstream lineStream(line);
    std::string firstToken;
    std::getline(lineStream, firstToken, ' ');  // "found string"

    std::string indexStr;
    std::getline(lineStream, indexStr, ' ');  // index string
    device.index = ConvertIntegerStringToInt(indexStr);

    std::getline(lineStream >> std::ws, device.name, ' ');  // device name

    std::string idCodeStr;
    std::getline(lineStream >> std::ws, idCodeStr,
                 ' ');  // idCode string in hex
    device.tapInfo.idCode = ConvertIntegerStringToInt64(idCodeStr, 0, 16);

    std::string irLenStr;
    std::getline(lineStream >> std::ws, irLenStr, ' ');  // irlength string
    device.tapInfo.irLen = ConvertIntegerStringToInt(irLenStr);

    std::string flashSizeStr;
    std::getline(lineStream >> std::ws, flashSizeStr,
                 ' ');  // flash size string in base 10
    device.flashSize = stol(flashSizeStr);

    for (const TapInfo& tap : supportedTAP) {
      if (tap.idCode == device.tapInfo.idCode) {
        device.tapInfo.enabled = tap.enabled;
        device.tapInfo.expected = tap.expected;
        device.tapInfo.irCap = tap.irCap;
        device.tapInfo.irMask = tap.irMask;
        device.tapInfo.irLen = tap.irLen;
        device.tapInfo.tapName = tap.tapName;
        devices.push_back(device);
      }
    }
  }

  return devices;
}

CfgStatus extractStatus(const std::string& statusString, bool& statusFound) {
  const std::string pattern(R"(\s*(\d+)\s+(\S+)\s+(\d+)\s+(\d+)\s*)");
  CfgStatus status;
  status.cfgDone = false;
  status.cfgError = false;
  std::istringstream iss(statusString);
  std::string line;
  size_t pos = -1;
  statusFound = false;
  std::vector<std::string> matches;
  std::vector<std::string> tokens;
  while (std::getline(iss, line)) {
    matches = findStringPattern(line, pattern);
    if (matches.size() == 0) {
      continue;
    }
    // Replace all the spaces with a single space
    while ((pos = line.find("  ")) != std::string::npos) {
      line.replace(pos, 2, " ");
    }
    // split the path into tokens
    StringUtils::tokenize(line, " ", tokens);
    if (tokens.size() != 5) {
      continue;
    }
    statusFound = true;
    break;
  }
  if (statusFound) {
    status.cfgDone = tokens[3] == "1" ? true : false;
    status.cfgError = tokens[4] == "1" ? true : false;
  }

  return status;
}

std::stringstream buildFpgaCableStringStream(const Cable& cable) {
  std::stringstream ss;
  ss << std::hex << std::showbase;
  ss << " -c \"adapter driver ftdi\"";
  if (cable.serialNumber != "") {
    ss << " -c \"adapter serial " << cable.serialNumber << "\"";
  }
  ss << " -c \"ftdi vid_pid " << cable.vendorId << " " << cable.productId
     << "\""
     << " -c \"ftdi layout_init 0x0c08 0x0f1b\"";
  ss << std::dec << std::noshowbase;
  ss << " -c \"adapter speed " << cable.speed << "\""
     << " -c \"transport select " << transportTypeToString(cable.transport)
     << "\"";
  return ss;
}

std::stringstream buildFpgaTargetStringStream(const Device& device) {
  std::stringstream ss;
  ss << " -c \"jtag newtap " << device.name << device.index << " tap -irlen "
     << device.tapInfo.irLen << " -expected-id 0x" << std::hex
     << device.tapInfo.expected << "\"";

  ss << std::dec;
  ss << " -c \"target create " << device.name << device.index
     << " riscv -endian little -chain-position " << device.name << device.index
     << ".tap\"";
  ss << " -c \"pld device gemini " << device.name << device.index << "\"";
  return ss;
}

std::string buildInitEndStringWithCommand(const std::string& command) {
  std::stringstream ss;
  ss << " -c \"init\""
     << " -c \"" << command << "\""
     << " -l /dev/stdout"
     << " -c \"exit\"";
  return ss.str();
}

std::string buildFpgaCableCommand(const Cable& cable,
                                  const std::string& command) {
  std::stringstream cmd;
  cmd = buildFpgaCableStringStream(cable);
  return cmd.str() + buildInitEndStringWithCommand(command);
}

std::string buildFpgaQueryStatusCommand(const Cable& cable,
                                        const Device& device) {
  std::stringstream cmd;
  std::stringstream cableSS = buildFpgaCableStringStream(cable);
  std::stringstream targetSS = buildFpgaTargetStringStream(device);
  cmd << cableSS.str() << targetSS.str();
  return cmd.str() + buildInitEndStringWithCommand(
                         "gemini status " + std::to_string(device.index));
}

std::string buildScanChainCommand(const Cable& cable) {
  std::stringstream cmd;
  cmd << buildFpgaCableCommand(cable, "scan_chain");
  return cmd.str();
}

std::string buildListDeviceCommand(const Cable& cable,
                                   const std::vector<TapInfo>& foundTapList) {
  if (foundTapList.size() == 0) {
    return "";
  }
  std::stringstream cmd;
  cmd = buildFpgaCableStringStream(cable);

  for (const TapInfo& tap : foundTapList) {
    cmd << " -c \"jtag newtap " << tap.tapName << tap.index << " tap -irlen "
        << tap.irLen << " -expected-id 0x" << std::hex << tap.expected << "\"";

    cmd << " -c \"target create " << tap.tapName << tap.index
        << " riscv -endian little -chain-position " << tap.tapName << tap.index
        << ".tap\"";
  }

  if (foundTapList.size() >= 1) {
    cmd << " -c \"pld device gemini ";
    for (size_t i = 0; i < foundTapList.size(); i++) {
      cmd << foundTapList[i].tapName << foundTapList[i].index;
      if (i != foundTapList.size() - 1) {
        cmd << " ";
      }
    }
    cmd << "\"";
  }

  cmd << " -c \"init\" -c \"gemini list\" -l /dev/stdout -c \"exit\"";
  return cmd.str();
}

std::string buildFpgaProgramCommand(const Cable& cable, const Device& device,
                                    const std::string& bitstreamFile) {
  std::stringstream cmd;
  std::stringstream programCommand;
  std::stringstream cableSS = buildFpgaCableStringStream(cable);
  std::stringstream targetSS = buildFpgaTargetStringStream(device);
  programCommand << "gemini load " << device.index << " fpga " << bitstreamFile
                 << " -p 1";
  cmd << cableSS.str() << targetSS.str();

  return cmd.str() + buildInitEndStringWithCommand(programCommand.str());
}

std::string buildFlashProgramCommand(
    const Cable& cable, const Device& device, const std::string& bitstreamFile,
    ProgramFlashOperation /*programFlashOperation*/) {
  std::stringstream cmd;
  std::stringstream programCommand;
  std::stringstream cableSS = buildFpgaCableStringStream(cable);
  std::stringstream targetSS = buildFpgaTargetStringStream(device);
  programCommand << "gemini load " << device.index << " flash " << bitstreamFile
                 << " -p 1";
  cmd << cableSS.str() << targetSS.str();

  return cmd.str() + buildInitEndStringWithCommand(programCommand.str());
}

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
    if (arg->m_args.size() < 2) {
      CFG_POST_ERR("Not enough arguments for %s. ", programmerCmd.name.c_str());
      programmerCmd.is_error = true;
      return programmerCmd;
    }
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
  std::unordered_set<std::string> uniqueValues;  // To store unique values
  std::stringstream ss(operations);
  std::string token;
  std::vector<std::string> operationsList;
  // the operations string is a comma separated list or | separated list of
  // operations
  while (std::getline(ss, token, ',')) {
    std::stringstream ss2(token);
    std::string subToken;

    while (std::getline(ss2, subToken, '|')) {
      // remove whitespace
      subToken.erase(
          std::remove_if(std::begin(subToken), std::end(subToken),
                         [](unsigned char ch) { return std::isspace(ch); }),
          std::end(subToken));
      // convert to lowercase
      std::transform(std::begin(subToken), std::end(subToken),
                     std::begin(subToken), ::tolower);
      if (uniqueValues.find(subToken) ==
          uniqueValues.end()) {         // Check if value already exists
        uniqueValues.insert(subToken);  // Add value to the unique set
        operationsList.push_back(subToken);
      }
    }
  }
  return operationsList;
}

bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations) {
  return std::find(supportedOperations.begin(), supportedOperations.end(),
                   operation) != supportedOperations.end();
}

}  // namespace FOEDAG