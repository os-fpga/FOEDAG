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

#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H

#include <Configuration/CFGCommon/CFGCommon.h>

#include <map>
#include <string>
#include <vector>

#include "nlohmann_json/json.hpp"

struct CFGCommon_ARG;

enum ARG_PROPERTY { IS_NONE_ARG, IS_ARG, IS_ARG_WITH_DEFAULT };

namespace FOEDAG {

void model_config_entry(CFGCommon_ARG* cmdarg);

class ModelConfig_IO {
 public:
  static void gen_ppdb(CFGCommon_ARG* cmdarg,
                       const std::map<std::string, std::string>& options,
                       const std::string& output);
  static void validate_instance(nlohmann::json& instance,
                                bool is_final = false);

 private:
  static void assign_json_object(nlohmann::json& object, const std::string& key,
                                 const std::string& value,
                                 const std::string& name,
                                 const std::string& feature);
  static void merge_property_instances(nlohmann::json& netlist_instances,
                                       nlohmann::json property_instances);
  static void merge_property_instance(nlohmann::json& netlist_instance,
                                      nlohmann::json property_instances);
  static void locate_instances(nlohmann::json& instances);
  static void locate_instance(nlohmann::json& instance);
  static void prepare_validate_location(
      nlohmann::json mapping, std::map<std::string, std::string>& args,
      CFG_Python_MGR& python);
  static bool validate_location(const std::string& module,
                                const std::string& name,
                                nlohmann::json& linked_objects,
                                nlohmann::json mapping,
                                std::map<std::string, std::string>& args,
                                CFG_Python_MGR& python);
  static void set_config_attributes(
      nlohmann::json& instances, nlohmann::json mapping,
      std::map<std::string, std::string> global_agrs, CFG_Python_MGR& python);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   const std::string& module,
                                   nlohmann::json inputs,
                                   nlohmann::json mapping,
                                   std::map<std::string, std::string>& args,
                                   nlohmann::json define,
                                   CFG_Python_MGR& python);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   nlohmann::json inputs, nlohmann::json rules,
                                   nlohmann::json results,
                                   nlohmann::json neg_results,
                                   std::map<std::string, std::string>& args,
                                   nlohmann::json define,
                                   CFG_Python_MGR& python);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   nlohmann::json& results,
                                   std::map<std::string, std::string>& args,
                                   nlohmann::json define,
                                   CFG_Python_MGR& python);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   std::map<std::string, std::string>& args,
                                   nlohmann::json object, std::string key,
                                   nlohmann::json value);
  static bool config_attribute_rule_match(
      nlohmann::json inputs, const std::string& input, nlohmann::json options,
      std::map<std::string, std::string>& args);
  static void define_args(nlohmann::json define,
                          std::map<std::string, std::string>& args,
                          CFG_Python_MGR& python);
  static ARG_PROPERTY get_arg_info(std::string str, std::string& name,
                                   std::string& value);
  static void write_json(nlohmann::json& instances, const std::string& file);
  static void write_json_instance(nlohmann::json& instance,
                                  std::ofstream& json);
  static void write_json_map(nlohmann::json& map, std::ofstream& json,
                             uint32_t space = 4);
  static void write_json_object(uint32_t space, const std::string& key,
                                const std::string& value, std::ofstream& json);
  static void write_json_data(const std::string& str, std::ofstream& json);
};

}  // namespace FOEDAG

#endif
