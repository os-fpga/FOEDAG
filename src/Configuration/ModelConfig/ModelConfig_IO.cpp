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

#include <fstream>
#include <iostream>

#include "CFGCommon/CFGArg.h"
#include "CFGCommon/CFGCommon.h"
#include "CFGCompiler/CFGCompiler.h"
#include "ModelConfig.h"

// Use cases:
//    1. [obj] -> I_BUF    ->
//    2. [obj] -> I_BUF    -> CLK_BUF  ->
//    3.       -> O_BUF    ->             [obj]
//    4. [obj] -> I_DELAY  -> I_SERDES ->         ?? not confirmed ??
//    5.       -> O_SERDES -> O_DELAY  -> [obj]   ?? not confirmed ??

namespace FOEDAG {

void ModelConfig_IO::gen_ppdb(CFGCommon_ARG* cmdarg,
                              const std::map<std::string, std::string>& options,
                              const std::string& output) {
  std::string netlist_ppdb = options.at("netlist_ppdb");
  std::string config_mapping = options.at("config_mapping");
  std::string property_json = options.find("property_json") != options.end()
                                  ? options.at("property_json")
                                  : "";
  CFG_POST_MSG("Netlist PPDB: %s", netlist_ppdb.c_str());
  CFG_POST_MSG("Config Mapping: %s", config_mapping.c_str());
  CFG_POST_MSG("Property JSON: %s", property_json.c_str());
  std::ifstream input;
  // Read the Netlist PPDB JSON
  input.open(netlist_ppdb.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json netlist_instances = nlohmann::json::parse(input);
  input.close();
  // Read the config mapping
  input.open(config_mapping.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json config_attributes_mapping = nlohmann::json::parse(input);
  input.close();
  // Read the property JSON if it exists
  nlohmann::json property_instances = nlohmann::json::object();
  if (property_json.size()) {
    input.open(property_json.c_str());
    CFG_ASSERT(input.is_open() && input.good());
    property_instances = nlohmann::json::parse(input);
    input.close();
  }
  // Merge the property
  merge_property_instances(netlist_instances, property_instances);
  // Locate the instances
  locate_instances(netlist_instances);
  // Finalize the attribute for configuration
  set_config_attributes(netlist_instances, config_attributes_mapping);
  // Remove messages
  if (netlist_instances.contains("messages")) {
    netlist_instances.erase("messages");
  }
  // Output
  std::ofstream ofile(output.c_str());
  ofile << netlist_instances.dump(2);
  ofile.close();
}

void ModelConfig_IO::validate_instance(nlohmann::json& instance) {
  CFG_ASSERT(instance.is_object());
  // Check existence
  CFG_ASSERT(instance.contains("module"));
  CFG_ASSERT(instance.contains("name"));
  CFG_ASSERT(instance.contains("linked_object"));
  CFG_ASSERT(instance.contains("linked_objects"));
  CFG_ASSERT(instance.contains("connectivity"));
  CFG_ASSERT(instance.contains("parameters"));
  // Check type
  CFG_ASSERT(instance["module"].is_string());
  CFG_ASSERT(instance["name"].is_string());
  CFG_ASSERT(instance["linked_object"].is_string());
  CFG_ASSERT(instance["linked_objects"].is_object());
  CFG_ASSERT(instance["connectivity"].is_object());
  CFG_ASSERT(instance["parameters"].is_object());
  // Check linked object
  CFG_ASSERT(instance["linked_objects"].size());
  for (auto& iter0 : instance["linked_objects"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter0.key())).is_string());
    nlohmann::json& object = iter0.value();
    CFG_ASSERT(object.is_object());
    // Check existence
    CFG_ASSERT(object.contains("location"));
    CFG_ASSERT(object.contains("properties"));
    CFG_ASSERT(!object.contains("config_attributes"));
    // Check type
    CFG_ASSERT(object["location"].is_string());
    CFG_ASSERT(object["properties"].is_object());
    for (auto& iter1 : object["properties"].items()) {
      CFG_ASSERT(((nlohmann::json)(iter1.key())).is_string());
      CFG_ASSERT(((nlohmann::json)(iter1.value())).is_string());
    }
  }
  // Check parameters
  for (auto& iter : instance["parameters"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    CFG_ASSERT(((nlohmann::json)(iter.value())).is_string());
  }
}

void ModelConfig_IO::assign_json_object(nlohmann::json& object,
                                        const std::string& key,
                                        const std::string& value,
                                        const std::string& name,
                                        const std::string& feature) {
  CFG_ASSERT(object.is_object());
  CFG_ASSERT(object.contains(key));
  CFG_ASSERT(object[key].is_string());
  std::string existing_value = std::string(object[key]);
  if (existing_value.empty()) {
    CFG_POST_MSG("Assign %s%s value:%s to \"%s\"", feature.c_str(), key.c_str(),
                 value.c_str(), name.c_str());
    object[key] = value;
  } else if (existing_value != value) {
    CFG_POST_MSG("Overwrite %s%s value:%s to \"%s\" (value existing: %s)",
                 feature.c_str(), key.c_str(), name.c_str(), value.c_str(),
                 existing_value.c_str());
    object[key] = value;
  }
}

void ModelConfig_IO::merge_property_instances(
    nlohmann::json& netlist_instances, nlohmann::json property_instances) {
  CFG_ASSERT(netlist_instances.is_object());
  CFG_ASSERT(netlist_instances.contains("instances"));
  CFG_ASSERT(netlist_instances["instances"].is_array());
  CFG_ASSERT(property_instances.is_object());
  if (property_instances.size()) {
    for (auto& instance : netlist_instances["instances"]) {
      // ToDO: Once yosys updated, remove this checking
      if (instance.contains("linked_objects")) {
        merge_property_instance(instance, property_instances);
      }
    }
  }
}

void ModelConfig_IO::merge_property_instance(
    nlohmann::json& netlist_instance, nlohmann::json property_instances) {
  validate_instance(netlist_instance);
  CFG_ASSERT(property_instances.is_object());
  CFG_ASSERT(property_instances.contains("instances"));
  CFG_ASSERT(property_instances["instances"].is_array());
  std::string module = std::string(netlist_instance["module"]);
  std::string name = std::string(netlist_instance["name"]);
  for (auto& property_instance : property_instances["instances"]) {
    CFG_ASSERT(property_instance.contains("name"));
    CFG_ASSERT(property_instance.contains("properties"));
    CFG_ASSERT(property_instance["name"].is_string());
    CFG_ASSERT(property_instance["properties"].is_object());
    for (auto& object_iter : netlist_instance["linked_objects"].items()) {
      std::string object_name = std::string(object_iter.key());
      nlohmann::json& object = object_iter.value();
      if (object_name == std::string(property_instance["name"])) {
        for (auto& iter : property_instance["properties"].items()) {
          CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
          std::string key = std::string(iter.key());
          if (CFG_find_string_in_vector({"WIRE", "CLK_BUF"}, module) >= 0 &&
              key != "PACKAGE_PIN" && key != "ROUTE_TO_FABRIC_CLK") {
            continue;
          }
          CFG_ASSERT(iter.value().is_string());
          std::string value = std::string(iter.value());
          if (!object["properties"].contains(key)) {
            object["properties"][key] = "";
          }
          assign_json_object(object["properties"], key, value, name,
                             "Property:");
        }
      }
    }
  }
}

void ModelConfig_IO::locate_instances(nlohmann::json& instances) {
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  for (auto& instance : instances["instances"]) {
    // ToDO: Once yosys updated, remove this checking
    if (instance.contains("linked_objects")) {
      locate_instance(instance);
    }
  }
}

void ModelConfig_IO::locate_instance(nlohmann::json& instance) {
  CFG_ASSERT(instance.is_object());
  validate_instance(instance);
  std::string name = std::string(instance["name"]);
  for (auto& object_iter : instance["linked_objects"].items()) {
    std::string object_name = std::string(object_iter.key());
    nlohmann::json& object = object_iter.value();
    if (object.contains("properties")) {
      CFG_ASSERT(object["properties"].is_object());
      if (object["properties"].contains("PACKAGE_PIN")) {
        CFG_ASSERT(object["properties"]["PACKAGE_PIN"].is_string());
        CFG_ASSERT(object["location"].is_string());
        std::string existing_location = std::string(object["location"]);
        std::string location = std::string(object["properties"]["PACKAGE_PIN"]);
        assign_json_object(object, "location", location, name, "");
      }
    }
  }
}

void ModelConfig_IO::set_config_attributes(nlohmann::json& instances,
                                           nlohmann::json mapping) {
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  CFG_ASSERT(mapping.is_object());
  CFG_ASSERT(mapping.contains("parameters"));
  CFG_ASSERT(mapping.contains("properties"));
  for (auto& instance : instances["instances"]) {
    // ToDO: Once yosys updated, remove this checking
    if (!instance.contains("linked_objects")) {
      CFG_ASSERT(!instance.contains("config_attributes"));
      instance["config_attributes"] = nlohmann::json::array();
      continue;
    }
    validate_instance(instance);
    std::string module = std::string(instance["module"]);
    for (auto& object_iter : instance["linked_objects"].items()) {
      std::string object_name = std::string(object_iter.key());
      nlohmann::json& object = object_iter.value();
      nlohmann::json parameters = nlohmann::json::object();
      nlohmann::json properties = nlohmann::json::object();
      nlohmann::json define = nlohmann::json::object();
      object["config_attributes"] = nlohmann::json::array();
      std::string location = std::string(object["location"]);
      if (instance.contains("parameters")) {
        parameters = instance["parameters"];
        CFG_ASSERT(parameters.is_object());
      }
      if (object.contains("properties")) {
        properties = object["properties"];
        CFG_ASSERT(properties.is_object());
      }
      if (mapping.contains("__define__")) {
        define = mapping["__define__"];
        CFG_ASSERT(define.is_object());
      }
      parameters["__location__"] = location;
      properties["__location__"] = location;
      std::map<std::string, std::string> args = {{"__location__", location}};
      set_config_attribute(object["config_attributes"], module, parameters,
                           mapping["parameters"], args, define);
      args = {{"__location__", location}};
      set_config_attribute(object["config_attributes"], module, properties,
                           mapping["properties"], args, define);
    }
  }
}

void ModelConfig_IO::set_config_attribute(
    nlohmann::json& config_attributes, const std::string& module,
    nlohmann::json inputs, nlohmann::json mapping,
    std::map<std::string, std::string>& args, nlohmann::json define) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(mapping.is_object());
  for (auto& iter : mapping.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    std::string key = std::string(iter.key());
    CFG_ASSERT(key.find(".") != std::string::npos);
    std::vector<std::string> module_feature =
        CFG_split_string(key, ".", 0, false);
    CFG_ASSERT(module_feature.size() == 2);
    if (module_feature[0] == module) {
      nlohmann::json rule_result = iter.value();
      CFG_ASSERT(rule_result.is_object());
      CFG_ASSERT(rule_result.contains("rules"));
      CFG_ASSERT(rule_result.contains("results"));
      nlohmann::json neg_results = nlohmann::json::object();
      if (rule_result.contains("neg_results")) {
        neg_results = rule_result["neg_results"];
      }
      set_config_attribute(config_attributes, inputs, rule_result["rules"],
                           rule_result["results"], neg_results, args, define);
    }
  }
}

void ModelConfig_IO::set_config_attribute(
    nlohmann::json& config_attributes, nlohmann::json inputs,
    nlohmann::json rules, nlohmann::json results, nlohmann::json neg_results,
    std::map<std::string, std::string>& args, nlohmann::json define) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(results.is_object());
  CFG_ASSERT(neg_results.is_object());
  size_t expected_match = rules.size();
  CFG_ASSERT(expected_match);
  size_t match = 0;
  for (auto& rule : rules.items()) {
    nlohmann::json key = rule.key();
    CFG_ASSERT(key.is_string());
    if (config_attribute_rule_match(inputs, std::string(key), rule.value(),
                                    args)) {
      match++;
    }
  }
  if (expected_match == match) {
    set_config_attribute(config_attributes, results, args, define);
  } else {
    set_config_attribute(config_attributes, neg_results, args, define);
  }
}

