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

#ifndef __JTAGADAPTER_H__
#define __JTAGADAPTER_H__

#include <cstdint>
#include <vector>

#include "Cable.h"
namespace FOEDAG {
class JtagAdapter {
 public:
  virtual std::vector<uint32_t> scan(const Cable &cable) = 0;
};
}  // namespace FOEDAG
#endif  //__JTAGADAPTER_H__
