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

#ifndef MODEL_CONFIG_IO_RESOURCE_H
#define MODEL_CONFIG_IO_RESOURCE_H

#include <Configuration/CFGCommon/CFGCommon.h>

namespace FOEDAG {

struct PIN_INFO {
  PIN_INFO(const std::string& in0, uint32_t in1, bool in2, uint32_t in3,
           uint32_t in4, uint32_t in5, const std::string& in6,
           const std::string& in7, const std::string& in8, uint32_t in9,
           uint32_t in10)
      : type(in0),
        bank(in1),
        is_clock(in2),
        index(in3),
        pair_index(in4),
        ab_io(in5),
        ab_name(in6),
        root_bank_mux_location(in7),
        root_bank_mux_resource(in8),
        root_bank_mux_core_input_index(in9),
        root_mux_input_index(in10) {
    CFG_ASSERT((type == "BOOT_CLOCK" && ab_name.size() == 0) ||
               ab_name.size() == 1);
  }
  const std::string type = "";
  const uint32_t bank = 0;
  const bool is_clock = false;
  const uint32_t index = 0;
  const uint32_t pair_index = 0;
  const uint32_t ab_io = 0;
  const std::string ab_name = "";
  const std::string root_bank_mux_location = "";
  const std::string root_bank_mux_resource = "";
  const uint32_t root_bank_mux_core_input_index = 0;
  const uint32_t root_mux_input_index = 0;
};

struct ModelConfig_IO_MODEL {
  ModelConfig_IO_MODEL(const std::string& name, const std::string& ric_name,
                       const std::string& type, const std::string& subtype,
                       uint32_t bank);
  void assign(const std::string* const_ptr, const std::string& value) const;
  void backup() const;
  void restore() const;
  void set_instantiator(const std::string& instantiator,
                        const std::string& sub_resource) const;
  void set_sub_resource(const std::string& sub_resource) const;
  const std::string m_name = "";
  const std::string m_ric_name = "";
  const std::string m_type = "";
  const std::string m_subtype = "";
  const uint32_t m_bank = 0;
  std::string m_instantiator = "";
  std::string m_backup_instantiator = "";
  std::string m_sub_resource = "";
};

struct ModelConfig_IO_RESOURCE {
  ModelConfig_IO_RESOURCE();
  ~ModelConfig_IO_RESOURCE();
  // Initialize the resource
  void add_resource(const std::string& resource, const std::string& name,
                    const std::string& ric_name, const std::string& type,
                    const std::string& subtype, uint32_t bank);
  // Query
  size_t get_resource_count(const std::string& resource);
  uint64_t get_resource_availability_index(const std::string& resource);
  std::vector<const ModelConfig_IO_MODEL*> get_used_resource(
      std::vector<const ModelConfig_IO_MODEL*>* models,
      const std::string& instantiator);
  std::vector<const ModelConfig_IO_MODEL*> get_used_resource(
      const std::string& resource, const std::string& instantiator);
  // Try to use the resource
  bool use_resource(std::vector<const ModelConfig_IO_MODEL*>* models,
                    const std::string& instantiator, const std::string& name,
                    const std::string& type, const std::string& sub_resource);
  bool use_resource(const std::string& resource,
                    const std::string& instantiator, const std::string& name,
                    const std::string& sub_resource);
  std::pair<bool, std::string> use_root_bank_clkmux(
      const std::string& module, const std::string& location,
      const std::string& sub_resource, PIN_INFO& pin_info);
  // Fail-safe mechanism
  void backup();
  void restore();
  std::map<std::string, std::vector<const ModelConfig_IO_MODEL*>*> m_resources;
  std::map<std::string, std::string> m_root_bank_clkmuxes;
  std::map<std::string, std::string> m_backup_root_bank_clkmuxes;
  std::string m_msg = "";
};

}  // namespace FOEDAG

#endif
