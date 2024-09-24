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

#ifndef MODEL_CONFIG_IO_H
#define MODEL_CONFIG_IO_H

#include <Configuration/CFGCommon/CFGCommon.h>

#include <map>
#include <string>
#include <vector>

#include "nlohmann_json/json.hpp"

struct ModelConfig_IO_MSG;

// clang-format off
enum MCIO_MSG_TYPE {
  IS_INFO,
  IS_WARNING,
  IS_ERROR,
  IS_DEBUG
};

enum ARG_PROPERTY {
  IS_NONE_ARG,
  IS_ARG,
  IS_ARG_WITH_DEFAULT
};
// clang-format on

struct MODEL_RESOURCE_INSTANCE {
  MODEL_RESOURCE_INSTANCE(const std::string& l, uint32_t p, uint32_t t,
                          uint32_t i)
      : location(l), possible(p), total(t), index(i) {
    CFG_ASSERT(possible != 0);
    CFG_ASSERT(total != 0);
    CFG_ASSERT(total <= 32);
    CFG_ASSERT(index < total);
    if (total != 32) {
      CFG_ASSERT(possible < (uint32_t)(1 << total));
    }
  }
  void backup() { backup_decision = decision; }
  void restore() { decision = backup_decision; }
  const std::string location = 0;
  const uint32_t possible = 0;
  const uint32_t total = 0;
  const uint32_t index = 0;
  uint32_t decision = 0;
  uint32_t backup_decision = 0;
};

typedef std::map<std::string, std::vector<MODEL_RESOURCE_INSTANCE*>>
    MODEL_RESOURCES;

struct PIN_INFO {
  PIN_INFO(const std::string& in0, uint32_t in1, bool in2, uint32_t in3,
           uint32_t in4, uint32_t in5, const std::string& in6,
           const std::string& in7)
      : type(in0),
        bank(in1),
        is_clock(in2),
        index(in3),
        pair_index(in4),
        ab_io(in5),
        ab_name(in6),
        model_name(in7) {
    CFG_ASSERT(((type == "BOOT_CLOCK" || type == "FABRIC_CLKBUF") &&
                ab_name.size() == 0) ||
               ab_name.size() == 1);
  }
  const std::string type = "";
  const uint32_t bank = 0;
  const bool is_clock = false;
  const uint32_t index = 0;
  const uint32_t pair_index = 0;
  const uint32_t ab_io = 0;
  const std::string ab_name = "";
  const std::string model_name = "";
};

namespace FOEDAG {

class ModelConfig_IO {
 public:
  ModelConfig_IO(const std::vector<std::string>& flag_options,
                 const std::map<std::string, std::string>& options,
                 const std::string& output);
  ~ModelConfig_IO();
  static void write_json(const std::string& file, bool status,
                         const std::string& feature, nlohmann::json& messages,
                         nlohmann::json& instances);

 private:
  void initialize_python(bool is_unittest, const std::string& routing_config);
  void validate_instances(nlohmann::json& instances);
  static void validate_instance(const nlohmann::json& instance,
                                bool is_final = false);
  void validate_routing(const nlohmann::json& routing, bool is_final);
  void merge_property_instances(nlohmann::json property_instances);
  void merge_property_instance(nlohmann::json& netlist_instance,
                               nlohmann::json property_instances);
  void locate_instances();
  void locate_instance(nlohmann::json& instance);
  void initialization();
  void validations(bool init, const std::string& key);
  void validation(nlohmann::json& instance, const std::string& key);
  void internal_error_validations();
  void invalidate_childs();
  void invalidate_chain(const std::string& linked_object);
  void assign_no_location_instance();
  void assign_no_location_instance_child_location(
      const std::string& linked_object);
  void prepare_instance_objects();
  nlohmann::json create_routing_object();
  nlohmann::json prepare_routing_json();
  void solve_routing_json(nlohmann::json& routings,
                          const std::string& route_model);
  std::string get_pll_port_name(std::string& pritimive_port_name);
  std::string get_iserdes_clk_mode(nlohmann::json& instance);
  void validate_routings_result(
      nlohmann::json& routing,
      std::map<std::string, std::map<std::string, std::string>>& results);
  void get_routing_result(nlohmann::json& instance, nlohmann::json& routing,
                          bool update_error);

