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

#include "HardwareManager.h"

#include "Configuration/CFGCommon/CFGCommon.h"
#include "libusb.h"

namespace FOEDAG {
const std::vector<HardwareManager_CABLE_INFO> HardwareManager::m_cable_db = {
    {"RsFtdi", FTDI, 0x0403, 0x6011},
    {"RsFtdi", FTDI, 0x0403, 0x6010},
    {"RsFtdi", FTDI, 0x0403, 0x6014},
    {"Jlink", JLINK, 0x1366, 0x0101}};

const std::vector<HardwareManager_DEVICE_INFO> HardwareManager::m_device_db = {
    {"Gemini", 0x1000563d, 5, 0xffffffff, GEMINI},
    {"OCLA", 0x10000db3, 5, 0xffffffff, OCLA}};

HardwareManager::HardwareManager(JtagAdapter* adapter) : m_adapter(adapter) {
  CFG_ASSERT(m_adapter != nullptr);
}

HardwareManager::~HardwareManager() {}

std::vector<Cable> HardwareManager::get_cables() {
  struct libusb_context* ctx = nullptr;         /**< Libusb context **/
  struct libusb_device** device_list = nullptr; /**< The usb device list **/
  struct libusb_device_handle* device_handle = nullptr;
  uint32_t cable_index = 1;
  char desc_string[HM_USB_DESC_LENGTH]; /* Max size of string descriptor */
  int rc;
  int device_count;
  std::vector<Cable> cables;

  rc = libusb_init(&ctx);
  CFG_ASSERT_MSG(rc == 0, "libusb_init() fail. Error: %s",
                 libusb_error_name(rc));

  device_count = (int)libusb_get_device_list(ctx, &device_list);
  for (int index = 0; index < device_count; index++) {
    struct libusb_device_descriptor device_descriptor;

    if (libusb_get_device_descriptor(device_list[index], &device_descriptor) !=
        0) {
      continue;
    }

    for (auto& cable_info : m_cable_db) {
      if (device_descriptor.idVendor == cable_info.vid &&
          device_descriptor.idProduct == cable_info.pid) {
        Cable cable{};

        cable.index = cable_index++;
        cable.vendor_id = device_descriptor.idVendor;
        cable.product_id = device_descriptor.idProduct;
        cable.port_addr = libusb_get_port_number(device_list[index]);
        cable.device_addr = libusb_get_device_address(device_list[index]);
        cable.bus_addr = libusb_get_bus_number(device_list[index]);
        cable.name = cable_info.name + "_" + std::to_string(cable.bus_addr) +
                     "_" + std::to_string(cable.port_addr);
        cable.cable_type = cable_info.type;
        cable.description = "";
        cable.serial_number =
            "";  // Note: not all usb cable has a serial number
        cable.speed = HM_DEFAULT_CABLE_SPEED_KHZ;
        cable.transport = TransportType::JTAG;
        cable.channel = 0;

        rc = libusb_open(device_list[index], &device_handle);
        if (rc < 0) {
          // free libusb resource
          libusb_free_device_list(device_list, 1);
          libusb_exit(ctx);
          CFG_ASSERT_MSG(false, "libusb_open() fail. Error: %s",
                         libusb_error_name(rc));
        }

        rc = libusb_get_string_descriptor_ascii(
            device_handle, device_descriptor.iProduct,
            (unsigned char*)desc_string, sizeof(desc_string));
        if (rc == 0) {
          cable.description = desc_string;
        }

        rc = libusb_get_string_descriptor_ascii(
            device_handle, device_descriptor.iSerialNumber,
            (unsigned char*)desc_string, sizeof(desc_string));
        if (rc == 0) {
          cable.serial_number = desc_string;
        }

        libusb_close(device_handle);
        cables.push_back(cable);
      }
    }
  }

  if (device_list != nullptr) {
    libusb_free_device_list(device_list, 1);
  }

  if (ctx != nullptr) {
    libusb_exit(ctx);
  }

  return cables;
}

bool HardwareManager::is_cable_exists(uint32_t cable_index) {
  Cable cable;
  return is_cable_exists(cable_index, cable);
}
bool HardwareManager::is_cable_exists(uint32_t cable_index, Cable& out_cable) {
  for (const auto& cable : get_cables()) {
    if (cable.index == cable_index) {
      out_cable = cable;
      return true;
    }
  }
  return false;
}

bool HardwareManager::is_cable_exists(std::string cable_name,
                                      bool numeric_name_as_index) {
  Cable cable;
  return is_cable_exists(cable_name, numeric_name_as_index, cable);
}

bool HardwareManager::is_cable_exists(std::string cable_name,
                                      bool numeric_name_as_index,
                                      Cable& out_cable) {
  if (numeric_name_as_index) {
    bool status = false;
    uint32_t cable_index =
        (uint32_t)CFG_convert_string_to_u64(cable_name, false, &status);
    if (status) {
      return is_cable_exists(cable_index, out_cable);
    }
  }
  for (const auto& cable : get_cables()) {
    if (cable.name.find(cable_name) == 0) {
      out_cable = cable;
      return true;
    }
  }
  return false;
}

std::vector<Tap> HardwareManager::get_taps(const Cable& cable) {
  auto idcode_array = m_adapter->scan(cable);
  std::vector<Tap> taps;
  uint32_t idx = 1;

  for (auto& idcode : idcode_array) {
    for (auto& device_info : m_device_db) {
      if ((idcode & device_info.irmask) ==
          (device_info.idcode & device_info.irmask)) {
        Tap tap{};
        tap.idcode = idcode;
        tap.index = idx++;
        tap.irlength = device_info.irlength;
        taps.push_back(tap);
      }
    }
  }

  return taps;
}

std::vector<Device> HardwareManager::get_devices() {
  std::vector<Device> devices{};

  for (const auto& cable : get_cables()) {
    auto list = get_devices(cable);
    devices.insert(devices.end(), list.begin(), list.end());
  }

  return devices;
}

std::vector<Device> HardwareManager::get_devices(uint32_t cable_index) {
  for (const auto& cable : get_cables()) {
    if (cable.index == cable_index) return get_devices(cable);
  }
  return {};
}

std::vector<Device> HardwareManager::get_devices(std::string cable_name,
                                                 bool numeric_name_as_index) {
  if (numeric_name_as_index) {
    bool status = false;
    uint32_t cable_index =
        (uint32_t)CFG_convert_string_to_u64(cable_name, false, &status);
    if (status) {
      return get_devices(cable_index);
    }
  }

  for (const auto& cable : get_cables()) {
    if (cable.name.find(cable_name) == 0) return get_devices(cable);
  }
  return {};
}

bool HardwareManager::find_device(std::string cable_name, uint32_t device_index,
                                  Device& device, std::vector<Tap>& taplist,
                                  bool numeric_name_as_index) {
  if (is_cable_exists(cable_name, numeric_name_as_index)) {
    for (const auto& d : get_devices(cable_name, numeric_name_as_index)) {
      if (d.index == device_index) {
        taplist = get_taps(d.cable);
        device = d;
        return true;
      }
    }
  }
  return false;
}

std::vector<Device> HardwareManager::get_devices(const Cable& cable) {
  auto idcode_array = m_adapter->scan(cable);
  uint32_t device_index = 1;
  uint32_t tap_index = 1;
  std::vector<Device> devices{};

  for (auto& idcode : idcode_array) {
    bool found = false;
    for (auto& device_info : m_device_db) {
      if ((idcode & device_info.irmask) ==
          (device_info.idcode & device_info.irmask)) {
        found = true;
        Tap tap{tap_index++, idcode, device_info.irlength};
        if (device_info.type == GEMINI || device_info.type == OCLA ||
            device_info.type == VIRGO) {
          Device device{};
          device.index = device_index++;
          device.type = device_info.type;
          device.name = device_info.name;
          device.cable = cable;
          device.tap = tap;
          devices.push_back(device);
        }
        break;
      }
    }

    CFG_ASSERT_MSG(found, "Unknown tap id 0x%08x", idcode);
  }

  return devices;
}

const std::vector<HardwareManager_DEVICE_INFO>&
HardwareManager::get_device_db() {
  return m_device_db;
}

}  // namespace FOEDAG