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

#include <iomanip>
#include <regex>
#include <sstream>
#include <unordered_set>

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"
#include "Programmer.h"
#include "ProgrammerGuiInterface.h"
#include "Utils/StringUtils.h"
#include "libusb.h"

namespace FOEDAG {

ProgrammerGuiInterface* Gui::m_guiInterface{nullptr};

static const std::vector<std::string> programmer_subcmd{
    "fpga_config", "fpga_status", "flash", "list_device", "list_cable"};

std::string transportTypeToString(TransportType transport) {
  switch (transport) {
    case TransportType::jtag:
      return "jtag";
      // Handle other transport types as needed
  }
  return std::string{};
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
  size_t pos = std::string::npos;
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
      tapInfo.index = CFG_convert_string_to_u64(tokens[0]);
      tapInfo.tapName = tokens[1];
      tapInfo.enabled = tokens[2] == "Y" ? true : false;
      tapInfo.idCode =
          static_cast<uint32_t>(CFG_convert_string_to_u64(tokens[3], 0, 0));
      tapInfo.expected =
          static_cast<uint32_t>(CFG_convert_string_to_u64(tokens[4], 0, 0));
      tapInfo.irLen =
          static_cast<uint32_t>(CFG_convert_string_to_u64(tokens[5]));
      tapInfo.irCap =
          static_cast<uint32_t>(CFG_convert_string_to_u64(tokens[6], 0, 0));
      tapInfo.irMask =
          static_cast<uint32_t>(CFG_convert_string_to_u64(tokens[7], 0, 0));
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
int extractDeviceList(const std::string& devicesString,
                      std::vector<Device>& devices) {
  devices.clear();

  std::istringstream iss(devicesString);
  std::string line;
  size_t pos = std::string::npos;
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
    device.index = static_cast<int>(CFG_convert_string_to_u64(indexStr));

    std::getline(lineStream >> std::ws, device.name, ' ');  // device name

    std::string idCodeStr;
    std::getline(lineStream >> std::ws, idCodeStr,
                 ' ');  // idCode string in hex
    device.tapInfo.idCode =
        static_cast<uint32_t>(CFG_convert_string_to_u64(idCodeStr));

    std::string irLenStr;
    std::getline(lineStream >> std::ws, irLenStr, ' ');  // irlength string
    device.tapInfo.irLen =
        static_cast<uint32_t>(CFG_convert_string_to_u64(irLenStr));

    std::string flashSizeStr;
    std::getline(lineStream >> std::ws, flashSizeStr,
                 ' ');  // flash size string in base 10
    device.flashSize =
        static_cast<uint32_t>(CFG_convert_string_to_u64(flashSizeStr));
    // check flashSize is multiple of 8 and power of 2
    if (!((device.flashSize % 8 == 0) && (device.flashSize > 0) &&
          ((device.flashSize & (device.flashSize - 1)) == 0))) {
      return ProgrammerErrorCode::InvalidFlashSize;
    }
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

  return ProgrammerErrorCode::NoError;
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
  if (!cable.serialNumber.empty()) {
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
    return std::string{};
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

std::string buildOTPProgramCommand(const Cable& cable, const Device& device,
                                   const std::string& bitstreamFile) {
  std::stringstream cmd;
  std::stringstream programCommand;
  std::stringstream cableSS = buildFpgaCableStringStream(cable);
  std::stringstream targetSS = buildFpgaTargetStringStream(device);
  programCommand << "gemini load " << device.index << " otp " << bitstreamFile
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

std::vector<std::string> parseOperationString(const std::string& operations) {
  std::unordered_set<std::string> uniqueValues;  // To store unique values
  std::stringstream ss(operations);
  std::string token;
  std::vector<std::string> operationsList;

  operationsList = CFG_split_string(operations, ",", 0, false);
  // Remove duplicates
  operationsList.erase(
      std::remove_if(std::begin(operationsList), std::end(operationsList),
                     [&uniqueValues](std::string& str) {
                       // erase empty space character
                       str.erase(
                           std::remove_if(std::begin(str), std::end(str),
                                          [](char c) { return c == ' '; }),
                           std::end(str));
                       return !uniqueValues.insert(str).second;
                     }),
      std::end(operationsList));
  return operationsList;
}

bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations) {
  return std::find(supportedOperations.begin(), supportedOperations.end(),
                   operation) != supportedOperations.end();
}

void printCableList(const std::vector<Cable>& cableList) {
  CFG_POST_MSG("Cable            ");
  CFG_POST_MSG("-----------------");
  for (const auto& cable : cableList) {
    CFG_POST_MSG("(%d) %s", cable.index, cable.name.c_str());
  }
  if (Gui::GuiInterface()) Gui::GuiInterface()->Cables(cableList);
}

void printDeviceList(const Cable& cable,
                     const std::vector<Device>& deviceList) {
  if (Gui::GuiInterface()) Gui::GuiInterface()->Devices(cable, deviceList);

  CFG_POST_MSG("Cable                       | Device            | Flash Size");
  CFG_POST_MSG("-------------------------------------------------------------");

  if (deviceList.size() == 0) {
    CFG_POST_MSG("  No device detected.");
    return;
  }
  for (size_t i = 0; i < deviceList.size(); i++) {
    std::ostringstream formattedOutput;
    std::string cable_name =
        "(" + std::to_string(cable.index) + ") " + cable.name;
    std::string device_name =
        "  (" + std::to_string(deviceList[i].index) + ") " + deviceList[i].name;
    std::string flashSize =
        "  " + CFG_convert_number_to_unit_string(deviceList[i].flashSize);
    formattedOutput << std::left << std::setw(28) << cable_name << std::setw(20)
                    << device_name << std::setw(20) << flashSize;
    CFG_POST_MSG("%s", formattedOutput.str().c_str());
  }
}

std::string buildCableDeviceAliasName(const Cable& cable,
                                      const Device& device) {
  std::string flashString =
      device.flashSize > 0
          ? (CFG_convert_number_to_unit_string(device.flashSize) + "B")
          : "na";
  return cable.name + "-" + device.name + "<" + std::to_string(device.index) +
         ">-" + flashString;
}

std::string buildCableDevicesAliasNameWithSpaceSeparatedString(
    const Cable& cable, const std::vector<Device>& devices) {
  std::string result;
  for (const auto& device : devices) {
    result += buildCableDeviceAliasName(cable, device) + " ";
  }
  // remove last space char
  if (!result.empty()) result.pop_back();
  return result;
}

std::string removeInfoAndNewline(const std::string& input) {
  std::string result = input;

  // Remove "info: " prefix
  size_t infoPos = result.find("Info : ");
  if (infoPos != std::string::npos) {
    result.erase(infoPos, 6);  // Length of "info: "
  }

  // Remove trailing newline character
  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result;
}

void InitializeCableMap(std::vector<Cable>& cables,
                        std::map<std::string, Cable>& cableMapObj) {
  int status = GetAvailableCables(cables);
  if (status != ProgrammerErrorCode::NoError) {
    CFG_POST_ERR("Failed to get available cables. Error code: %d", status);
    return;
  }
  if (cables.size() == 0) {
    CFG_POST_MSG("No cable found.");
    return;
  }
  cableMapObj.clear();
  for (size_t i = 0; i < cables.size(); i++) {
    cableMapObj[cables[i].name] = cables[i];
    cableMapObj[std::to_string(cables[i].index)] = cables[i];
  }
}

bool findDeviceFromDb(const std::vector<HwDevices>& cableDeviceDb,
                      const Cable& cable, std::string deviceName,
                      Device& device) {
  for (const auto& hwDevices : cableDeviceDb) {
    if (hwDevices.getCable().name == cable.name) {
      if (hwDevices.findDevice(deviceName, device)) {
        return true;
      }
    }
  }
  return false;
}

bool findDeviceFromDb(const std::vector<HwDevices>& cableDeviceDb,
                      const Cable& cable, int deviceIndex, Device& device) {
  for (const auto& hwDevices : cableDeviceDb) {
    if (hwDevices.getCable().name == cable.name) {
      if (hwDevices.findDevice(deviceIndex, device)) {
        return true;
      }
    }
  }
  return false;
}

void InitializeHwDb(
    std::vector<HwDevices>& cableDeviceDb,
    std::map<std::string, Cable>& cableMap,
    std::function<void(const Cable&, const std::vector<Device>&)>
        printDeviceList) {
  int status = 0;
  std::vector<Device> devices;
  std::vector<Cable> cables;
  InitializeCableMap(cables, cableMap);
  cableDeviceDb.clear();
  for (const Cable& cable : cables) {
    status = ListDevices(cable, devices);
    if (status != ProgrammerErrorCode::NoError) {
      CFG_POST_ERR(
          "Failed to list devices during InitializeHwDb. Error code: %d",
          status);
      return;
    }
    HwDevices cableStore(cable);
    cableStore.addDevices(devices);
    cableDeviceDb.push_back(cableStore);
    if (printDeviceList) printDeviceList(cable, devices);
  }
}

void Gui::SetGuiInterface(ProgrammerGuiInterface* guiInterface) {
  m_guiInterface = guiInterface;
}

ProgrammerGuiInterface* Gui::GuiInterface() { return m_guiInterface; }

}  // namespace FOEDAG
