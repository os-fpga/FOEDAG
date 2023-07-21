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

#include "CFGCommon/CFGCommon.h"
#include "Programmer.h"

using namespace FOEDAG;
int main(int argc, const char** argv) {
  CFG_POST_MSG("This is Programmer cmd");
  CFGCommon_ARG cmdarg;

  // programmer_entry(&cmdarg);

  // simple programmer API manual testing
  std::string openOCDPath = "/home/guanyung/dev/openocd/src/openocd";
  InitLibrary(openOCDPath);
  std::vector<Device> devices;

  std::vector<FOEDAG::Cable> cables;
  GetAvailableCables(cables);
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
    if (ListDevices(cables[0], devices)) {
      CFG_POST_MSG("### ListDevices API testing ###");
      for (auto& device : devices) {
        CFG_POST_MSG("-------------------------");
        CFG_POST_MSG("Device name: %s", device.name.c_str());
        CFG_POST_MSG("Device index: %d", device.index);
        CFG_POST_MSG("Device jtagId: %d", device.tapInfo.idCode);
        CFG_POST_MSG("Device mask: %d", device.tapInfo.irMask);
        CFG_POST_MSG("Device irlength: %d", device.tapInfo.irLen);
        CFG_POST_MSG("Device flashSize: %d", device.flashSize);
      }
      CFG_POST_MSG("-------------------------");
    }
  }

  // Device device = devices[0];
  // CfgStatus status;
  // if (GetFpgaStatus(device, status)) {
  //   CFG_POST_MSG("### GetFpgaStatus API testing ###");
  //   CFG_POST_MSG("Device cfgDone: %s",
  //   std::to_string(status.cfgDone).c_str()); CFG_POST_MSG("Device cfgError:
  //   %s",
  //                std::to_string(status.cfgError).c_str());
  // }

  // OutputCallback callback = [](std::string msg) {
  //   CFG_POST_MSG("### OutputCallback API testing ###");
  //   CFG_POST_MSG("OutputCallback msg: %s", msg.c_str());
  // };
  // ProgressCallback progress = [](double percent) {
  //   CFG_POST_MSG("### ProgressCallback API testing ###");
  //   CFG_POST_MSG("ProgressCallback percent: %s",
  //                std::to_string(percent).c_str());
  // };
  // std::atomic<bool> stop = false;
  // ProgramFpga(device, "test.bit", "config.cfg", nullptr, callback, progress,
  //             stop);

  return 0;
}
