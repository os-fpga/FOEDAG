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
#include "Programmer_error_code.h"
#include "Utils/StringUtils.h"

namespace FOEDAG {

ProgrammerGuiInterface* Gui::m_guiInterface{nullptr};

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

CfgStatus extractStatus(const std::string& statusString, bool& statusFound) {
  const std::string pattern(R"(\s*(\d+)\s+(\S+)\s+(\d+)\s+(\d+)\s*)");
  CfgStatus status;
  status.cfgDone = false;
  status.cfgError = false;
  std::istringstream iss(statusString);
  std::string line;
  size_t pos = -1;
  statusFound = false;
  std::vector<std::string> matches, tokens;
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

void processCableList(const std::vector<Cable>& cableList, bool verbose) {
  if (cableList.size() == 0) {
    CFG_POST_MSG("  No cables is connected.");
    return;
  }
  if (verbose) {
    printCableList(cableList, verbose);
  }
  if (Gui::GuiInterface()) Gui::GuiInterface()->Cables(cableList);
}

void processDeviceList(const Cable& cable,
                       const std::vector<Device>& deviceList, bool verbose) {
  if (verbose) {
    printDeviceList(cable, deviceList, verbose);
  }
  if (Gui::GuiInterface()) Gui::GuiInterface()->Devices(cable, deviceList);
}

void printCableList(const std::vector<Cable>& cableList, bool verbose) {
  if (verbose) {
    CFG_POST_MSG("Cable            ");
    CFG_POST_MSG("-----------------");
    for (const auto& cable : cableList) {
      CFG_POST_MSG("(%d) %s", cable.index, cable.name.c_str());
    }
  }
}

void printDeviceList(const Cable& cable, const std::vector<Device>& deviceList,
                     bool verbose) {
  if (deviceList.size() == 0) {
    CFG_POST_MSG("  No device is detected.");
    return;
  }
  if (verbose) {
    CFG_POST_MSG(
        "Cable                       | Device            | Flash Size");
    CFG_POST_MSG(
        "-------------------------------------------------------------");
    for (size_t i = 0; i < deviceList.size(); i++) {
      std::ostringstream formattedOutput;
      std::string cable_name =
          "(" + std::to_string(cable.index) + ") " + cable.name;
      std::string device_name = "  (" + std::to_string(deviceList[i].index) +
                                ") " + deviceList[i].name;
      std::string flashSize =
          "  " + CFG_convert_number_to_unit_string(deviceList[i].flashSize);
      formattedOutput << std::left << std::setw(28) << cable_name
                      << std::setw(20) << device_name << std::setw(20)
                      << flashSize;
      CFG_POST_MSG("%s", formattedOutput.str().c_str());
    }
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

int CheckCableAndDevice(HardwareManager& hardware_manager, const Cable& cable,
                        const Device& device, Device& detectedDevice,
                        std::vector<Tap>& taplist) {
  if (!hardware_manager.is_cable_exists(cable.name)) {
    return ProgrammerErrorCode::CableNotFound;
  }

  if (!hardware_manager.find_device(cable.name, device.index, detectedDevice,
                                    taplist)) {
    return ProgrammerErrorCode::DeviceNotFound;
  }

  // Return a success code or any other specific error code if needed.
  return ProgrammerErrorCode::NoError;
}

void Gui::SetGuiInterface(ProgrammerGuiInterface* guiInterface) {
  m_guiInterface = guiInterface;
}

ProgrammerGuiInterface* Gui::GuiInterface() { return m_guiInterface; }

}  // namespace FOEDAG