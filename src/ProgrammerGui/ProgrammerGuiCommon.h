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

#include <QStringList>

#include "Configuration/Programmer/Programmer_helper.h"

namespace FOEDAG {

inline bool operator==(const Cable &c1, const Cable &c2) {
  return c1.vendorId == c2.vendorId && c1.productId == c2.productId &&
         c1.busAddr == c2.busAddr && c1.portAddr == c2.portAddr &&
         c1.deviceAddr == c2.deviceAddr && c1.channel == c2.channel &&
         c1.serialNumber == c2.serialNumber &&
         c1.description == c2.description && c1.speed == c2.speed &&
         c1.transport == c2.transport && c1.name == c2.name &&
         c1.index == c2.index;
}

inline bool operator==(const TapInfo &t1, const TapInfo &t2) {
  return t1.enabled == t2.enabled && t1.expected == t2.expected &&
         t1.idCode == t2.idCode && t1.index == t2.index &&
         t1.irCap == t2.irCap && t1.irLen == t2.irLen &&
         t1.irMask == t2.irMask && t1.tapName == t2.tapName;
}

inline bool operator==(const Device &d1, const Device &d2) {
  return d1.index == d2.index && d1.flashSize == d2.flashSize &&
         d1.name == d2.name && d1.tapInfo == d2.tapInfo;
}

class ProgrammerCable {
 public:
  ProgrammerCable(const Cable &cab)
      : m_name(QString::fromStdString(cab.name)), m_cable(cab) {}
  ProgrammerCable() = default;
  bool operator==(const ProgrammerCable &other) const {
    return m_cable == other.m_cable && m_name == other.m_name;
  }
  QString name() const { return m_name; }

 private:
  QString m_name{};
  Cable m_cable{};
};

class ProgrammerDevice {
 public:
  ProgrammerDevice(const Device &dev)
      : m_name(QString::fromStdString(dev.name)), m_device(dev) {}
  ProgrammerDevice() = default;
  bool hasFlash() const { return m_device.flashSize > 0; }
  bool operator==(const ProgrammerDevice &other) const {
    return m_device == other.m_device && m_name == other.m_name;
  }
  int index() const { return m_device.index; }
  QString name() const { return m_name; }

  uint32_t idCode() const { return m_device.tapInfo.idCode; }
  uint32_t irMask() const { return m_device.tapInfo.irMask; }
  uint32_t irLen() const { return m_device.tapInfo.irLen; }

 private:
  QString m_name{};
  Device m_device{};
};

inline bool operator<(const ProgrammerDevice &d1, const ProgrammerDevice &d2) {
  return d1.name().size() < d2.name().size();
}

struct DeviceOptions {
  QString file;
  QStringList operations;
  ProgressCallback progress;
};

struct DeviceInfo {
  ~DeviceInfo() { delete flash; }
  ProgrammerDevice dev{};
  ProgrammerCable cable{};
  DeviceOptions options;
  bool isFlash{false};
  DeviceInfo *flash{nullptr};
};

enum Type {
  Fpga,
  Flash,
  Otp,
};

struct DeviceEntity {
  ProgrammerCable cable{};
  ProgrammerDevice device{};
  Type type{};
};

static inline QString HardwareFrequencyKey() {
  return QString{"HardwareFrequency/%1"};
}

static inline QString ProgrammerTitle() {
  return QString{"Raptor Programmer and Debugger"};
}

}  // namespace FOEDAG