  /*
    Functions to set configuration attributes
  */
  void set_config_attributes();
  void set_config_attribute(nlohmann::json& config_attributes,
                            const std::string& module,
                            const std::string& pre_primitive,
                            const nlohmann::json& post_primitives,
                            nlohmann::json inputs, nlohmann::json mapping,
                            nlohmann::json connectivity,
                            std::map<std::string, std::string>& args,
                            nlohmann::json define);
  void set_config_attribute_by_rules(
      nlohmann::json& config_attributes, nlohmann::json inputs,
      nlohmann::json connectivity, nlohmann::json rules, nlohmann::json results,
      nlohmann::json neg_results, std::map<std::string, std::string>& args,
      nlohmann::json define);
  void set_config_attribute_by_rule(nlohmann::json& config_attributes,
                                    nlohmann::json& results,
                                    std::map<std::string, std::string>& args,
                                    nlohmann::json define);
  void finalize_config_result(nlohmann::json& config_attributes,
                              nlohmann::json result, nlohmann::json define,
                              std::map<std::string, std::string>& args);
  bool config_attribute_rule_match(nlohmann::json inputs,
                                   nlohmann::json connectivity,
                                   const std::string& input,
                                   nlohmann::json options,
                                   std::map<std::string, std::string>& args);

  /*
    Helper function
  */
  void assign_json_object(nlohmann::json& object, const std::string& key,
                          const std::string& value, const std::string& name,
                          const std::string& feature);
  std::string get_location(const std::string& name,
                           std::string* module = nullptr,
                           uint8_t* status = nullptr);
  void set_validation_msg(bool status, std::string& msg,
                          const std::string& module, const std::string& name,
                          const std::string& location,
                          const std::string& seq_name, bool skip = false);
  static std::vector<std::string> get_json_string_list(
      const nlohmann::json& strings, std::map<std::string, std::string>& args);
  void retrieve_instance_args(nlohmann::json& instance,
                              std::map<std::string, std::string>& args);
  void define_args(nlohmann::json define,
                   std::map<std::string, std::string>& args);
  ARG_PROPERTY get_arg_info(std::string str, std::string& name,
                            std::string& value);
  void post_msg(MCIO_MSG_TYPE type, uint32_t space, const std::string& msg);
  PIN_INFO get_pin_info(const std::string& name);

  /*
    Functions to check sibling rules
  */
  bool is_siblings_match(nlohmann::json& rules,
                         const std::string& pre_primitive,
                         const nlohmann::json& post_primitives);
  bool is_siblings_match(nlohmann::json& primitive,
                         const std::string& primitive_name, bool match);
  bool is_siblings_match(nlohmann::json& primitive,
                         const nlohmann::json& postprimitives, bool match);
  /*
    Helper to write JSON
  */
  void write_json(const std::string& file);
  static void write_json_instance(nlohmann::json& instance,
                                  std::ofstream& json);
  static void write_json_object(const std::string& key,
                                const std::string& value, std::ofstream& json,
                                uint32_t space = 3);
  static void write_json_map(nlohmann::json& map, std::ofstream& json,
                             uint32_t space = 4);
  static void write_json_array(std::vector<std::string> array,
                               std::ofstream& json, uint32_t space = 4);
  static void write_json_data(const std::string& str, std::ofstream& json);

 protected:
  bool m_status = true;
  CFG_Python_MGR* m_python = nullptr;
  std::string m_pll_workaround = "";
  nlohmann::json m_instances;
  nlohmann::json m_config_mapping;
  std::map<std::string, std::string> m_global_args;
  std::vector<ModelConfig_IO_MSG*> m_messages;
  std::string m_routing_config = "";
  std::map<uint32_t, int> m_routing_instance_tracker;
};

}  // namespace FOEDAG

#endif