void ModelConfig_IO::set_config_attribute(
    nlohmann::json& config_attributes, nlohmann::json& results,
    std::map<std::string, std::string>& args, nlohmann::json define) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(results.is_object());
  for (auto& result : results.items()) {
    CFG_ASSERT(((nlohmann::json)(result.key())).is_string());
    std::string key = (std::string)(result.key());
    std::string name = "";
    std::string mapped_name = "";
    if (key.size() >= 9 && key.find("__other") == 0 &&
        key.rfind("__") == (key.size() - 2)) {
      nlohmann::json values = result.value();
      CFG_ASSERT(values.is_array());
      for (nlohmann::json value : values) {
        CFG_ASSERT(value.is_object());
        nlohmann::json object = nlohmann::json::object();
        for (auto& str :
             std::vector<std::string>({"__name__", "__mapped_name__",
                                       "__optional__", "__define__"})) {
          if (value.contains(str)) {
            CFG_ASSERT(value[str].is_string());
            object[str] = std::string(value[str]);
            value.erase(str);
          }
        }
        if (object.contains("__define__")) {
          std::vector<std::string> definitions = CFG_split_string(
              std::string(object["__define__"]), ";", 0, false);
          for (auto definition : definitions) {
            CFG_ASSERT(define.size());
            CFG_ASSERT(define.contains(definition));
            define_args(define[definition], args);
          }
          object.erase("__define__");
          if (object.contains("__mapped_name__")) {
            std::string value = std::string(object["__mapped_name__"]);
            for (auto arg : args) {
              value = CFG_replace_string(value, arg.first, arg.second, false);
            }
            object["__mapped_name__"] = value;
          }
        }
        for (auto& sub_result : value.items()) {
          CFG_ASSERT(((nlohmann::json)(sub_result.key())).is_string());
          std::string sub_key = (std::string)(sub_result.key());
          set_config_attribute(config_attributes, args, object, sub_key,
                               sub_result.value());
        }
      }
    } else {
      set_config_attribute(config_attributes, args, nlohmann::json::object({}),
                           key, result.value());
    }
  }
}

