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

#include <filesystem>
#include <iostream>

#include "CFGCommon/CFGCommon.h"
#include "Programmer.h"

void progressCallback(std::string msg) { CFG_POST_MSG(msg.c_str()); }

void messageCallback(std::string msg) { CFG_POST_MSG(msg.c_str()); }

using namespace FOEDAG;
int main(int argc, const char** argv) {
  CFG_POST_MSG("This is Programmer cmd");
  // CFGCommon_ARG cmdarg;
  // programmer_entry(&cmdarg);

  // It is currently to use for
  // programmer API manual testing, and example of using the API
  // <TODO Standalone programmer cmd line executable EDA-1935>
  // eventually it will be a standalone programmer executable

  // openocd path is the same as the executable path
  std::filesystem::path openOcdExecutablePath = std::filesystem::current_path();
  std::string openOcdName;
// if windows
#ifdef _WIN32
  openOcdName = "openocd.exe";
#else
  openOcdName = "openocd";
#endif
  openOcdExecutablePath /= openOcdName;

  int ret = InitLibrary(openOcdExecutablePath.string());
  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  }
  std::vector<Device> devices;

  std::vector<FOEDAG::Cable> cables;
  ret = GetAvailableCables(cables);
  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  } else if (cables.size() == 0) {
    CFG_POST_MSG("No programming cable found. Make sure it is connected.");
    return -1;
  }
  CFG_POST_MSG("return %d", ret);
  CFG_POST_MSG("-- GetAvailableCables(cables) --")
  for (auto& cable : cables) {
    CFG_POST_MSG("--------------")
    CFG_POST_MSG("product ID: 0x%04x", cable.productId);
    CFG_POST_MSG("vendor  ID: 0x%04x", cable.vendorId);
    CFG_POST_MSG("serial    : %s", cable.serialNumber.c_str());
    CFG_POST_MSG("desc      : %s", cable.description.c_str());
    CFG_POST_MSG("bus       : %d", cable.busAddr);
    CFG_POST_MSG("port      : %d", cable.portAddr);
    CFG_POST_MSG("device    : %d", cable.deviceAddr);
    CFG_POST_MSG("speed     : %d", cable.speed);
    CFG_POST_MSG("transport : %d", cable.transport);
    CFG_POST_MSG("channel   : %d", cable.channel);
  }

  CFG_POST_MSG("-- ListDevices(cables, outputMsg) --")
  ret = ListDevices(cables[0], devices);
  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  }
  if (devices.size() == 0) {
    CFG_POST_MSG("No device found. Make sure the board power is ON.");
    return -1;
  }
  CFG_POST_MSG("### ListDevices API testing ###");
  for (auto& device : devices) {
    CFG_POST_MSG("-------------------------");
    CFG_POST_MSG("Device name: %s", device.name.c_str());
    CFG_POST_MSG("Device index: %d", device.index);
    CFG_POST_MSG("Device jtagId: 0x%08x", device.tapInfo.idCode);
    CFG_POST_MSG("Device mask: 0x%08x", device.tapInfo.irMask);
    CFG_POST_MSG("Device irlength: %d", device.tapInfo.irLen);
    CFG_POST_MSG("Device flashSize: %s bits",
                 CFG_convert_number_to_unit_string(device.flashSize).c_str());
    CFG_POST_MSG("-------------------------");
  }

  CfgStatus status;
  std::string statusCmdOutput;
  ret = GetFpgaStatus(cables[0], devices[0], status, statusCmdOutput);
  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  }
  CFG_POST_MSG("### GetFpgaStatus API testing before programming ###");
  CFG_POST_MSG("Device cfgDone: %d", status.cfgDone);
  CFG_POST_MSG("Device cfgError: %d", status.cfgError);

  int userInput = 1;
  std::atomic<bool> stop = false;
  std::string bitfile{};
  std::cout << "Enter 1 for fpga programming (default  1)\n"
            << "2 for flash programming" << std::endl;
  std::cin >> userInput;
  std::cout << "Enter bitstream file for programming\n";
  std::cin >> bitfile;
  if (userInput == 1) {
    ret = ProgramFpga(cables[0], devices[0], bitfile, stop,
                      nullptr,          //&std::cout,
                      nullptr,          // messageCallback, //nullptr
                      progressCallback  // progressCallback
    );
  } else {
    ProgramFlashOperation modes = ProgramFlashOperation::Program;
    ret = ProgramFlash(cables[0], devices[0], bitfile, stop, modes,
                       nullptr,          //&std::cout,
                       messageCallback,  // nullptr
                       nullptr           // progressCallback
    );
  }

  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  }

  ret = GetFpgaStatus(cables[0], devices[0], status, statusCmdOutput);
  if (ret != NoError) {
    CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
    return ret;
  }
  CFG_POST_MSG("### GetFpgaStatus API testing after programming ###");
  CFG_POST_MSG("Device cfgDone: %d", status.cfgDone);
  CFG_POST_MSG("Device cfgError: %d", status.cfgError);

  return 0;
}