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

#include "Programmer/ProgrammerGuiInterface.h"
#include "Utils/sequential_map.h"

namespace FOEDAG {

bool operator==(const Cable &c1, const Cable &c2);
bool operator==(const Device &d1, const Device &d2);
bool operator==(const TapInfo &t1, const TapInfo &t2);

bool operator<(const Device &d1, const Device &d2);  // required by std::map

struct DeviceBitstream {
  std::string bitstream{};
  std::string flashBitstream{};
};

class ProgrammerGuiIntegration : public QObject, public ProgrammerGuiInterface {
  Q_OBJECT

 public:
  explicit ProgrammerGuiIntegration(QObject *parent = nullptr);
  const sequential_map<Cable, std::vector<Device>> &devices() const;
  void Cables(const std::vector<Cable> &cables) override;
  void Devices(const Cable &cable, const std::vector<Device> &devices) override;
  void Progress(const std::string &progress) override;
  void ProgramFpga(const Cable &cable, const Device &device,
                   const std::string &file) override;
  void Flash(const Cable &cable, const Device &device,
             const std::string &file) override;

  const std::pair<Cable, Device> &CurrentDevice() const;
  bool IsFlash() const;
  std::string File(const Device &dev, bool flash) const;

 signals:
  void progress(const std::string &progress);
  void autoDetect();

 private:
  sequential_map<Cable, std::vector<Device>> m_devices;
  std::pair<Cable, Device> m_current;
  std::map<Device, DeviceBitstream> m_files;
  bool m_flash{false};
};

}  // namespace FOEDAG
