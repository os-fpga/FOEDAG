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
#include "Programmer.h"

namespace FOEDAG {

class HwDevices {
 public:
  HwDevices() = default;
  HwDevices(const Cable& cable);
  HwDevices operator=(const HwDevices& other);
  const Cable& getCable() const;
  const std::vector<Device>& getDevices() const;

  void addDevices(const std::vector<Device> sourceDevices);
  void addDevice(const Device& device);
  void clearDevices();
  size_t getDevicesCount() const;
  bool findDevice(int index, Device& device) const;
  bool findDevice(std::string name, Device& device) const;

 private:
  Cable m_cable;
  std::vector<Device> m_devices;
};

}  // namespace FOEDAG