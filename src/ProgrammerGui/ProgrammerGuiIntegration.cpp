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
#include "ProgrammerGuiIntegration.h"

#include <QDebug>

namespace FOEDAG {

bool operator==(const Cable &c1, const Cable &c2) {
  return c1.vendorId == c2.vendorId && c1.productId == c2.productId &&
         c1.busAddr == c2.busAddr && c1.portAddr == c2.portAddr &&
         c1.deviceAddr == c2.deviceAddr && c1.channel == c2.channel &&
         c1.serialNumber == c2.serialNumber &&
         c1.description == c2.description && c1.speed == c2.speed &&
         c1.transport == c2.transport && c1.name == c2.name &&
         c1.index == c2.index;
}

bool operator==(const Device &d1, const Device &d2) {
  // TODO add taps
  return d1.index == d2.index && d1.flashSize == d2.flashSize &&
         d1.name == d2.name;
}

ProgrammerGuiIntegration::ProgrammerGuiIntegration(QObject *parent)
    : QObject(parent) {
  qRegisterMetaType<std::string>("std::string");
}

const sequential_map<Cable, std::vector<Device> >
    &ProgrammerGuiIntegration::devices() const {
  return m_devices;
}

void ProgrammerGuiIntegration::Cables(const std::vector<Cable> &cables) {
  for (const auto &cable : cables) {
    m_devices[cable] = {};
  }
}

void ProgrammerGuiIntegration::Devices(const Cable &cable,
                                       const std::vector<Device> &devices) {
  for (const auto &[cab, dev] : m_devices.values()) {
    if (cab == cable) {
      m_devices[cab] = devices;
      break;
    }
  }
  emit autoDetect();
}

void ProgrammerGuiIntegration::Progress(const std::string &progress) {
  emit this->progress(progress);
}

void ProgrammerGuiIntegration::ProgramFpga(const Cable &cable,
                                           const Device &device,
                                           const std::string &file) {
  m_current = std::make_pair(cable, device);
  m_flash = false;
  m_files.push_back(std::make_pair(device, file));
}

void ProgrammerGuiIntegration::Flash(const Cable &cable, const Device &device) {
  m_current = std::make_pair(cable, device);
  m_flash = true;
}

const std::pair<Cable, Device> &ProgrammerGuiIntegration::CurrentDevice()
    const {
  return m_current;
}

bool ProgrammerGuiIntegration::IsFlash() const { return m_flash; }

const std::vector<std::pair<Device, std::string> >
    &ProgrammerGuiIntegration::Files() const {
  return m_files;
}

std::string ProgrammerGuiIntegration::File(const Device &dev) const {
  for (auto &[device, file] : m_files) {
    if (device == dev) return file;
  }
  return {};
}

}  // namespace FOEDAG
