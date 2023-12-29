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

#include "ModelConfig.h"

#include "CFGCommon/CFGArg.h"
#include "CFGCommon/CFGCommon.h"
#include "DeviceModeling/Model.h"
#include "DeviceModeling/device.h"
#include "nlohmann_json/json.hpp"

#define DEBUG_PRINT 0
#define DEBUG_PRINT_API 0
#define SUPPORT_PARAM 0

namespace FOEDAG {

struct ModelConfg_BITFIELD {
 public:
  ModelConfg_BITFIELD(const std::string& block_name,
                      const std::string& user_name, const std::string& name,
                      uint32_t addr, uint32_t size,
                      std::shared_ptr<ParameterType<int>> type)
      : m_block_name(block_name),
        m_user_name(user_name),
        m_name(name),
        m_addr(addr),
        m_size(size),
        m_type(type) {
    CFG_ASSERT(m_size > 0 && m_size <= 32);
  }
  const std::string m_block_name;
  const std::string m_user_name;
  const std::string m_name;
  const uint32_t m_addr;
  const uint32_t m_size;
  uint32_t m_default = 0;
  uint32_t m_value = 0;
  std::shared_ptr<ParameterType<int>> m_type;
};

struct ModelConfg_API_ATTRIBUTE {
 public:
  ModelConfg_API_ATTRIBUTE(const std::string& name, const std::string& value)
      : m_name(name), m_value(value) {
    CFG_ASSERT(m_name.size());
    CFG_ASSERT(m_value.size());
  }
  const std::string m_name;
  const std::string m_value;
};

struct ModelConfg_API_SETTING {
 public:
  void add_instance_equation(const std::string& instance_equation) {
    m_instance_equation = instance_equation;
  }
  void add_attr(const std::string& attr, const std::string& value) {
#if DEBUG_PRINT_API
    printf("    add_attr: %s -> %s\n", attr.c_str(), value.c_str());
#endif
    m_attributes.push_back(ModelConfg_API_ATTRIBUTE(attr, value));
  }
  std::string m_instance_equation = "";
  std::vector<ModelConfg_API_ATTRIBUTE> m_attributes;
};

struct ModelConfg_API {
 public:
  ModelConfg_API(const std::string& name) : m_name(name) {
    CFG_ASSERT(m_name.size());
  }
  ~ModelConfg_API() {
    while (m_setting.size()) {
      delete m_setting.begin()->second;
      m_setting.erase(m_setting.begin());
    }
  }
  ModelConfg_API_SETTING*& add_setting(const std::string& setting) {
#if DEBUG_PRINT_API
    printf("  add_setting: %s\n", setting.c_str());
#endif
    CFG_ASSERT(m_setting.find(setting) == m_setting.end());
    m_setting[setting] = new ModelConfg_API_SETTING();
    return m_setting[setting];
  }
  const ModelConfg_API_SETTING* get_setting(const std::string& setting) {
    if (m_setting.find(setting) != m_setting.end()) {
      return m_setting[setting];
    }
    return nullptr;
  }
  const std::string m_name;
  std::map<std::string, ModelConfg_API_SETTING*> m_setting;
};

class ModelConfg_DEVICE {
 public:
  ModelConfg_DEVICE(const std::string& feature, const std::string& model,
                    device* dev)
      : m_feature(feature), m_model(model), m_device(dev), m_total_bits(0) {
    CFG_ASSERT(m_model.size());
    CFG_ASSERT(m_device != nullptr);
    auto block = const_cast<device*>(m_device)->get_block(m_model);
    CFG_ASSERT(block != nullptr);
    std::vector<uint8_t> mask;
    create_bitfields(block.get(), mask, "  ", "", "0", 0);
    CFG_ASSERT(m_total_bits);
    CFG_ASSERT((m_total_bits + 7) / 8 == mask.size());
    if (m_total_bits % 8) {
      for (size_t i = 0; i < m_total_bits / 8; i++) {
        CFG_ASSERT(mask[i] == 0xFF);
      }
      CFG_ASSERT(mask.back() == ((1 << (m_total_bits % 8)) - 1));
    } else {
      for (auto b : mask) {
        CFG_ASSERT(b == 0xFF);
      }
    }
    reset();
  }
  ~ModelConfg_DEVICE() {
    while (m_bitfields.size()) {
      delete m_bitfields.begin()->second;
      m_bitfields.erase(m_bitfields.begin());
    }
    while (m_api.size()) {
      delete m_api.begin()->second;
      m_api.erase(m_api.begin());
    }
  }
  const device* get_device() { return m_device; }
  void reset() {
    // Write a code to reset value to default
  }
  void check_json_setting(nlohmann::json& json,
                          const std::vector<std::string>& vector) {
    CFG_ASSERT(json.is_object());
    CFG_ASSERT(json.size() == vector.size());
    for (auto& iter : json.items()) {
      nlohmann::json key = iter.key();
      CFG_ASSERT(key.is_string());
      CFG_ASSERT(CFG_find_string_in_vector(vector, (std::string)(key)) >= 0);
      CFG_ASSERT(iter.value().is_string());
    }
  }
  void add_api_setting(ModelConfg_API*& api, const std::string& setting,
                       nlohmann::json& json) {
    CFG_ASSERT(json.is_array());
    CFG_ASSERT(json.size());
    ModelConfg_API_SETTING*& set = api->add_setting(setting);
    for (auto& iter : json) {
      CFG_ASSERT(iter.is_object());
      if (iter.contains("instance")) {
        check_json_setting(iter, {"instance"});
        set->add_instance_equation((std::string)(iter["instance"]));
      } else {
        check_json_setting(iter, {"attr", "value"});
        set->add_attr((std::string)(iter["attr"]),
                      (std::string)(iter["value"]));
      }
    }
  }
  void add_api(const std::string& api, nlohmann::json& json) {
#if DEBUG_PRINT_API
    printf("add_api: %s\n", api.c_str());
#endif
    CFG_ASSERT(json.is_object());
    CFG_ASSERT(json.size());
    if (m_api.find(api) != m_api.end()) {
      delete m_api[api];
    }
    m_api[api] = new ModelConfg_API(api);
    for (auto& iter : json.items()) {
      nlohmann::json key = iter.key();
      CFG_ASSERT(key.is_string());
      add_api_setting(m_api[api], (std::string)(key), iter.value());
    }
  }
  void set_api(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    CFG_ASSERT(file.is_open() && file.good());
    nlohmann::json api = nlohmann::json::parse(file);
    file.close();
    // Must start with a dict/map
    CFG_ASSERT(api.is_object());
    CFG_ASSERT(api.size());
    for (auto& iter : api.items()) {
      nlohmann::json key = iter.key();
      CFG_ASSERT(key.is_string());
      // std::string key_str = (std::string)(key);
      add_api((std::string)(key), iter.value());
    }
  }
  void set_attr(const std::string& instance, const std::string& name,
                const std::string& value) {
    /*
    printf("set_attr: %s: %s -> %s\n", instance.c_str(), name.c_str(),
           value.c_str());
    */
    ModelConfg_BITFIELD* bitfield = get_bitfield(instance, name);
    CFG_ASSERT_MSG(bitfield != nullptr,
                   "Could not find bitfield '%s' for block instance '%s'",
                   name.c_str(), instance.c_str());
    uint32_t v = 0;
    if (is_number(value)) {
      v = (uint32_t)(CFG_convert_string_to_u64(value));
    } else {
      CFG_ASSERT(bitfield->m_type != nullptr);
      CFG_ASSERT(bitfield->m_type.get() != nullptr);
      v = bitfield->m_type.get()->get_enum_value(value);
    }
    CFG_ASSERT(bitfield->m_size == 32 ||
               (v < ((uint32_t)(1) << bitfield->m_size)));
    bitfield->m_value = v;
  }
  void set_attr(const std::map<std::string, std::string>& options) {
    std::string instance = options.at("instance");
    std::string name = options.at("name");
    std::string value = options.at("value");
    if (m_api.find(name) != m_api.end()) {
      const ModelConfg_API_SETTING* setting = m_api[name]->get_setting(value);
      CFG_ASSERT_MSG(setting != nullptr, "Could not find '%s' API setting '%s'",
                     name.c_str(), value.c_str());
      for (auto& attr : setting->m_attributes) {
        set_attr(instance, attr.m_name, attr.m_value);
      }
    } else {
      set_attr(instance, name, value);
    }
  }
  void write(const std::map<std::string, std::string>& options,
             const std::string& filename) {
    CFG_ASSERT(m_total_bits);
    std::string format = options.at("format");
    CFG_ASSERT(format == "BIT" || format == "WORD" || format == "DETAIL" ||
               format == "TCL" || format == "BIN");
    uint32_t addr = 0;
    std::vector<uint8_t> data;
    std::ofstream file;
    if (format != "BIN") {
      file.open(filename.c_str());
      CFG_ASSERT(file.is_open());
      CFG_ASSERT(file.good());
      file << CFG_print("// Feature: %s\n", m_feature.c_str()).c_str();
      file << CFG_print("// Model: %s\n", m_model.c_str()).c_str();
      file << CFG_print("// Total Bits: %d\n", m_total_bits).c_str();
      file << CFG_print("// Timestamp:\n").c_str();
      if (format == "TCL") {
        file << CFG_print("model_config set_model -feature %s %s\n",
                          m_feature.c_str(), m_model.c_str())
                    .c_str();
      }
    }
    if (format == "BIT" || format == "WORD" || format == "BIN") {
      for (uint32_t i = 0; i < (((m_total_bits + 31) / 32) * 4); i++) {
        data.push_back(0);
      }
    }
    std::string block_name = "";
    while (addr < m_total_bits) {
      CFG_ASSERT(m_bitfields.find(addr) != m_bitfields.end());
      const ModelConfg_BITFIELD* bitfield = m_bitfields.at(addr);
      CFG_ASSERT(addr == bitfield->m_addr);
      if (data.size()) {
        for (uint32_t i = 0; i < bitfield->m_size; i++, addr++) {
          if (bitfield->m_value & (1 << i)) {
            data[addr >> 3] |= (1 << (addr & 7));
          }
        }
      } else if (format == "DETAIL") {
        if (bitfield->m_block_name != block_name) {
          file << CFG_print("Block %s [%s]\n", bitfield->m_block_name.c_str(),
                            bitfield->m_user_name.c_str())
                      .c_str();
          file << "  Attributes:\n";
          block_name = bitfield->m_block_name;
        }
        file << CFG_print(
                    "    %*s - Addr: 0x%08X, Size: %2d, Value: (0x%08X) %d\n",
                    m_max_attr_name_length, bitfield->m_name.c_str(),
                    bitfield->m_addr, bitfield->m_size, bitfield->m_value,
                    bitfield->m_value)
                    .c_str();
        addr += bitfield->m_size;
      } else {
        block_name = bitfield->m_user_name.size() ? bitfield->m_user_name
                                                  : bitfield->m_block_name;
        file << CFG_print(
                    "model_config set_attr -instance %s -name %s -value %d\n",
                    block_name.c_str(), bitfield->m_name.c_str(),
                    bitfield->m_value)
                    .c_str();
        addr += bitfield->m_size;
      }
    }
    CFG_ASSERT(addr == m_total_bits);
    if (data.size()) {
      if (format == "BIT") {
        for (uint32_t i = 0; i < m_total_bits; i++) {
          if (data[i >> 3] & (1 << (i & 7))) {
            file << "1\n";
          } else {
            file << "0\n";
          }
        }
      } else if (format == "WORD") {
        uint32_t* words = (uint32_t*)(&data[0]);
        uint32_t word_count = (m_total_bits + 31) / 32;
        for (uint32_t i = 0; i < word_count; i++) {
          file << CFG_print("%08X", words[i]).c_str();
          if ((i + 1) == word_count && (m_total_bits % 32) != 0) {
            file << CFG_print(" // (Valid LSBits: %d, Dummy MSBits: %d)\n",
                              m_total_bits % 32, 32 - (m_total_bits % 32))
                        .c_str();
          } else {
            file << "\n";
          }
        }
      } else {
        CFG_write_binary_file(filename, &data[0], (m_total_bits + 7) / 8);
      }
    }
    if (file.is_open()) {
      file.flush();
      file.close();
    }
  }

