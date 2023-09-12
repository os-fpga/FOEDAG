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

#include "Programmer.h"

namespace FOEDAG {

class ProgrammerGuiInterface {
 public:
  virtual ~ProgrammerGuiInterface() {}
  virtual void Cables(const std::vector<Cable> &cables) = 0;
  virtual void Devices(const Cable &cable,
                       const std::vector<Device> &devices) = 0;
  virtual void Progress(const std::string &progress) = 0;
  virtual void ProgramFpga(const Cable &cable, const Device &device,
                           const std::string &file) = 0;
  virtual void Flash(const Cable &cable, const Device &device) = 0;
};
}  // namespace FOEDAG