void ModelConfig_IO::set_config_attribute(
    nlohmann::json& config_attributes, std::map<std::string, std::string>& args,
    nlohmann::json object, std::string key, nlohmann::json value) {
  CFG_ASSERT(object.is_object());
  CFG_ASSERT(value.is_string());
  CFG_ASSERT(!object.contains(key));
  std::string final_value = std::string(value);
  for (auto arg : args) {
    final_value = CFG_replace_string(final_value, arg.first, arg.second, false);
  }
  object[key] = final_value;
  config_attributes.push_back(object);
}

bool ModelConfig_IO::config_attribute_rule_match(
    nlohmann::json inputs, const std::string& input, nlohmann::json options,
    std::map<std::string, std::string>& args) {
  bool match = false;
  CFG_ASSERT(options.is_string() || options.is_array());
  if (inputs.contains(input)) {
    if (options.is_array()) {
      for (auto& option : options) {
        CFG_ASSERT(option.is_string());
        if (inputs[input] == std::string(option)) {
          match = true;
          break;
        }
      }
    } else {
      std::string option = std::string(options);
      std::string arg_name = "";
      std::string default_value = "";
      if (get_arg_info(option, arg_name, default_value) != IS_NONE_ARG) {
        args[arg_name] = inputs[input];
        match = true;
      } else if (inputs[input] == option) {
        match = true;
      }
    }
  } else {
    if (options.is_string()) {
      std::string option = std::string(options);
      std::string arg_name = "";
      std::string default_value = "";
      if (get_arg_info(option, arg_name, default_value) ==
          IS_ARG_WITH_DEFAULT) {
        args[arg_name] = default_value;
        match = true;
      }
    }
  }
  return match;
}

