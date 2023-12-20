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

#ifndef __OPENOCDHELPER_H__
#define __OPENOCDHELPER_H__

#include <string>
#include <vector>

#include "Device.h"

namespace FOEDAG {
std::string convert_transport_to_string(TransportType transport,
                                        std::string defval = "jtag");
std::string build_cable_config(const Cable& cable);
std::string build_tap_config(const std::vector<Tap>& taplist);
std::string build_target_config(const Device& device);
std::string create_openocd_command(const std::string& subcmd,
                                   const Device& device,
                                   const std::vector<Tap>& taplist,
                                   const std::string& bitfile,
                                   const std::string& openocd);
}  // namespace FOEDAG

#endif  // __OPENOCDHELPER_H__