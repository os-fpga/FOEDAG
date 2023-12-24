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

#define DEBUG_PRINT 0
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

class ModelConfg_DEVICE {
 public:
  ModelConfg_DEVICE(const std::string& feature, const std::string& model,
                    device* dev)
      : m_feature(feature), m_model(model), m_device(dev), m_total_bits(0) {
    CFG_ASSERT(m_model.size());
    CFG_ASSERT(m_device != nullptr);
    auto block = const_cast<device*>(m_device)->get_block(m_model);
    CFG_ASSERT(block != nullptr);
    create_bitfields(block.get(), "  ", "", "0", 0);
    CFG_ASSERT(m_total_bits);
    CFG_ASSERT((m_total_bits + 7) / 8 == m_mask.size());
    if (m_total_bits % 8) {
      for (size_t i = 0; i < m_total_bits / 8; i++) {
        CFG_ASSERT(m_mask[i] == 0xFF);
      }
      CFG_ASSERT(m_mask.back() == ((1 << (m_total_bits % 8)) - 1));
    } else {
      for (auto b : m_mask) {
        CFG_ASSERT(b == 0xFF);
      }
    }
    reset();
  }
  ~ModelConfg_DEVICE() {
    for (auto& iter : m_bitfields) {
      delete iter.second;
    }
  }
  const device* get_device() { return m_device; }
  void reset() {
    // Write a code to reset value to default
  }
  void set_attr(const std::map<std::string, std::string>& options) {
    std::string instance = options.at("instance");
    std::string name = options.at("name");
    std::string value = options.at("value");
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
  void write(const std::map<std::string, std::string>& options) {
    CFG_ASSERT(m_total_bits);
    std::string filename = options.at("file");
    std::string format = options.at("format");
    CFG_ASSERT(format == "BIT" || format == "WORD" || format == "DETAIL");
    std::ofstream file;
    file.open(filename.c_str());
    CFG_ASSERT(file.good());
    file << CFG_print("// Feature: %s\n", m_feature.c_str()).c_str();
    file << CFG_print("// Model: %s\n", m_model.c_str()).c_str();
    file << CFG_print("// Total Bits: %d\n", m_total_bits).c_str();
    file << CFG_print("// Timestamp:\n").c_str();
    uint32_t addr = 0;
    if (format == "WORD") {
      uint64_t cascaded_index = 0;
      uint64_t cascaded_value = 0;
      while (addr < m_total_bits) {
        CFG_ASSERT(m_bitfields.find(addr) != m_bitfields.end());
        const ModelConfg_BITFIELD* bitfield = m_bitfields.at(addr);
        CFG_ASSERT(addr == bitfield->m_addr);
        for (uint32_t i = 0; i < bitfield->m_size;
             i++, addr++, cascaded_index++) {
          if (bitfield->m_value & (1 << i)) {
            cascaded_value |= (uint64_t)(1) << cascaded_index;
          }
        }
        CFG_ASSERT(cascaded_index > 0 && cascaded_index < 64);
        if (cascaded_index >= 32) {
          file << CFG_print("%08X\n", (uint32_t)(cascaded_value));
          cascaded_index -= 32;
          cascaded_value >>= (uint64_t)(32);
        }
      }
      CFG_ASSERT(cascaded_index < 32);
      if (cascaded_index) {
        file << CFG_print("%08X // (Valid LSBits: %d, Dummy MSBits: %d)\n",
                          (uint32_t)(cascaded_value), cascaded_index,
                          32 - cascaded_index);
      }
    } else {
      std::string block_name = "";
      while (addr < m_total_bits) {
        CFG_ASSERT(m_bitfields.find(addr) != m_bitfields.end());
        const ModelConfg_BITFIELD* bitfield = m_bitfields.at(addr);
        CFG_ASSERT(addr == bitfield->m_addr);
        if (bitfield->m_block_name != block_name) {
          if (format == "DETAIL") {
            file << CFG_print("Block %s [%s]\n", bitfield->m_block_name.c_str(),
                              bitfield->m_user_name.c_str())
                        .c_str();
            file << "  Attributes:\n";
          }
          block_name = bitfield->m_block_name;
        }
        if (format == "DETAIL") {
          file << CFG_print(
                      "    %*s - Addr: 0x%08X, Size: %2d, Value: (0x%08X) %d\n",
                      m_max_attr_name_length, bitfield->m_name.c_str(),
                      bitfield->m_addr, bitfield->m_size, bitfield->m_value,
                      bitfield->m_value)
                      .c_str();
        } else {
          for (uint32_t i = 0; i < bitfield->m_size; i++) {
            if (bitfield->m_value & (1 << i)) {
              file << "1\n";
            } else {
              file << "0\n";
            }
          }
        }
        addr += bitfield->m_size;
      }
    }
    file.close();
    CFG_ASSERT(addr == m_total_bits);
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
                    uint32_t size, std::shared_ptr<ParameterType<int>> type) {
    CFG_ASSERT(size != 0);
    if ((addr + size) > m_total_bits) {
      m_total_bits = addr + size;
      while (((m_total_bits + 7) / 8) > m_mask.size()) {
        m_mask.push_back(0);
      }
    }
    if (bitfield_name.size() > m_max_attr_name_length) {
      m_max_attr_name_length = bitfield_name.size();
    }
    for (uint32_t i = 0, j = addr; i < size; i++, j++) {
      CFG_ASSERT((m_mask[j >> 3] & (1 << (j & 7))) == 0);
      m_mask[j >> 3] |= (1 << (j & 7));
    }
    CFG_ASSERT(m_bitfields.find(addr) == m_bitfields.end());
    m_bitfields[addr] = new ModelConfg_BITFIELD(
        block_name, user_name, bitfield_name, addr, size, type);
  }
  void create_bitfields(const device_block* block, const std::string& space,
                        const std::string& name, const std::string& addr_name,
                        uint32_t offset) {
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
        add_bitfield(name, user_name, iter.first, addr, size, attr_type);
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
        add_bitfield(name, user_name, iter.first, addr, size, nullptr);
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
      create_bitfields(inst->get_block().get(), space + "    ", child_name,
                       next_addr_name,
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
  std::vector<uint8_t> m_mask;
};

class ModelConfg_MRG {
 public:
  ModelConfg_MRG() {}
  ~ModelConfg_MRG() {
    for (auto& iter : m_feature_devices) {
      delete iter.second;
    }
  }
  void set_model(const std::map<std::string, std::string>& options) {
    std::string feature = options.at("feature");
    std::string model = options.at("model");
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
  void set_attr(const std::map<std::string, std::string>& options) {
    set_feature("set_attr", options);
    m_current_device->set_attr(options);
  }
  void write(const std::map<std::string, std::string>& options) {
    set_feature("write", options);
    m_current_device->write(options);
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
};

static ModelConfg_MRG m_mgr;

void model_config_entry(CFGCommon_ARG* cmdarg) {
  CFG_ASSERT(cmdarg->raws.size());
  std::vector<std::string> flag_options;
  std::map<std::string, std::string> options;
  std::vector<std::string> positional_options;
  if (cmdarg->raws[0] == "set_model") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {},
                  {"feature", "model"}, {}, false);
    m_mgr.set_model(options);
  } else if (cmdarg->raws[0] == "set_attr") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {},
                  {"instance", "name", "value"}, {"feature"}, false);
    m_mgr.set_attr(options);
  } else if (cmdarg->raws[0] == "write") {
    CFGArg::parse("model_config", cmdarg->raws.size(), &cmdarg->raws[0],
                  flag_options, options, positional_options, {},
                  {"file", "format"}, {"feature"}, false);
    m_mgr.write(options);
  } else {
    CFG_INTERNAL_ERROR("model_config does not support '%s' command",
                       cmdarg->raws[0].c_str());
  }
}

}  // namespace FOEDAG
