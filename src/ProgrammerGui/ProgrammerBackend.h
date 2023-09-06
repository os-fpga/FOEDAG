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
#pragma once

#include <QObject>
#include <QString>

namespace FOEDAG {
struct Device;

struct FoedagDevice {
  QString name;
  Device *dev{nullptr};
};

using ProgressCallback_ = std::function<void(std::string)>;
using OutputCallback = std::function<void(const QString &)>;

class ProgrammerBackend {
 public:
  ProgrammerBackend();
  int InitLibraryAPI(const QString &openocd);
  std::pair<bool, QString> ListDevicesAPI(std::vector<FoedagDevice> &devices);
  int ProgramFpgaAPI(const FoedagDevice &device, const QString &bitfile,
                     const QString &cfgfile, std::ostream *outStream,
                     OutputCallback outputMsg, ProgressCallback_ callback,
                     std::atomic<bool> *stop);
  int ProgramFlashAPI(const FoedagDevice &device, const QString &bitfile,
                      const QString &cfgfile, std::ostream *outStream,
                      OutputCallback outputMsg, ProgressCallback_ callback,
                      std::atomic<bool> *stop);
  bool StatusAPI(const FoedagDevice &device);
};
}  // namespace FOEDAG
