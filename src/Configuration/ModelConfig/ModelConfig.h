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

#include <map>
#include <string>
#include <vector>

#include "nlohmann_json/json.hpp"

struct CFGCommon_ARG;

namespace FOEDAG {

void model_config_entry(CFGCommon_ARG* cmdarg);

class ModelConfig_IO {
 public:
  static void gen_ppdb(CFGCommon_ARG* cmdarg,
                       const std::map<std::string, std::string>& options,
                       const std::string& output);

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
  static void set_config_attributes(nlohmann::json& instances,
                                    nlohmann::json mapping);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   const std::string& module,
                                   nlohmann::json inputs,
                                   nlohmann::json mapping);
  static void set_config_attribute(nlohmann::json& config_attributes,
                                   nlohmann::json inputs, nlohmann::json rules,
                                   nlohmann::json results,
                                   nlohmann::json neg_results);
  static bool config_attribute_rule_match(
      nlohmann::json inputs, const std::string& input, nlohmann::json options,
      std::map<std::string, std::string>& args);
};

}  // namespace FOEDAG

#endif