 protected:
  bool is_number(const std::string& str) {
    return (str.find_first_not_of("0123456789") == std::string::npos) ||
           (str.find("0x") == 0 && str.size() > 2 &&
            str.substr(2).find_first_not_of("0123456789abcdefABCDEF") ==
                std::string::npos);
  }
  ModelConfg_BITFIELD* get_bitfield(const std::string& instance,
                                    const std::string& name) {
    ModelConfg_BITFIELD* bitfield = nullptr;
    for (auto& b : m_bitfields) {
      if ((b.second->m_block_name == instance ||
           b.second->m_user_name == instance) &&
          b.second->m_name == name) {
        bitfield = b.second;
        break;
      }
    }
    return bitfield;
  }
  void add_bitfield(const std::string& block_name, const std::string& user_name,
                    const std::string& bitfield_name, uint32_t addr,
                    uint32_t size, std::shared_ptr<ParameterType<int>> type,
                    std::vector<uint8_t>& mask) {
    CFG_ASSERT(size != 0);
    if ((addr + size) > m_total_bits) {
      m_total_bits = addr + size;
      while (((m_total_bits + 7) / 8) > mask.size()) {
        mask.push_back(0);
      }
    }
    if (bitfield_name.size() > m_max_attr_name_length) {
      m_max_attr_name_length = bitfield_name.size();
    }
    for (uint32_t i = 0, j = addr; i < size; i++, j++) {
      CFG_ASSERT((mask[j >> 3] & (1 << (j & 7))) == 0);
      mask[j >> 3] |= (1 << (j & 7));
    }
    CFG_ASSERT(m_bitfields.find(addr) == m_bitfields.end());
    m_bitfields[addr] = new ModelConfg_BITFIELD(
        block_name, user_name, bitfield_name, addr, size, type);
  }
  void create_bitfields(const device_block* block, std::vector<uint8_t>& mask,
                        const std::string& space, const std::string& name,
                        const std::string& addr_name, uint32_t offset) {
#if DEBUG_PRINT
    printf("%sBlock: %s\n", space.c_str(), block->block_name().c_str());
#endif
#if SUPPORT_PARAM
    if (block->attributes().size() || block->int_parameters().size()) {
#else
    if (block->attributes().size()) {
#endif
      std::string user_name =
          const_cast<device*>(m_device)->getCustomerName(name);
#if DEBUG_PRINT
      printf("%s  Fullname %s -> [%s]\n", name.c_str(), user_name.c_str());
#endif
      for (auto& iter : block->attributes()) {
        Parameter<int>* attr = iter.second.get();
        auto attr_type = iter.second->get_type();
        uint32_t addr = offset + (uint32_t)(attr->get_address());
        uint32_t size = (uint32_t)(attr_type->get_size());
#if DEBUG_PRINT
        printf("%s  Attribute %s - Address: %d (%s), Size: %d\n", space.c_str(),
               iter.first.c_str(), addr, addr_name.c_str(), size);
#endif
        add_bitfield(name, user_name, iter.first, addr, size, attr_type, mask);
      }
#if SUPPORT_PARAM
      for (auto& iter : block->int_parameters()) {
        Parameter<int>* param = iter.second.get();
        uint32_t addr = offset + (uint32_t)(param->get_address());
        uint32_t size = (uint32_t)(param->get_size());
#if DEBUG_PRINT
        printf("%s  Param %s - Address: %d (%s), Size: %d\n", space.c_str(),
               iter.first.c_str(), addr, addr_name.c_str(), size);
#endif
        add_bitfield(name, user_name, iter.first, addr, size, nullptr, mask);
      }
#endif
    }
    for (auto& iter : block->instances()) {
      auto inst = iter.second.get();
#if DEBUG_PRINT
#if 1
      printf("%s  Instance%ld %s: Addr %d + %d\n", space.c_str(),
             space.size() / 4, iter.first.c_str(), offset,
             inst->get_logic_address());
#else
      printf("%s  Instance%ld %s: Addr %d + %d (X%d_Y%d_Z%d)\n", space.c_str(),
             space.size() / 4, iter.first.c_str(), offset,
             inst->get_logic_address(), inst->get_logic_location_x(),
             inst->get_logic_location_y(), inst->get_logic_location_z());
#endif
#endif
      std::string child_name = iter.first.c_str();
      if (name.size()) {
        child_name = name + "." + child_name;
      }
      std::string next_addr_name =
          addr_name + " + " + std::to_string(inst->get_logic_address());
      create_bitfields(inst->get_block().get(), mask, space + "    ",
                       child_name, next_addr_name,
                       offset + (uint32_t)(inst->get_logic_address()));
    }
  }

 private:
  const std::string m_feature;
  const std::string m_model;
  const device* m_device;
  uint32_t m_total_bits = 0;
  uint32_t m_max_attr_name_length = 0;
  std::map<size_t, ModelConfg_BITFIELD*> m_bitfields;
  std::map<std::string, ModelConfg_API*> m_api;
};

static class ModelConfg_MRG {
 public:
  ModelConfg_MRG() {}
  ~ModelConfg_MRG() {
    while (m_feature_devices.size()) {
      delete m_feature_devices.begin()->second;
      m_feature_devices.erase(m_feature_devices.begin());
    }
  }
  void set_model(const std::map<std::string, std::string>& options,
                 const std::string& model) {
    std::string feature = options.at("feature");
    device* dev = Model::get_modler().get_device_model(model);
    CFG_ASSERT_MSG(dev != nullptr, "Could not find device model '%s'",
                   model.c_str());
    m_current_feature = feature;
    if (m_feature_devices.find(m_current_feature) == m_feature_devices.end()) {
      m_feature_devices[m_current_feature] =
          new ModelConfg_DEVICE(m_current_feature, model, dev);
    } else {
      CFG_ASSERT_MSG(
          m_feature_devices.at(m_current_feature)->get_device() == dev,
          "Mismatch feature '%s' device", m_current_feature.c_str());
      m_feature_devices.at(m_current_feature)->reset();
    }
    m_current_device = m_feature_devices.at(m_current_feature);
  }
  void set_api(const std::map<std::string, std::string>& options,
               const std::string& filepath) {
    set_feature("set_api", options);
    m_current_device->set_api(filepath);
  }
  void set_attr(const std::map<std::string, std::string>& options) {
    set_feature("set_attr", options);
    m_current_device->set_attr(options);
  }
  void write(const std::map<std::string, std::string>& options,
             const std::string& filename) {
    set_feature("write", options);
    m_current_device->write(options, filename);
  }

