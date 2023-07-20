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

#include <thread>

#include "Configuration/Programmer/Programmer.h"
#include "Utils/StringUtils.h"

namespace FOEDAG {

ProgrammerBackend::ProgrammerBackend() {}

int ProgrammerBackend::InitLibraryAPI(const QString &openocd) {
  return InitLibrary(openocd.toStdString());
}

std::pair<bool, QString> ProgrammerBackend::ListDevicesAPI(
    std::vector<FoedagDevice> &devices) {
  std::vector<Device> devs;
  std::string out;
  std::vector<Cable> cables;
  devices.push_back({"Dummy device 0"});
  devices.push_back({"Dummy device 1"});
  devices.push_back({"Dummy device 2"});
  return std::make_pair(true, QString{});
}

int ProgrammerBackend::ProgramFpgaAPI(
    const FoedagDevice &device, const QString &bitfile, const QString &cfgfile,
    std::ostream *outStream, OutputCallback outputMsg,
    ProgressCallback_ callback, std::atomic<bool> *stop) {
  int progress = 0;
  while (progress < 100) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if (*stop) return 1;
    progress += 20;
    if (callback) callback(StringUtils::to_string<int>(progress));
  }
  return 0;
}

int ProgrammerBackend::ProgramFlashAPI(
    const FoedagDevice &device, const QString &bitfile, const QString &cfgfile,
    std::ostream *outStream, OutputCallback outputMsg,
    ProgressCallback_ callback, std::atomic<bool> *stop) {
  int progress = 0;
  while (progress < 100) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (*stop) return 1;
    progress += 20;
    if (outputMsg)
      outputMsg(QString{"Info: burn flash, progress: %1%\n"}.arg(
          QString::number(progress)));
    if (callback) callback(QString::number(progress).toStdString());
  }
  return 0;
}

bool ProgrammerBackend::StatusAPI(const FoedagDevice &device) {
  std::vector<Device> devs;
  if (device.dev) {
    CfgStatus stat{};
    bool ok = /*GetFpgaStatus(*device.dev, stat)*/ true;
    if (ok) return stat.cfgDone == true && stat.cfgError == false;
  }
  return true;
}

}  // namespace FOEDAG
