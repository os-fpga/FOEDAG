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
  std::vector<Device> devices;
  std::string outputMsg;
  if (ListDevices(devices, outputMsg)) {
    CFG_POST_MSG("### ListDevices API testing ###");
    for (auto& device : devices) {
      CFG_POST_MSG("-------------------------");
      CFG_POST_MSG("Device name: %s", device.name.c_str());
      CFG_POST_MSG("Device index: %s", std::to_string(device.index).c_str());
      CFG_POST_MSG("Device jtagId: %s", device.jtagId.c_str());
      CFG_POST_MSG("Device mask: %s", std::to_string(device.mask).c_str());
      CFG_POST_MSG("Device irlength: %s",
                   std::to_string(device.irlength).c_str());
      CFG_POST_MSG("Device flashSize: %s",
                   std::to_string(device.flashSize).c_str());
    }
    CFG_POST_MSG("-------------------------");
    CFG_POST_MSG(outputMsg.c_str());
  }

  Device device = devices[0];
  CfgStatus status;
  if (GetFpgaStatus(device, status)) {
    CFG_POST_MSG("### GetFpgaStatus API testing ###");
    CFG_POST_MSG("Device cfgDone: %s", std::to_string(status.cfgDone).c_str());
    CFG_POST_MSG("Device cfgError: %s",
                 std::to_string(status.cfgError).c_str());
  }

  OutputCallback callback = [](std::string msg) {
    CFG_POST_MSG("### OutputCallback API testing ###");
    CFG_POST_MSG("OutputCallback msg: %s", msg.c_str());
  };
  ProgressCallback progress = [](double percent) {
    CFG_POST_MSG("### ProgressCallback API testing ###");
    CFG_POST_MSG("ProgressCallback percent: %s",
                 std::to_string(percent).c_str());
  };
  std::atomic<bool> stop = false;
  ProgramFpga(device, "test.bit", "config.cfg", nullptr, callback, progress,
              stop);

  return 0;
}
