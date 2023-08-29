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

#include "HwDevices.h"

#include <vector>

namespace FOEDAG {

HwDevices::HwDevices(const Cable& cable) : m_cable(cable) {}

HwDevices HwDevices::operator=(const HwDevices& other) {
  m_cable = other.m_cable;
  m_devices = other.m_devices;
  return *this;
}
const Cable& HwDevices::getCable() const { return m_cable; }

const std::vector<Device>& HwDevices::getDevices() const { return m_devices; }

void HwDevices::addDevices(const std::vector<Device> sourceDevices) {
  m_devices.insert(m_devices.end(), sourceDevices.begin(), sourceDevices.end());
}

void HwDevices::addDevice(const Device& device) { m_devices.push_back(device); }

void HwDevices::clearDevices() { m_devices.clear(); }

size_t HwDevices::getDevicesCount() const { return m_devices.size(); }

bool HwDevices::findDevice(int index, Device& device) const {
  bool found = false;
  for (const auto& d : m_devices) {
    if (d.index == index) {
      device = d;
      found = true;
      break;
    }
  }
  return found;
}

bool HwDevices::findDevice(std::string name, Device& device) const {
  bool found = false;
  for (const auto& d : m_devices) {
    if (d.name == name) {
      device = d;
      found = true;
    }
  }
  return found;
}

}  // namespace FOEDAG