 protected:
  void set_feature(const std::string& command,
                   const std::map<std::string, std::string>& options) {
    std::string feature = options.find("feature") != options.end()
                              ? options.at("feature")
                              : m_current_feature;
    CFG_ASSERT_MSG(
        feature.size(),
        "model_config is not able to '%s' because missing 'feature' input",
        command.c_str());
    m_current_feature = feature;
    CFG_ASSERT_MSG(
        m_feature_devices.find(m_current_feature) != m_feature_devices.end(),
        "Device model for feature '%s' is not set", m_current_feature.c_str());
    m_current_device = m_feature_devices.at(m_current_feature);
  }

 private:
  std::string m_current_feature;
  ModelConfg_DEVICE* m_current_device;
  std::map<std::string, ModelConfg_DEVICE*> m_feature_devices;
} ModelConfg_DEVICE_DLL;

void model_config_entry(CFGCommon_ARG* cmdarg) {
  CFG_ASSERT(cmdarg->raws.size());
  std::vector<std::string> flag_options;
  std::map<std::string, std::string> options;
  std::vector<std::string> positional_options;
  if (cmdarg->raws[0] == "set_model") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {}, {"feature"},
                  {}, 1);
    ModelConfg_DEVICE_DLL.set_model(options, positional_options[0]);
  } else if (cmdarg->raws[0] == "set_api") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {}, {},
                  {"feature"}, 1);
    ModelConfg_DEVICE_DLL.set_api(options, positional_options[0]);
  } else if (cmdarg->raws[0] == "set_attr") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {},
                  {"instance", "name", "value"}, {"feature"}, 0);
    ModelConfg_DEVICE_DLL.set_attr(options);
  } else if (cmdarg->raws[0] == "write") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {}, {"format"},
                  {"feature"}, 1);
    ModelConfg_DEVICE_DLL.write(options, positional_options[0]);
  } else {
    CFG_INTERNAL_ERROR("model_config does not support '%s' command",
                       cmdarg->raws[0].c_str());
  }
}

}  // namespace FOEDAG