void ModelConfig_IO::define_args(nlohmann::json define,
                                 std::map<std::string, std::string>& args) {
  CFG_ASSERT(define.is_object());
  for (auto& iter : define.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    std::string key = std::string(iter.key());
    nlohmann::json value = iter.value();
    CFG_ASSERT(value.is_array());
    std::vector<std::string> commands;
    std::map<std::string, std::string> str_maps;
    std::map<std::string, uint32_t> int_maps;
    if (args.find(key) != args.end()) {
      args.erase(key);
    }
    for (nlohmann::json d : value) {
      CFG_ASSERT(d.is_string());
      std::string command = std::string(d);
      for (auto arg : args) {
        command = CFG_replace_string(command, arg.first, arg.second, false);
      }
      commands.push_back(command);
    }
    CFG_Python(commands, {key}, {}, str_maps, int_maps);
    CFG_ASSERT(str_maps.size() == 1);
    CFG_ASSERT(str_maps.find(key) != str_maps.end());
    args[key] = str_maps[key];
  }
}

ARG_PROPERTY ModelConfig_IO::get_arg_info(std::string str, std::string& name,
                                          std::string& value) {
  ARG_PROPERTY result = IS_NONE_ARG;
  if (str.size() >= 7 && str.find("__arg") == 0 &&
      str.rfind("__") == (str.size() - 2)) {
    // This is an argument format string
    name = str;
    value = "";
    result = IS_ARG;
    size_t index0 = str.find("{default:");
    size_t index1 = str.find("}");
    if (index0 != std::string::npos && index1 != std::string::npos &&
        index1 > (index0 + 9)) {
      size_t default_size = index1 - index0 + 1;
      value = str.substr(index0 + 9, default_size - 10);
      name = str.erase(index0, default_size);
      result = IS_ARG_WITH_DEFAULT;
    }
  }
  return result;
}

}  // namespace FOEDAG
