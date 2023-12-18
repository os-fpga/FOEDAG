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
#include "HwDevices.h"

struct libusb_device_handle;

namespace FOEDAG {

// forward declaration
struct Cable;
struct Device;
struct Tap;
struct CfgStatus;
class ProgrammerGuiInterface;
enum class ProgramFlashOperation : uint32_t;
enum TransportType;

struct ProgrammerCommand {
  std::string name;
  std::string executable_cmd;
  bool is_error = false;
};

class Gui {
  static ProgrammerGuiInterface* m_guiInterface;

 public:
  static void SetGuiInterface(ProgrammerGuiInterface* guiInterface);
  static ProgrammerGuiInterface* GuiInterface();
};

double ExtractNumber(const std::string& line);
std::string UpdateDownloadProgress(double percentage);
std::vector<std::string> findStringPattern(const std::string& input,
                                           const std::string& pattern);
std::vector<std::string> parseOperationString(const std::string& operation);
bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations);

void printCableList(const std::vector<Cable>& cableList, bool verbose);
void processCableList(const std::vector<Cable>& cableList, bool verbose);
void printDeviceList(const Cable& cable, const std::vector<Device>& deviceList,
                     bool verbose);
void processDeviceList(const Cable& cable,
                       const std::vector<Device>& deviceList, bool verbose);
std::string buildCableDeviceAliasName(const Cable& cable, const Device& device);
std::string buildCableDevicesAliasNameWithSpaceSeparatedString(
    const Cable& cable, const std::vector<Device>& devices);

std::string removeInfoAndNewline(const std::string& input);

CfgStatus extractStatus(const std::string& statusString, bool& statusFound);

}  // namespace FOEDAG