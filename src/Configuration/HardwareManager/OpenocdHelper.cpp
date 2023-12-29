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

#include "OpenocdHelper.h"

#include <sstream>

namespace FOEDAG {

std::string convert_transport_to_string(TransportType transport,
                                        std::string defval) {
  switch (transport) {
    case TransportType::JTAG:
      return "jtag";
      // Handle other transport types as needed
  }
  return defval;
}

std::string build_cable_config(const Cable& cable) {
  std::ostringstream ss;

  // setup cable type specific configuration
  if (cable.cable_type == FTDI) {
    ss << " -c \"adapter driver ftdi;"
       << "ftdi vid_pid " << std::hex << std::showbase << cable.vendor_id << " "
       << cable.product_id << ";" << std::noshowbase << std::dec
       << "ftdi layout_init 0x0c08 0x0f1b;";

    if (!cable.serial_number.empty()) {
      ss << "adapter serial " << cable.serial_number << ";";
    }
  } else if (cable.cable_type == JLINK) {
    ss << " -c \"adapter driver jlink;";
  }

  // setup general cable configuration
  ss << "adapter speed " << cable.speed << ";"
     << "transport select " << convert_transport_to_string(cable.transport)
     << ";"
     << "telnet_port disabled;"
     << "gdb_port disabled;\"";

  return ss.str();
}

std::string build_tap_config(const std::vector<Tap>& taplist) {
  std::ostringstream ss;

  // setup tap configuration
  if (!taplist.empty()) {
    ss << " -c \"";
    for (const auto& tap : taplist) {
      ss << "jtag newtap tap" << tap.index << " tap"
         << " -irlen " << tap.irlength << " -expected-id " << std::hex
         << std::showbase << tap.idcode << ";" << std::noshowbase << std::dec;
    }
    ss << "\"";
  }

  return ss.str();
}

std::string build_target_config(const Device& device) {
  std::ostringstream ss;

  // setup target configuration
  if (device.type == GEMINI || device.type == VIRGO) {
    ss << " -c \"target create gemini" << device.index
       << " riscv -endian little -chain-position tap" << device.tap.index
       << ".tap;\""
       // add pld driver
       << " -c \"pld device gemini gemini" << device.index << "\"";
  } else if (device.type == OCLA) {
    ss << " -c \"target create gemini" << device.index
       << " testee -chain-position tap" << device.tap.index << ".tap;\"";
  }

  return ss.str();
}

std::string create_openocd_command(const std::string& subcmd,
                                   const Device& device,
                                   const std::vector<Tap>& taplist,
                                   const std::string& bitfile,
                                   const std::string& openocd) {
  std::stringstream ss;
  ss << " -l /dev/stdout"  // <-- not Windows friendly
     << " -d2";

  ss << build_cable_config(device.cable) << build_tap_config(taplist)
     << build_target_config(device);

  std::string cmd = "gemini load  1 " + subcmd + " " + bitfile + " -p 1 -d " +
                    (device.type == DeviceType::VIRGO ? "virgo" : "gemini");

  ss << " -c \"init\"";
  ss << " -c \"" << cmd << "\"";
  ss << " -c \"exit\"";

  std::string openocd_command = "OPENOCD_DEBUG_LEVEL=-3 " + openocd + ss.str();

  return openocd_command;
}

}  // end namespace FOEDAG
