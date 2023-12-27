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

#ifndef __HARDWAREMANAGER_H__
#define __HARDWAREMANAGER_H__

#include <atomic>
#include <iostream>
#include <stdexcept>

#include "Device.h"
#include "JtagAdapter.h"
#include "Tap.h"

#define HM_USB_DESC_LENGTH (256)
#define HM_DEFAULT_CABLE_SPEED_KHZ (1000)
namespace FOEDAG {

struct HardwareManager_CABLE_INFO {
  std::string name;
  CableType type;
  uint16_t vid;
  uint16_t pid;
};

struct HardwareManager_DEVICE_INFO {
  std::string name;
  uint32_t idcode;
  uint32_t irlength;
  uint32_t irmask;
  DeviceType type;
};

class HardwareManager {
 public:
  HardwareManager(JtagAdapter *m_adapter);
  virtual ~HardwareManager();
  std::vector<Tap> get_taps(const Cable &cable);
  std::vector<Cable> get_cables();
  bool is_cable_exists(uint32_t cable_index);
  bool is_cable_exists(uint32_t cable_index, Cable &out_cable);
  bool is_cable_exists(std::string cable_name,
                       bool numeric_name_as_index = false);
  bool is_cable_exists(std::string cable_name, bool numeric_name_as_index,
                       Cable &out_cable);
  std::vector<Device> get_devices();
  std::vector<Device> get_devices(const Cable &cable);
  std::vector<Device> get_devices(uint32_t cable_index);
  std::vector<Device> get_devices(std::string cable_name,
                                  bool numeric_name_as_index = false);
  bool find_device(std::string cable_name, uint32_t device_index,
                   Device &device, std::vector<Tap> &taplist,
                   bool numeric_name_as_index = false);
  static const std::vector<HardwareManager_DEVICE_INFO> &get_device_db();

 private:
  static const std::vector<HardwareManager_CABLE_INFO> m_cable_db;
  static const std::vector<HardwareManager_DEVICE_INFO> m_device_db;
  JtagAdapter *m_adapter;
};

}  // namespace FOEDAG
#endif  //__HARDWAREMANAGER_H__
