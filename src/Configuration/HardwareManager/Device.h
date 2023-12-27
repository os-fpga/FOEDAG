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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <cstdint>
#include <string>

#include "Cable.h"
#include "Tap.h"
namespace FOEDAG {

enum DeviceType { GEMINI, VIRGO, OCLA, OEM /* for all non-RS devices */ };

struct Device {
  uint32_t index;
  std::string name;
  int flashSize;
  DeviceType type;
  Cable cable;
  Tap tap;
};

}  // namespace FOEDAG

#endif  // __DEVICE_H__
