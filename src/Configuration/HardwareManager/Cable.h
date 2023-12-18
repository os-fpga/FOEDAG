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

#ifndef __CABLE_H__
#define __CABLE_H__

#include <cstdint>
#include <string>
namespace FOEDAG {
enum TransportType { JTAG = 1 };

enum CableType { FTDI, JLINK };

struct Cable {
  uint32_t index;
  uint16_t vendor_id;
  uint16_t product_id;
  uint8_t bus_addr;
  uint8_t port_addr;
  uint8_t device_addr;
  uint16_t channel;
  uint32_t speed;
  std::string serial_number;
  std::string description;
  std::string name;
  TransportType transport;
  CableType cable_type;
};
struct CompareCable {
  bool operator()(const Cable& a, const Cable& b) const {
    if (a.name < b.name) return true;
    if (a.name > b.name) return false;
    return a.index < b.index;
  }
};

}  // namespace FOEDAG
#endif  // __CABLE_H__