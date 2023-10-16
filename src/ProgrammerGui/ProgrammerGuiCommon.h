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
struct DeviceOptions {
  QString file;
  QStringList operations;
  ProgressCallback progress;
};

struct DeviceInfo {
  ~DeviceInfo() { delete flash; }
  Device dev{};
  Cable cable{};
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
  Cable cable{};
  Device device{};
  Type type{};
};

static inline QString HardwareFrequencyKey() {
  return QString{"HardwareFrequency/%1"};
}

static inline QString ProgrammerTitle() {
  return QString{"Raptor Programmer and Debugger"};
}

}  // namespace FOEDAG
