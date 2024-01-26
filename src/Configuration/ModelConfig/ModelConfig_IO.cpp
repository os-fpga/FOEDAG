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

namespace FOEDAG {

void ModelConfig_IO::gen_ppdb(CFGCommon_ARG* cmdarg,
                              const std::map<std::string, std::string>& options,
                              const std::string& output) {
  std::string netlist_ppdb = options.find("netlist_ppdb") != options.end()
                                 ? options.at("netlist_ppdb")
                                 : "";
  std::string property_json = options.find("property_json") != options.end()
                                  ? options.at("property_json")
                                  : "";
  std::string api_dir =
      options.find("api_dir") != options.end() ? options.at("api_dir") : "";
  std::string device_name = std::string(cmdarg->device);
  CFG_string_tolower(device_name);
  std::string datapath = cmdarg->dataPath;
  if (netlist_ppdb.empty()) {
    if (cmdarg->analyzePath.size()) {
      netlist_ppdb =
          CFG_print("%s/netlist.ppdb.json", cmdarg->analyzePath.c_str());
    } else {
      netlist_ppdb = "netlist.ppdb.json";
    }
  }
  if (property_json.empty()) {
    if (cmdarg->analyzePath.size()) {
      property_json =
          CFG_print("%s/design.property.json", cmdarg->analyzePath.c_str());
    } else {
      property_json = "design.property.json";
    }
  }
  if (api_dir.empty()) {
    if (datapath.size() > 0 && device_name.size() > 0) {
      api_dir = CFG_print("%s/etc/devices/%s/model_config", datapath.c_str(),
                          device_name.c_str());
    } else {
      api_dir = "model_config";
    }
  }
#if 0
  printf("Netlist PPDB: %s\n", netlist_ppdb.c_str());
  printf("Property JSON: %s\n", property_json.c_str());
  printf("Model Config DIR: %s\n", api_dir.c_str());
#endif
  std::ifstream input;
  input.open(netlist_ppdb.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json netlist_instances = nlohmann::json::parse(input);
  input.close();
  input.open(property_json.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json property_instances = nlohmann::json::parse(input);
  input.close();
  input.open(
      CFG_print("%s/config_attributes.mapping.json", api_dir.c_str()).c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json config_attributes_mapping = nlohmann::json::parse(input);
  input.close();
  merge_instances(netlist_instances, property_instances);
  locate_instances(netlist_instances);
  link_instances(netlist_instances);
  set_config_attributes(netlist_instances, config_attributes_mapping);
  // Output
  std::ofstream ofile(output.c_str());
  ofile << netlist_instances.dump(2);
  ofile.close();
}

void ModelConfig_IO::merge_instances(nlohmann::json& netlist_instances,
                                     nlohmann::json property_instances) {
  CFG_ASSERT(netlist_instances.is_object());
  CFG_ASSERT(netlist_instances.contains("instances"));
  CFG_ASSERT(netlist_instances["instances"].is_array());
  for (auto& instance : netlist_instances["instances"]) {
    merge_instance(instance, property_instances);
  }
}

void ModelConfig_IO::merge_instance(nlohmann::json& netlist_instance,
                                    nlohmann::json property_instances) {
  CFG_ASSERT(netlist_instance.is_object());
  CFG_ASSERT(netlist_instance.contains("name"));
  CFG_ASSERT(netlist_instance.contains("module"));
  CFG_ASSERT(netlist_instance.contains("connectivity"));
  CFG_ASSERT(!netlist_instance.contains("properties"));
  CFG_ASSERT(netlist_instance["module"].is_string());
  std::string module = std::string(netlist_instance["module"]);
  nlohmann::json connectivity = netlist_instance["connectivity"];
  CFG_ASSERT(connectivity.is_object());
  CFG_ASSERT(connectivity.contains("I"));
  CFG_ASSERT(connectivity.contains("O"));
  CFG_ASSERT(connectivity["I"].is_string());
  CFG_ASSERT(connectivity["O"].is_string());
  // Use cases:
  //    1. [obj] --> I_BUF --> CLK_BUF -->
  //    2. [obj] --> I_BUF -->
  //    3. --> O_BUF --> [obj]
  //    4. [obj] --> I_DELAY --> I_SERDES     ?? not confirmed ??
  //    5. --> O_SERDES --> O_DELAY --> [obj] ?? not confirmed ??
  if (module == "CLK_BUF") {
  } else if (module == "I_BUF") {
    std::string object = std::string(connectivity["I"]);
    merge_instance_property(netlist_instance, object, property_instances);
  } else if (module == "O_BUF") {
    std::string object = std::string(connectivity["O"]);
    merge_instance_property(netlist_instance, object, property_instances);
  } else {
    // To add more module support later
    CFG_INTERNAL_ERROR("Unsupported PPDB module %s", module.c_str());
  }
}

void ModelConfig_IO::merge_instance_property(
    nlohmann::json& netlist_instance, const std::string& object,
    nlohmann::json property_instances) {
  CFG_ASSERT(property_instances.is_object());
  CFG_ASSERT(property_instances.contains("instances"));
  CFG_ASSERT(property_instances["instances"].is_array());
  for (auto& instance : property_instances["instances"]) {
    CFG_ASSERT(instance.contains("name"));
    CFG_ASSERT(instance.contains("properties"));
    CFG_ASSERT(instance["name"].is_string());
    CFG_ASSERT(instance["properties"].is_object());
    if (object == std::string(instance["name"])) {
      for (auto& iter : instance["properties"].items()) {
        CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
        CFG_ASSERT(iter.value().is_string());
        std::string key = std::string(iter.key());
        std::string value = std::string(iter.value());
        if (!netlist_instance.contains("properties")) {
          netlist_instance["properties"] = nlohmann::json::object();
        }
        netlist_instance["properties"][key] = value;
      }
    }
  }
}

void ModelConfig_IO::locate_instances(nlohmann::json& instances) {
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  for (auto& instance : instances["instances"]) {
    locate_instance(instance);
  }
}

void ModelConfig_IO::locate_instance(nlohmann::json& instance) {
  CFG_ASSERT(instance.is_object());
  CFG_ASSERT(instance.contains("module"));
  CFG_ASSERT(!instance.contains("location"));
  CFG_ASSERT(instance["module"].is_string());
  std::string module = std::string(instance["module"]);
  if (module == "I_BUF" || module == "O_BUF") {
    if (instance.contains("properties")) {
      CFG_ASSERT(instance["properties"].is_object());
      if (instance["properties"].contains("PACKAGE_PIN")) {
        CFG_ASSERT(instance["properties"]["PACKAGE_PIN"].is_string());
        instance["location"] =
            std::string(instance["properties"]["PACKAGE_PIN"]);
      }
    }
  }
}

void ModelConfig_IO::link_instances(nlohmann::json& instances) {
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  size_t instance_count = instances["instances"].size();
  size_t count = 0;
  for (auto& instance : instances["instances"]) {
    CFG_ASSERT(instance.is_object());
    CFG_ASSERT(instance.contains("module"));
    CFG_ASSERT(instance["module"].is_string());
    CFG_ASSERT(!instance.contains("linked_object"));
  }
  std::map<std::string, std::string> linked_objects;
  while (count != instance_count) {
    bool updated = false;
    for (auto& instance : instances["instances"]) {
      if (!instance.contains("linked_object")) {
        std::string module = std::string(instance["module"]);
        nlohmann::json connectivity = instance["connectivity"];
        CFG_ASSERT(connectivity.is_object());
        CFG_ASSERT(connectivity.contains("I"));
        CFG_ASSERT(connectivity.contains("O"));
        CFG_ASSERT(connectivity["I"].is_string());
        CFG_ASSERT(connectivity["O"].is_string());
        if (module == "I_BUF") {
          instance["linked_object"] = std::string(connectivity["I"]);
          linked_objects[std::string(connectivity["O"])] =
              std::string(instance["linked_object"]);
          count++;
          updated = true;
        } else if (module == "O_BUF") {
          instance["linked_object"] = std::string(connectivity["O"]);
          linked_objects[std::string(connectivity["I"])] =
              std::string(instance["linked_object"]);
          count++;
          updated = true;
        } else if (module == "CLK_BUF") {
          if (linked_objects.find(std::string(connectivity["I"])) !=
              linked_objects.end()) {
            instance["linked_object"] =
                linked_objects[std::string(connectivity["I"])];
            linked_objects[std::string(connectivity["O"])] =
                std::string(instance["linked_object"]);
            count++;
            updated = true;
          }
        } else {
          CFG_INTERNAL_ERROR("Unsupported PPDB module %s", module.c_str());
        }
      }
    }
    // if no further update, break to prevent recursive loop
    if (!updated) {
      break;
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
    CFG_ASSERT(instance.is_object());
    CFG_ASSERT(instance.contains("module"));
    CFG_ASSERT(instance["module"].is_string());
    CFG_ASSERT(!instance.contains("config_attributes"));
    std::string module = std::string(instance["module"]);
    nlohmann::json parameters = nlohmann::json::object();
    nlohmann::json properties = nlohmann::json::object();
    instance["config_attributes"] = nlohmann::json::array();
    if (instance.contains("parameters")) {
      parameters = instance["parameters"];
      CFG_ASSERT(parameters.is_object());
    }
    if (instance.contains("properties")) {
      properties = instance["properties"];
      CFG_ASSERT(properties.is_object());
    }
    set_config_attribute(instance["config_attributes"], module, parameters,
                         mapping["parameters"]);
    set_config_attribute(instance["config_attributes"], module, properties,
                         mapping["properties"]);
    // Remove config_attributes if the size=0
    if (instance["config_attributes"].size() == 0) {
      instance.erase("config_attributes");
    }
  }
}

void ModelConfig_IO::set_config_attribute(nlohmann::json& config_attributes,
                                          const std::string& module,
                                          nlohmann::json inputs,
                                          nlohmann::json mapping) {
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
      set_config_attribute(config_attributes, inputs, rule_result["rules"],
                           rule_result["results"]);
    }
  }
}

void ModelConfig_IO::set_config_attribute(nlohmann::json& config_attributes,
                                          nlohmann::json inputs,
                                          nlohmann::json rules,
                                          nlohmann::json results) {
  CFG_ASSERT(config_attributes.is_array());
  size_t expected_match = rules.size();
  CFG_ASSERT(expected_match);
  size_t match = 0;
  std::map<std::string, std::string> args;
  for (auto& rule : rules.items()) {
    nlohmann::json key = rule.key();
    CFG_ASSERT(key.is_string());
    if (config_attribute_rule_match(inputs, std::string(key), rule.value(),
                                    args)) {
      match++;
    }
  }
  if (expected_match == match) {
    for (auto& result : results.items()) {
      nlohmann::json key = result.key();
      nlohmann::json value = result.value();
      CFG_ASSERT(key.is_string());
      CFG_ASSERT(value.is_string());
      std::string final_value = std::string(value);
      for (auto arg : args) {
        final_value =
            CFG_replace_string(final_value, arg.first, arg.second, false);
      }
      config_attributes.push_back(
          nlohmann::json::object({{std::string(key), final_value}}));
    }
  }
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
      if (option.size() >= 7 && option.find("__arg") == 0 &&
          option.rfind("__") == (option.size() - 2)) {
        args[option] = inputs[input];
        match = true;
      } else if (inputs[input] == option) {
        match = true;
      }
    }
  }
  return match;
}

}  // namespace FOEDAG
