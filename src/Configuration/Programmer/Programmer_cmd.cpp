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

#include <iostream>

#include "CFGCommon/CFGCommon.h"
#include "Programmer.h"

void progressCallback(std::string msg) { std::cout << msg; }

void messageCallback(std::string msg) { std::cout << "MS-CALLBACK :" << msg; }

using namespace FOEDAG;
int main(int argc, const char** argv) {
  CFG_POST_MSG("This is Programmer cmd");
  CFGCommon_ARG cmdarg;

  // programmer_entry(&cmdarg);
  // simple programmer API manual testing

  auto formatMemorySize = [](int sizeInBits) -> std::string {
    const int bitsInKilobit = 1024;
    const int bitsInMegabit = 1024 * 1024;

    if (sizeInBits >= bitsInMegabit) {
      int megabits = sizeInBits / bitsInMegabit;
      return std::to_string(megabits) + "M";
    } else if (sizeInBits >= bitsInKilobit) {
      int kilobits = sizeInBits / bitsInKilobit;
      return std::to_string(kilobits) + "K";
    } else {
      return std::to_string(sizeInBits);
    }
  };

  std::string openOCDPath = "/home/guanyung/dev/openocd/src/openocd";
  openOCDPath = "/home/asic01/development/openocd_rs/src/openocd";
  int ret = InitLibrary(openOCDPath);
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
    return 0;
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
  if (cables.size() > 0) {
    ret = ListDevices(cables[0], devices);
    if (ret != NoError) {
      CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
      return ret;
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
                   formatMemorySize(device.flashSize).c_str());

      CFG_POST_MSG("-------------------------");
    }
  }
  if (devices.size() > 0) {
    CfgStatus status;
    ret = GetFpgaStatus(cables[0], devices[0], status);
    if (ret != NoError) {
      CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
      return ret;
    }
    CFG_POST_MSG("### GetFpgaStatus API testing ###");
    CFG_POST_MSG("Device cfgDone: %s", std::to_string(status.cfgDone).c_str());
    CFG_POST_MSG("Device cfgError: %s",
                 std::to_string(status.cfgError).c_str());
  }

  int userInput = 1;
  std::atomic<bool> stop = false;
  std::string bitfile =
      "/home/asic01/development/ubi_fsbl_fpga_faked_spare_reg.bin";
  std::cout << "Enter 1 for fpga programming (default  1)\n"
            << "2 for flash programming" << std::endl;
  std::cin >> userInput;
  if (userInput == 1) {
    ret = ProgramFpga(cables[0], devices[0], bitfile, stop,
                      nullptr,          //&std::cout,
                      nullptr,          // messageCallback, //nullptr
                      progressCallback  // progressCallback
    );
  } else {
    bitfile = "/home/asic01/development/ubi_fsbl_fpga_flash_program.bin";
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

  if (devices.size() > 0) {
    CfgStatus status;
    ret = GetFpgaStatus(cables[0], devices[0], status);
    if (ret != NoError) {
      CFG_POST_MSG("Error code = %d. %s", ret, GetErrorMessage(ret).c_str());
      return ret;
    }
    CFG_POST_MSG("### GetFpgaStatus API testing ###");
    CFG_POST_MSG("Device cfgDone: %s", std::to_string(status.cfgDone).c_str());
    CFG_POST_MSG("Device cfgError: %s",
                 std::to_string(status.cfgError).c_str());
  }

  return 0;
}
