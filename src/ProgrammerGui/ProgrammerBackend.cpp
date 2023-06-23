/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#include "ProgrammerBackend.h"
#include "Configuration/Programmer/Programmer.h"

#include <thread>

namespace FOEDAG {

ProgrammerBackend::ProgrammerBackend() {}

std::pair<bool, QString> ProgrammerBackend::ListDevicesAPI(
    std::vector<FoedagDevice> &devices) {
    std::vector<Device> devs;
    std::string out;
    bool ok = ListDevices(devs, out);
    devices.clear();
    for (const auto &dev : devs) {
      devices.push_back({QString::fromStdString(dev.name), new Device{dev}});
    }
    return std::make_pair(ok, QString::fromStdString(out));
//  devices.push_back({"Dummy device 0"});
//  devices.push_back({"Dummy device 1"});
//  devices.push_back({"Dummy device 2"});
//  return std::make_pair(true, QString{});
}

int ProgrammerBackend::ProgramFpgaAPI(
    const FoedagDevice &device, const QString &bitfile, const QString &cfgfile,
    std::ostream *outStream, OutputCallback outputMsg,
    ProgressCallback callback, std::atomic<bool> *stop) {
  if (device.dev) {
    auto outCallback = [outputMsg](const std::string &str) {
      outputMsg(QString::fromStdString(str));
    };
    return ProgramFpga(*device.dev, bitfile.toStdString(),
                       cfgfile.toStdString(), outStream, outCallback, callback,
                       stop);
  }
  //  int progress = 0;
  //  while (progress < 100) {
  //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  //    if (*stop) return 1;
  //    progress += 20;
  //    if (callback) callback(static_cast<double>(progress));
  //  }
  return 1;
}

int ProgrammerBackend::ProgramFlashAPI(
    const FoedagDevice &device, const QString &bitfile, const QString &cfgfile,
    std::ostream *outStream, OutputCallback outputMsg,
    ProgressCallback callback, std::atomic<bool> *stop) {
  int progress = 0;
  while (progress < 100) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (*stop) return 1;
    progress += 20;
    if (outputMsg)
      outputMsg(QString{"Info: burn flash, progress: %1%\n"}.arg(
          QString::number(progress)));
    if (callback) callback(static_cast<double>(progress));
  }
  return 0;
}

bool ProgrammerBackend::StatusAPI(const FoedagDevice &device) {
  std::vector<Device> devs;
  if (device.dev) {
    CfgStatus stat{};
    bool ok = GetFpgaStatus(*device.dev, stat);
    if (ok) return stat.cfgDone == true && stat.cfgError == false;
  }
  return false;
}

}  // namespace FOEDAG
