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

#include "ModelConfig_IO.h"

#include <fstream>
#include <iostream>

#include "CFGCommon/CFGArg.h"
#include "CFGCommon/CFGCommon.h"
#include "CFGCompiler/CFGCompiler.h"

#define ENABLE_DEBUG_MSG (0)
#define POST_INFO_MSG(space, ...) \
  { post_msg(MCIO_MSG_TYPE::IS_INFO, space, CFG_print(__VA_ARGS__)); }
#define POST_WARN_MSG(space, ...) \
  { post_msg(MCIO_MSG_TYPE::IS_WARNING, space, CFG_print(__VA_ARGS__)); }
#define POST_ERROR_MSG(space, ...) \
  { post_msg(MCIO_MSG_TYPE::IS_ERROR, space, CFG_print(__VA_ARGS__)); }
#define POST_DEBUG_MSG(space, ...) \
  { post_msg(MCIO_MSG_TYPE::IS_DEBUG, space, CFG_print(__VA_ARGS__)); }

const bool g_enable_python = true;

struct ModelConfig_IO_MSG {
  ModelConfig_IO_MSG(uint32_t o, const std::string& m) : offset(o), msg(m) {
#if ENABLE_DEBUG_MSG
    printf("DEBUG: ");
    for (uint32_t i = 0; i < offset; i++) {
      printf("  ");
    }
    printf("%s\n", msg.c_str());
#endif
  }
  const uint32_t offset = 0;
  const std::string msg = "";
};

namespace FOEDAG {

/*
  Constructor
*/
ModelConfig_IO::ModelConfig_IO(
    CFGCommon_ARG* cmdarg, const std::vector<std::string>& flag_options,
    const std::map<std::string, std::string>& options,
    const std::string& output) {
  std::string netlist_ppdb = options.at("netlist_ppdb");
  std::string config_mapping = options.at("config_mapping");
  std::string property_json = options.find("property_json") != options.end()
                                  ? options.at("property_json")
                                  : "";
  bool is_unittest = std::find(flag_options.begin(), flag_options.end(),
                               "is_unittest") != flag_options.end();
  if (!is_unittest) {
    POST_INFO_MSG(0, "Netlist PPDB: %s", netlist_ppdb.c_str());
    POST_INFO_MSG(0, "Config Mapping: %s", config_mapping.c_str());
    POST_INFO_MSG(0, "Property JSON: %s", property_json.c_str());
  }
  std::ifstream input;
  // Read the Netlist PPDB JSON
  input.open(netlist_ppdb.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  nlohmann::json netlist_instances = nlohmann::json::parse(input);
  input.close();
  // Read the config mapping
  input.open(config_mapping.c_str());
  CFG_ASSERT(input.is_open() && input.good());
  m_config_mapping = nlohmann::json::parse(input);
  input.close();
  CFG_ASSERT(m_config_mapping.is_object());
  CFG_ASSERT(m_config_mapping.contains("parameters"));
  CFG_ASSERT(m_config_mapping.contains("properties"));
  // Read the property JSON if it exists
  nlohmann::json property_instances = nlohmann::json::object();
  if (property_json.size()) {
    input.open(property_json.c_str());
    CFG_ASSERT(input.is_open() && input.good());
    property_instances = nlohmann::json::parse(input);
    input.close();
  }
  CFG_Python_MGR python;
  // Base on device name, use the appropriate router
  m_resource = &m_62x44_resource;
  // Validate instances
  validate_instances(netlist_instances);
  // Merge the property
  merge_property_instances(property_instances);
  // Locate the instances
  locate_instances();
  // Prepare for validation for location
  if (g_enable_python) {
    initialization(python);
  } else {
    POST_WARN_MSG(0, "Skip pin assignment legality check");
  }
  // Validate the location
  validations(true, "__primary_validation__", python);
  // Invalid children if parent is invalid
  invalidate_childs();
  // Assign location to Boot Clock
  assign_boot_clock_location();
  // Allocate FCLK routing
  allocate_fclk_routing();
  // Set CLKBUF configuration attributes
  set_clkbuf_config_attributes();
  // Allocate PLL
  allocate_pll();
  // Set PLL (mainly on fclk)
  set_pll_config_attributes();
  // Remaining validation
  validations(false, "__secondary_validation__", python);
  // Finalize the attribute for configuration
  set_config_attributes(python);
  // Print warning
  for (auto& instance : m_instances) {
    validate_instance(instance, true);
    if (!instance["__validation__"]) {
      POST_WARN_MSG(0, "Generated IO bitstream is invalid");
      break;
    }
  }
  // Output
  write_json(output);
}

/*
  Destructor
*/
ModelConfig_IO::~ModelConfig_IO() {
  while (m_messages.size()) {
    delete m_messages.back();
    m_messages.pop_back();
  }
}

/*
  Entry function to validate JSON format of instances
  Store the instances as m_instances
*/
void ModelConfig_IO::validate_instances(nlohmann::json& instances) {
  POST_INFO_MSG(0, "Validate Instances");
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  CFG_ASSERT(m_instances.is_null());
  m_instances = instances["instances"];
  for (auto& instance : m_instances) {
    validate_instance(instance);
  }
}

/*
  Real function to validate JSON format of instance
*/
void ModelConfig_IO::validate_instance(nlohmann::json& instance,
                                       bool is_final) {
  CFG_ASSERT(instance.is_object());
  // Check existence
  CFG_ASSERT(instance.contains("module"));
  CFG_ASSERT(instance.contains("name"));
  CFG_ASSERT(instance.contains("linked_object"));
  CFG_ASSERT(instance.contains("linked_objects"));
  CFG_ASSERT(instance.contains("connectivity"));
  CFG_ASSERT(instance.contains("parameters"));
  CFG_ASSERT(instance.contains("pre_primitive"));
  CFG_ASSERT(instance.contains("post_primitives"));
  CFG_ASSERT(instance.contains("route_clock_to"));
  if (is_final) {
    CFG_ASSERT(instance.contains("route_clock_result"));
  }
  // Check type
  CFG_ASSERT(instance["module"].is_string());
  CFG_ASSERT(instance["name"].is_string());
  CFG_ASSERT(instance["linked_object"].is_string());
  CFG_ASSERT(instance["linked_objects"].is_object());
  CFG_ASSERT(instance["connectivity"].is_object());
  CFG_ASSERT(instance["parameters"].is_object());
  CFG_ASSERT(instance["pre_primitive"].is_string());
  CFG_ASSERT(instance["post_primitives"].is_array());
  CFG_ASSERT(instance["route_clock_to"].is_object());
  if (is_final) {
    CFG_ASSERT(instance["route_clock_result"].is_object());
  }
  // Check linked object
  CFG_ASSERT(instance["linked_objects"].size());
  for (auto& iter0 : instance["linked_objects"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter0.key())).is_string());
    nlohmann::json& object = iter0.value();
    CFG_ASSERT(object.is_object());
    // Check existence
    CFG_ASSERT(object.contains("location"));
    CFG_ASSERT(object.contains("properties"));
    if (is_final) {
      CFG_ASSERT(object.contains("config_attributes"));
    } else {
      CFG_ASSERT(!object.contains("config_attributes"));
    }
    // Check type
    CFG_ASSERT(object["location"].is_string());
    CFG_ASSERT(object["properties"].is_object());
    if (is_final) {
      CFG_ASSERT(object["config_attributes"].is_array());
    }
    for (auto& iter1 : object["properties"].items()) {
      CFG_ASSERT(((nlohmann::json)(iter1.key())).is_string());
      CFG_ASSERT(((nlohmann::json)(iter1.value())).is_string());
    }
    if (is_final) {
      for (auto& cfg_attr : object["config_attributes"]) {
        CFG_ASSERT(cfg_attr.is_object());
        for (auto& iter2 : cfg_attr.items()) {
          CFG_ASSERT(((nlohmann::json)(iter2.key())).is_string());
          CFG_ASSERT(((nlohmann::json)(iter2.value())).is_string());
        }
      }
    }
  }
  // Check parameters
  for (auto& iter : instance["parameters"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    CFG_ASSERT(((nlohmann::json)(iter.value())).is_string());
  }
  // Check post_primitives
  for (auto& post_primitive : instance["post_primitives"]) {
    CFG_ASSERT(post_primitive.is_string());
  }
  // Check route_clock_to
  for (auto& iter : instance["route_clock_to"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    nlohmann::json dest = iter.value();
    CFG_ASSERT(dest.is_array());
    for (auto& d : dest) {
      CFG_ASSERT(d.is_string());
    }
  }
  if (is_final) {
    // Check route_clock_result
    for (auto& iter : instance["route_clock_result"].items()) {
      CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
      nlohmann::json dest = iter.value();
      CFG_ASSERT(dest.is_array());
      for (auto& d : dest) {
        CFG_ASSERT(d.is_string());
      }
    }
  }
  // Check validation
  if (is_final) {
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
  }
}

/*
  Entry function to merge the property json into instances
*/
void ModelConfig_IO::merge_property_instances(
    nlohmann::json property_instances) {
  POST_INFO_MSG(0, "Merge properties into instances");
  CFG_ASSERT(property_instances.is_object());
  if (property_instances.size()) {
    for (auto& instance : m_instances) {
      merge_property_instance(instance, property_instances);
    }
  }
}

/*
  Real function to merge the property json into instance
*/
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

/*
  Entry function to re-location instances
*/
void ModelConfig_IO::locate_instances() {
  POST_INFO_MSG(0, "Re-location instances");
  for (auto& instance : m_instances) {
    locate_instance(instance);
  }
}

/*
  Real function to re-location instance
*/
void ModelConfig_IO::locate_instance(nlohmann::json& instance) {
  CFG_ASSERT(instance.is_object());
  validate_instance(instance);
  std::string name = std::string(instance["name"]);
  for (auto& object_iter : instance["linked_objects"].items()) {
    std::string object_name = std::string(object_iter.key());
    nlohmann::json& object = object_iter.value();
    nlohmann::json& properties = object["properties"];
    if (properties.contains("PACKAGE_PIN")) {
      std::string location = (std::string)(properties["PACKAGE_PIN"]);
      assign_json_object(object, "location", location, name, "");
    }
  }
}

/*
  Call config mapping initialization if it was defined
*/
void ModelConfig_IO::initialization(CFG_Python_MGR& python) {
  POST_INFO_MSG(0, "Configure Mapping file initialization");
  if (m_config_mapping.contains("__init__")) {
    define_args(m_config_mapping["__init__"], m_global_args, python);
  }
}

/*
  Entry function to perform validation based on mapping
*/
void ModelConfig_IO::validations(bool init, const std::string& key,
                                 CFG_Python_MGR& python) {
  POST_INFO_MSG(0, "Validation using '%s' rule", key.c_str());
  MODEL_RESOURCES resource_instances;
  for (auto& instance : m_instances) {
    // This is the most basic validation which will be run first
    // Hence it is impossible there is any object key
    validate_instance(instance);
    if (init) {
      CFG_ASSERT(!instance.contains("__validation__"));
      CFG_ASSERT(!instance.contains("__validation_msg__"));
    } else {
      CFG_ASSERT(instance.contains("__validation__"));
      CFG_ASSERT(instance.contains("__validation_msg__"));
      CFG_ASSERT(instance["__validation__"].is_boolean());
      CFG_ASSERT(instance["__validation_msg__"].is_string());
    }
    if (g_enable_python) {
      if (init || instance["__validation__"]) {
        validation(instance, resource_instances, python, key);
      }
    } else {
      instance["__validation__"] = false;
      instance["__validation_msg__"] = "Skip because Python is ignored";
    }
  }
  for (auto iter : resource_instances) {
    while (iter.second.size()) {
      delete iter.second.back();
      iter.second.pop_back();
    }
  }
}

void ModelConfig_IO::validation(nlohmann::json& instance,
                                MODEL_RESOURCES& resource_instances,
                                CFG_Python_MGR& python,
                                const std::string& key) {
  validate_instance(instance);
  std::string module = std::string(instance["module"]);
  CFG_ASSERT(module.size());
  std::string name = std::string(instance["name"]);
  nlohmann::json& linked_objects = instance["linked_objects"];
  CFG_ASSERT(linked_objects.is_object());
  std::string msg = "";
  if (instance.contains("__validation__")) {
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    msg = (std::string)(instance["__validation_msg__"]);
    if (msg.find("Skipped.") == 0) {
      // Reset the message since "Skipped" message is meaningless
      msg = "";
    }
  }
  if (m_config_mapping.contains(key)) {
    nlohmann::json validation_rules = m_config_mapping[key];
    CFG_ASSERT(validation_rules.is_object());
    if (validation_rules.contains("__seqeunce__")) {
      // Get the locations
      size_t i = 0;
      std::map<std::string, std::string> args = m_global_args;
      std::string locations = "";
      for (auto& object_iter : linked_objects.items()) {
        nlohmann::json& object = object_iter.value();
        std::string location = std::string(object["location"]);
        args[CFG_print("__location%d__", i)] = location;
        if (i == 0) {
          locations = location;
        } else {
          locations = CFG_print("%s,%s", locations.c_str(), location.c_str());
        }
        i++;
      }
      // Loop through all the validation checking sequence
      nlohmann::json sequence = validation_rules["__seqeunce__"];
      CFG_ASSERT(sequence.is_array());
      bool status = true;
      for (auto seq : sequence) {
        CFG_ASSERT(seq.is_string());
        std::string seq_name = std::string(seq);
        CFG_ASSERT(validation_rules.contains(seq_name));
        nlohmann::json validation_info = validation_rules[seq_name];
        CFG_ASSERT(validation_info.is_object());
        CFG_ASSERT(validation_info.contains("__module__"));
        CFG_ASSERT(validation_info.contains("__equation__"));
        nlohmann::json modules = validation_info["__module__"];
        nlohmann::json __equation__ = validation_info["__equation__"];
        CFG_ASSERT(modules.is_array());
        CFG_ASSERT(__equation__.is_array());
        for (auto m : modules) {
          CFG_ASSERT(m.is_string());
          if ((std::string(m) == "__all__" || std::string(m) == module) &&
              is_siblings_match(validation_info,
                                (std::string)(instance["pre_primitive"]),
                                instance["post_primitives"])) {
            bool is_resource = false;
            if (validation_info.contains("__connectivity__")) {
              CFG_ASSERT(validation_info["__connectivity__"].is_array());
              uint32_t connectivity_count = 0;
              for (auto& con : validation_info["__connectivity__"]) {
                CFG_ASSERT(con.is_string());
                if (instance["connectivity"].contains(std::string(con))) {
                  connectivity_count++;
                }
              }
              args["__connectivity_count__"] =
                  CFG_print("%d", connectivity_count);
            }
            bool parameter_status = true;
            if (validation_info.contains("__parameter__")) {
              CFG_ASSERT(validation_info["__parameter__"].is_array());
              for (auto& param : validation_info["__parameter__"]) {
                CFG_ASSERT(param.is_string());
                std::string parameter = std::string(param);
                if (instance["parameters"].contains(parameter)) {
                  args[parameter] = instance["parameters"][parameter];
                } else {
                  parameter_status = false;
                  break;
                }
              }
            }
            if (!parameter_status) {
              // Does not have enough parameter to do the validation
              set_validation_msg(parameter_status, msg, module, name, locations,
                                 seq_name);
              break;
            }
            std::vector<std::string> equations =
                get_json_string_list(__equation__, args);
            if (validation_info.contains("__resource__")) {
              CFG_ASSERT(validation_info["__resource__"].is_boolean());
              is_resource = (bool)(validation_info["__resource__"]);
            }
            if (is_resource) {
              python.run(equations, {"__resource_name__", "__location__",
                                     "__resource__", "__total_resource__"});
            } else {
              python.run(equations, {"pin_result"});
            }
            if (is_resource) {
              CFG_ASSERT(python.results().size() == 4);
              std::string name = python.result_str("__resource_name__");
              std::string location = python.result_str("__location__");
              uint32_t resource = python.result_u32("__resource__");
              uint32_t total = python.result_u32("__total_resource__");
              if (resource_instances.find(name) == resource_instances.end()) {
                resource_instances[name] =
                    std::vector<MODEL_RESOURCE_INSTANCE*>({});
              }
              MODEL_RESOURCE_INSTANCE* new_instance =
                  new MODEL_RESOURCE_INSTANCE(
                      location, resource, total,
                      (uint32_t)(resource_instances[name].size()));
              status = allocate_resource(resource_instances[name], new_instance,
                                         false);
              if (status && validation_info.contains("__if_resource_pass__")) {
                std::string updated_resource = "__updated_resource__ = {";
                size_t i = 0;
                for (auto& r : resource_instances[name]) {
                  if (i) {
                    updated_resource += ",";
                  }
                  updated_resource =
                      CFG_print("%s '%s' : %d", updated_resource.c_str(),
                                r->location.c_str(), r->decision);
                }
                updated_resource += " }";
                std::vector<std::string> equations = get_json_string_list(
                    validation_info["__if_resource_pass__"], args);
                equations.insert(equations.begin(), updated_resource);
                python.run(equations, {});
              }
              if (!status && validation_info.contains("__if_resource_fail__")) {
                std::vector<std::string> equations = get_json_string_list(
                    validation_info["__if_resource_fail__"], args);
                python.run(equations, {});
              }
              set_validation_msg(status, msg, module, name, locations,
                                 seq_name);
            } else {
              CFG_ASSERT(python.results().size() == 1);
              status = python.result_bool("pin_result");
              set_validation_msg(status, msg, module, name, locations,
                                 seq_name);
            }
            // If you hit one module, no need to check the rest
            break;
          }
        }
        if (!status) {
          break;
        }
      }
      instance["__validation__"] = status;
      instance["__validation_msg__"] = msg;
    } else {
      instance["__validation__"] = true;
      instance["__validation_msg__"] = CFG_print(
          "Skipped. No validation sequence is defined for key %s", key.c_str());
    }
  } else {
    instance["__validation__"] = true;
    instance["__validation_msg__"] =
        CFG_print("Skipped. No validation is needed for key %s", key.c_str());
  }
}

/*
  Entry function to invalidate the childs associated with the parent (if it is
  invalid).

  This should be called right after validations(__location_validation__)

  validations(__location_validation__) invalidate PORT primitives (always be
  parent) if any
*/
void ModelConfig_IO::invalidate_childs() {
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (!instance["__validation__"]) {
      std::string linked_object = (std::string)(instance["linked_object"]);
      invalidate_child(linked_object);
    }
  }
}

/*
  Real function to invalidate the childs associated with the parent (if it is
  invalid)
*/
void ModelConfig_IO::invalidate_child(const std::string& linked_object) {
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if ((std::string)(instance["linked_object"]) == linked_object &&
        instance["__validation__"]) {
      instance["__validation__"] = false;
      instance["__validation_msg__"] = "Invalidated because parent is invalid";
    }
  }
}

/*
  Assign location to BOOT_CLOCK
*/
void ModelConfig_IO::assign_boot_clock_location() {
  POST_INFO_MSG(0, "Assign Boot Clock location");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && instance["module"] == "BOOT_CLOCK") {
      for (auto iter : instance["linked_objects"].items()) {
        nlohmann::json& object = iter.value();
        CFG_ASSERT(((std::string)(object["location"])).size() == 0);
        object["location"] = CFG_print("__SKIP_LOCATION_CHECK__:%s",
                                       ((std::string)(iter.key())).c_str());
      }
      assign_boot_clock_child_location(instance["linked_object"]);
    }
  }
}

/*
  Assign location to BOOT_CLOCK child
*/
void ModelConfig_IO::assign_boot_clock_child_location(
    const std::string& linked_object) {
  POST_INFO_MSG(1, "Assign Boot Clock child location");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && instance["module"] != "BOOT_CLOCK" &&
        instance["linked_object"] == linked_object) {
      for (auto iter : instance["linked_objects"].items()) {
        nlohmann::json& object = iter.value();
        CFG_ASSERT(((std::string)(object["location"])).size() == 0);
        object["location"] = CFG_print("__SKIP_LOCATION_CHECK__:%s",
                                       ((std::string)(iter.key())).c_str());
      }
    }
  }
}

/*
  Determine the FCLK resource ultilization
*/
void ModelConfig_IO::allocate_fclk_routing() {
  POST_INFO_MSG(0, "Allocate FCLK routing resource");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    CFG_ASSERT(!instance.contains("route_clock_result"));
    instance["route_clock_result"] = nlohmann::json::object();
    if (instance["__validation__"]) {
      if (instance["module"] == "CLK_BUF") {
        m_resource->backup();
        allocate_clkbuf_fclk_routing(instance, "O");
        if (!instance["__validation__"]) {
          m_resource->restore();
        }
      } else if (instance["module"] == "PLL") {
        std::vector<std::string> port_sequence = {
#if 1
          "CLK_OUT_DIV4",
          "CLK_OUT_DIV3",
          "CLK_OUT_DIV2",
          "CLK_OUT"
        };
#else
          "CLK_OUT",
          "CLK_OUT_DIV2",
          "CLK_OUT_DIV3",
          "CLK_OUT_DIV4"
        };
#endif
        m_resource->backup();
        for (auto port : port_sequence) {
          if (instance["route_clock_to"].contains(port)) {
            allocate_pll_fclk_routing(instance, port);
            if (!instance["__validation__"]) {
              m_resource->restore();
              break;
            }
          }
        }
      }
    }
  }
}

/*
  Determine the FCLK resource ultilization by CLKBUF
  Unittest: All negative tests included
*/
void ModelConfig_IO::allocate_clkbuf_fclk_routing(nlohmann::json& instance,
                                                  const std::string& port) {
  validate_instance(instance);
  CFG_ASSERT(instance["__validation__"]);
  CFG_ASSERT(instance.contains("route_clock_result"));
  std::string name = instance["name"];
  std::string src_location = get_location(name);
  PIN_INFO src_pin_info(src_location);
  std::string src_type = src_pin_info.type;
  CFG_string_tolower(src_type);
  nlohmann::json dest_instances = nlohmann::json::array();
  nlohmann::json& result = instance["route_clock_result"];
  CFG_ASSERT(result.is_object());
  if (instance["route_clock_to"].contains(port)) {
    dest_instances = instance["route_clock_to"][port];
    result[port] = nlohmann::json::array();
  }
  POST_DEBUG_MSG(1, "CLKBUF %s (location:%s)", name.c_str(),
                 src_location.c_str());
  for (auto& dinstance : dest_instances) {
    std::string dest_instance = (std::string)(dinstance);
    std::string dest_module = "";
    std::string dest_location = get_location(dest_instance, &dest_module);
    std::string err_msg = "";
    POST_DEBUG_MSG(2, "Route to gearbox module %s (location:%s)",
                   dest_instance.c_str(), dest_location.c_str());
    if (dest_location.size()) {
      PIN_INFO dest_pin_info(dest_location);
      // Pin only can route to same within same type and same bank
      if (src_pin_info.type == dest_pin_info.type &&
          src_pin_info.bank == dest_pin_info.bank) {
        std::string type = dest_pin_info.type;
        CFG_string_tolower(type);
        char ab = char('A') + char(dest_pin_info.rx_io);
        std::string fclk_name =
            CFG_print("%s_fclk_%d_%c", type.c_str(), dest_pin_info.bank, ab);
        if (m_resource->use_fclk(src_location, fclk_name)) {
          result[port].push_back(m_resource->m_msg);
          POST_DEBUG_MSG(3, m_resource->m_msg.c_str());
        } else {
          err_msg = clkbuf_routing_failure_msg(
              name, src_location, dest_instance, dest_module, dest_location);
          err_msg = CFG_print("%s. Reason: %s", err_msg.c_str(),
                              m_resource->m_msg.c_str());
        }
      } else {
        err_msg = clkbuf_routing_failure_msg(name, src_location, dest_instance,
                                             dest_module, dest_location);
        err_msg = CFG_print("%s. Reason: They are not in same physical bank",
                            err_msg.c_str());
      }
    } else {
      err_msg =
          clkbuf_routing_failure_msg(name, src_location, dest_instance, "", "");
      err_msg =
          CFG_print("%s. Reason: Module usage is invalid in the first place",
                    err_msg.c_str());
    }
    if (err_msg.size()) {
      instance["__validation__"] = false;
      instance["__validation_msg__"] = "Fail to route the clock";
      result[port].push_back(err_msg);
      POST_WARN_MSG(3, err_msg.c_str());
    }
  }
  if (instance["__validation__"]) {
    // Only route to fabric if there is parameter 'ROUTE_TO_FABRIC_CLK' exists
    if (instance["parameters"].contains("ROUTE_TO_FABRIC_CLK")) {
      POST_DEBUG_MSG(2, "Route clock-capable pin %s (location:%s) to fabric",
                     name.c_str(), src_location.c_str());
      char ab = char('A') + char(src_pin_info.rx_io);
      std::string fclk_name =
          CFG_print("%s_fclk_%d_%c", src_type.c_str(), src_pin_info.bank, ab);
      if (m_resource->use_fclk(src_location, fclk_name)) {
        POST_DEBUG_MSG(3, m_resource->m_msg.c_str());
      } else {
        instance["__validation__"] = false;
        std::string msg = CFG_print(
            "Not able to route clock-capable pin %s (location:%s) to fabric. "
            "Reason: %s",
            name.c_str(), src_location.c_str(), m_resource->m_msg.c_str());
        instance["__validation_msg__"] = msg;
        POST_WARN_MSG(3, msg.c_str());
      }
    } else {
      POST_DEBUG_MSG(2,
                     "It is not used by fabric. Skip FCLK resource allocation");
    }
  }
}

/*
  Determine the FCLK resource ultilization by PLL
  Unittest: All negative tests included
*/
void ModelConfig_IO::allocate_pll_fclk_routing(nlohmann::json& instance,
                                               const std::string& port) {
  validate_instance(instance);
  CFG_ASSERT(instance["__validation__"]);
  CFG_ASSERT(instance["route_clock_to"].contains(port));
  CFG_ASSERT(instance.contains("route_clock_result"));
  std::string name = instance["name"];
  std::string src_location = get_location(name);
  PIN_INFO src_pin_info(src_location);
  nlohmann::json dest_instances = instance["route_clock_to"][port];
  nlohmann::json& result = instance["route_clock_result"];
  CFG_ASSERT(result.is_object());
  result[port] = nlohmann::json::array();
  POST_DEBUG_MSG(1, "PLL %s Port %s (location:%s)", name.c_str(), port.c_str(),
                 src_location.c_str());
  if (port == "CLK_OUT") {
    for (auto& dinstance : dest_instances) {
      std::string dest_instance = (std::string)(dinstance);
      std::string dest_module = "";
      std::string dest_location = get_location(dest_instance, &dest_module);
      std::string err_msg = "";
      POST_DEBUG_MSG(2, "Route to gearbox module %s (location:%s)",
                     dest_instance.c_str(), dest_location.c_str());
      if (dest_location.size()) {
        PIN_INFO dest_pin_info(dest_location);
        // Pin only can route to same within same type and same bank
        if ((src_pin_info.type == "HVR" && dest_pin_info.type == "HVL") ||
            (src_pin_info.type == "HVL" && dest_pin_info.type == "HVR")) {
          std::string pll = (src_pin_info.type == "HVL") ? "PLL #0" : "PLL #1";
          err_msg =
              pll_routing_failure_msg(name, port, src_location, dest_instance,
                                      dest_module, dest_location);
          err_msg =
              CFG_print("%s. Reason: %s (needed by %s) cannot route to %s",
                        err_msg.c_str(), pll.c_str(), src_pin_info.type.c_str(),
                        dest_pin_info.type.c_str());
        } else {
          std::string type = dest_pin_info.type;
          uint32_t fclk = (dest_pin_info.index < 20) ? 0 : 1;
          char ab = char('A') + char(fclk);
          std::string fclk_name =
              CFG_print("%s_fclk_%d_%c", CFG_string_tolower(type).c_str(),
                        dest_pin_info.bank, ab);
          std::string fclk_src = CFG_print("PLL:%s", src_location.c_str());
          if (m_resource->use_fclk(fclk_src, fclk_name)) {
            std::vector<const ModelConfig_IO_MODEL*> fclks =
                m_resource->get_used_fclk(fclk_src);
            uint32_t requested_pll_resource = 0;
            for (auto& fclk : fclks) {
              uint32_t requested_pll = m_resource->fclk_use_pll(fclk->m_name);
              requested_pll_resource |= (1 << requested_pll);
            }
            if (requested_pll_resource == 3) {
              err_msg =
                  clkbuf_routing_failure_msg(name, src_location, dest_instance,
                                             dest_module, dest_location);
              err_msg = CFG_print(
                  "%s. Reason: A single PLL instance cannot route from both "
                  "PLL #0 and PLL#1. You need to explicitely instantiate two "
                  "PLLs",
                  err_msg.c_str());
            } else {
              result[port].push_back(m_resource->m_msg);
              POST_DEBUG_MSG(3, m_resource->m_msg.c_str());
            }
          } else {
            err_msg = clkbuf_routing_failure_msg(
                name, src_location, dest_instance, dest_module, dest_location);
            err_msg = CFG_print("%s. Reason: %s", err_msg.c_str(),
                                m_resource->m_msg.c_str());
          }
        }
      } else {
        err_msg = pll_routing_failure_msg(name, port, src_location,
                                          dest_instance, "", "");
        err_msg =
            CFG_print("%s. Reason: Module usage is invalid in the first place",
                      err_msg.c_str());
      }
      if (err_msg.size()) {
        instance["__validation__"] = false;
        instance["__validation_msg__"] = "Fail to route the clock";
        result[port].push_back(err_msg);
        POST_WARN_MSG(3, err_msg.c_str());
      }
    }
  } else {
    for (auto& dinstance : dest_instances) {
      std::string dest_instance = (std::string)(dinstance);
      std::string err_msg = pll_routing_failure_msg(name, port, src_location,
                                                    dest_instance, "", "");
      err_msg = CFG_print(
          "%s. Reason: Only PLL output port 'CLK_OUT' can use FCLK resource",
          err_msg.c_str());
      result[port].push_back(err_msg);
      POST_WARN_MSG(2, err_msg.c_str());
    }
    instance["__validation__"] = false;
    instance["__validation_msg__"] = "Fail to route the clock";
  }
}

/*
  Entry function to determine the configuration attributes of CLKBUF
*/
void ModelConfig_IO::set_clkbuf_config_attributes() {
  POST_INFO_MSG(0, "Set CLKBUF configuration attributes");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && instance["module"] == "CLK_BUF") {
      if (instance["parameters"].contains("ROUTE_TO_FABRIC_CLK")) {
        set_clkbuf_config_attribute(instance);
      }
    }
  }
}

/*
  Real function to determine the configuration attributes of CLKBUF
*/
void ModelConfig_IO::set_clkbuf_config_attribute(nlohmann::json& instance) {
  validate_instance(instance);
  CFG_ASSERT(instance["__validation__"]);
  std::string name = instance["name"];
  std::string src_location = get_location(name);
  PIN_INFO src_pin_info(src_location);
  uint32_t root_mux = 0;
  if (src_pin_info.type == "HP") {
    root_mux = 0;
  } else if (src_pin_info.type == "HVL") {
    root_mux = 8;
  } else {
    CFG_ASSERT(src_pin_info.type == "HVR");
    root_mux = 16;
  }
  if (src_pin_info.bank == 1) {
    root_mux += 2;
  }
  root_mux += src_pin_info.rx_io;
  // Set FCLK
  set_fclk_config_attribute(instance);
  // Set default for ROOT_BANK_CLKMUX
  for (auto& ab : std::vector<char>({'A', 'B'})) {
    instance[CFG_print("__CDR_CLK_ROOT_SEL_%c__", ab)] = "__DONT__";
    instance[CFG_print("__CORE_CLK_ROOT_SEL_%c__", ab)] = "__DONT__";
  }
  uint32_t core_clk_in_index = src_pin_info.index - (20 * src_pin_info.rx_io);
  char ab = char('A') + char(src_pin_info.rx_io);
  instance[CFG_print("__CORE_CLK_ROOT_SEL_%c__", ab)] =
      std::to_string(core_clk_in_index);
  instance["__bank__"] = std::to_string(src_pin_info.bank);
  instance["__ROOT_MUX_SEL__"] = std::to_string(root_mux);
}

/*
  Entry function to determine the configuration attributes of PLL
*/
void ModelConfig_IO::set_pll_config_attributes() {
  POST_INFO_MSG(0, "Set PLL remaining configuration attributes");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && instance["module"] == "PLL") {
      set_pll_config_attribute(instance);
    }
  }
}

/*
  Real function to determine the configuration attributes of PLL
*/
void ModelConfig_IO::set_pll_config_attribute(nlohmann::json& instance) {
  set_fclk_config_attribute(instance);
}

void ModelConfig_IO::set_fclk_config_attribute(nlohmann::json& instance) {
  POST_DEBUG_MSG(1, "Set FCLK configuration attributes");
  validate_instance(instance);
  CFG_ASSERT(instance["__validation__"]);
  std::string name = instance["name"];
  std::string src_location = get_location(name);
  PIN_INFO src_pin_info(src_location);
  CFG_ASSERT(instance["module"] == "CLK_BUF" || instance["module"] == "PLL");
  bool is_pll = instance["module"] == "PLL";
  // Set the FCLK MUX
  nlohmann::json config = nlohmann::json::array();
  std::string query_name = src_location;
  if (is_pll) {
    query_name = CFG_print("PLL:%s", src_location.c_str());
  }
  std::vector<const ModelConfig_IO_MODEL*> resources =
      m_resource->get_used_fclk(query_name);
  if (resources.size() == 0) {
    POST_DEBUG_MSG(2, "Skip for %s", query_name.c_str());
  }
  for (auto resource : resources) {
    POST_DEBUG_MSG(2, "%s %s (location:%s) use %s", is_pll ? "PLL" : "CLKBUF",
                   name.c_str(), src_location.c_str(),
                   resource->m_name.c_str());
    char ab = resource->m_name.back();
    std::string location = resource->m_ric_name;
    std::map<std::string, uint32_t> fclk_data = {
        {"cfg_rxclk_phase_sel", (is_pll ? 0 : 1)},
        {"cfg_rx_fclkio_sel", (is_pll ? 0 : src_pin_info.rx_io)},
        {"cfg_vco_clk_sel", (is_pll ? 1 : 0)}};
    for (auto& iter : fclk_data) {
      nlohmann::json attribute = nlohmann::json::object();
      attribute["__location__"] = location;
      attribute[CFG_print("%s_%c_%d", iter.first.c_str(), ab,
                          resource->m_bank)] = std::to_string(iter.second);
      config.push_back(attribute);
    }
  }
  instance["cpp_config_attributes"] = config;
}

/*
  Entry function to determine the PLL resource
*/
void ModelConfig_IO::allocate_pll() {
  POST_INFO_MSG(
      0, "Allocate PLL resource (and set PLLREF configuration attributes)");
  CFG_ASSERT(m_resource->get_pll_availability() ==
             (uint32_t)((1 << m_resource->m_plls.size()) - 1));
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(!instance.contains("__pll_resource__"));
  }
  uint32_t init_undecided_pll_count = 0;
  while ((init_undecided_pll_count = undecided_pll())) {
    allocate_pll(false);
    uint32_t undecided_pll_count = undecided_pll();
    if (init_undecided_pll_count == undecided_pll_count) {
      allocate_pll(true);
    }
  }
}

/*
  Real function to determine the PLL resource
  Unittest: All negative tests included
*/
void ModelConfig_IO::allocate_pll(bool force) {
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["module"] == "PLL" && instance["__validation__"] &&
        !instance.contains("__pll_resource__")) {
      std::string name = instance["name"];
      std::string src_location = get_location(name);
      PIN_INFO src_pin_info(src_location);
      // Check if FCLK decided which PLL to use
      std::vector<const ModelConfig_IO_MODEL*> fclks =
          m_resource->get_used_fclk(CFG_print("PLL:%s", src_location.c_str()));
      std::string fclk_names = "";
      for (auto& fclk : fclks) {
        if (fclk_names.size()) {
          fclk_names =
              CFG_print("%s, %s", fclk_names.c_str(), fclk->m_name.c_str());
        } else {
          fclk_names = fclk->m_name;
        }
      }
      POST_DEBUG_MSG(1, "PLL %s (location:%s) uses FCLK '%s'", name.c_str(),
                     src_location.c_str(), fclk_names.c_str());
      uint32_t requested_pll_resource = 0;
      for (auto& fclk : fclks) {
        uint32_t request_bank = m_resource->fclk_use_pll(fclk->m_name);
        requested_pll_resource |= (1 << request_bank);
      }
      uint32_t pin_resource =
          src_pin_info.type == "HVL" ? 1 : (src_pin_info.type == "HVR" ? 2 : 3);
      uint32_t pll_availability = m_resource->get_pll_availability();
      POST_DEBUG_MSG(2,
                     "Pin resource: %d, PLL FCLK requested resource: %d, PLL "
                     "availability: %d",
                     pin_resource, requested_pll_resource, pll_availability);
      std::string msg = "";
      if (requested_pll_resource == 0) {
        msg =
            "PLL request resource is 0 - does not need to route PLL output to "
            "FCLK. Only need to configure PLLREF configuration attributes";
        POST_WARN_MSG(3, msg.c_str());
      }
      uint32_t final_resource = pin_resource & pll_availability;
      uint32_t one_count = 0;
      uint32_t pll_index = 0;
      if (requested_pll_resource) {
        final_resource &= requested_pll_resource;
      }
      for (size_t i = 0; i < m_resource->m_plls.size(); i++) {
        if (final_resource & (uint32_t)(1 << i)) {
          one_count++;
          pll_index = i;
          if (force) {
            POST_DEBUG_MSG(3, "Force to use first found resource");
            force = false;
            break;
          }
        }
      }
      if (one_count == 0) {
        msg = CFG_print("Cannot fit in any Pin/FCLK/PLL resource");
        instance["__validation__"] = false;
        instance["__validation_msg__"] = msg;
        POST_WARN_MSG(3, msg.c_str());
      } else if (one_count == 1) {
        instance["__pll_resource__"] = std::to_string(pll_index);
        std::string pll_resource_name = CFG_print("pll_%d", pll_index);
        CFG_ASSERT(m_resource->use_pll(src_location, pll_resource_name));
        POST_DEBUG_MSG(3, m_resource->m_msg.c_str());
        POST_DEBUG_MSG(4, "Set PLLREF configuration attributes");
        uint32_t rx_io = src_pin_info.rx_io == 0 ? 0 : 3;
        uint32_t divide_by_2 = 0;
        if (instance["parameters"].contains("DIVIDE_CLK_IN_BY_2")) {
          std::string temp = instance["parameters"]["DIVIDE_CLK_IN_BY_2"];
          if (CFG_find_string_in_vector({"TRUE", "ON", "1"}, temp) >= 0) {
            divide_by_2 = 1;
          }
        }
        if (src_pin_info.type == "BOOT_CLOCK") {
          instance["__cfg_pllref_hv_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hv_bank_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hp_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hp_bank_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_use_hv__"] = "__DONT__";
          instance["__cfg_pllref_use_rosc__"] = std::to_string(1);
          instance["__cfg_pllref_use_div__"] = std::to_string(divide_by_2);
        } else if (src_pin_info.type == "HP") {
          instance["__cfg_pllref_hv_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hv_bank_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hp_rx_io_sel__"] = std::to_string(rx_io);
          instance["__cfg_pllref_hp_bank_rx_io_sel__"] =
              std::to_string(src_pin_info.bank);
          instance["__cfg_pllref_use_hv__"] = std::to_string(0);
          instance["__cfg_pllref_use_rosc__"] = std::to_string(0);
          instance["__cfg_pllref_use_div__"] = std::to_string(divide_by_2);
        } else {
          instance["__cfg_pllref_hv_rx_io_sel__"] = std::to_string(rx_io & 1);
          instance["__cfg_pllref_hv_bank_rx_io_sel__"] =
              std::to_string(src_pin_info.bank);
          instance["__cfg_pllref_hp_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_hp_bank_rx_io_sel__"] = "__DONT__";
          instance["__cfg_pllref_use_hv__"] = std::to_string(1);
          instance["__cfg_pllref_use_rosc__"] = std::to_string(0);
          instance["__cfg_pllref_use_div__"] = std::to_string(divide_by_2);
        }
      } else {
        msg =
            CFG_print("It is flexible to use more than one PLL. Decide later");
        POST_DEBUG_MSG(3, msg.c_str());
      }
    }
  }
}

/*
  Determine the FCLK resource ultilization by PLL
*/
uint32_t ModelConfig_IO::undecided_pll() {
  uint32_t count = 0;
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    if (instance["module"] == "PLL" && instance["__validation__"] &&
        !instance.contains("__pll_resource__")) {
      count++;
    }
  }
  return count;
}

/**********************************
 *
 * Fucntions to set configure attributes
 *
 **********************************/
/*
  Entry function to set configuration attributes
*/
void ModelConfig_IO::set_config_attributes(CFG_Python_MGR& python) {
  POST_INFO_MSG(0, "Set configuration attributes");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    std::string module = std::string(instance["module"]);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    if (!instance["__validation__"]) {
      CFG_ASSERT(!instance.contains("config_attributes"));
      for (auto& object_iter : instance["linked_objects"].items()) {
        nlohmann::json& object = object_iter.value();
        object["config_attributes"] = nlohmann::json::array();
      }
      continue;
    }
    std::map<std::string, std::string> instance_args = m_global_args;
    retrieve_instance_args(instance, instance_args);
    POST_DEBUG_MSG(1, "Module: %s (%s)",
                   ((std::string)(instance["module"])).c_str(),
                   ((std::string)(instance["name"])).c_str());
    for (auto& object_iter : instance["linked_objects"].items()) {
      std::string object_name = std::string(object_iter.key());
      POST_DEBUG_MSG(2, "Object: %s", object_name.c_str());
      nlohmann::json& object = object_iter.value();
      nlohmann::json parameters = nlohmann::json::object();
      nlohmann::json properties = nlohmann::json::object();
      nlohmann::json define = nlohmann::json::object();
      object["config_attributes"] = nlohmann::json::array();
      // If there is cpp_config_attributes
      if (instance.contains("cpp_config_attributes")) {
        object["config_attributes"] = instance["cpp_config_attributes"];
      }
      std::string location = std::string(object["location"]);
      if (instance.contains("parameters")) {
        parameters = instance["parameters"];
        CFG_ASSERT(parameters.is_object());
      }
      if (object.contains("properties")) {
        properties = object["properties"];
        CFG_ASSERT(properties.is_object());
      }
      if (m_config_mapping.contains("__define__")) {
        define = m_config_mapping["__define__"];
        CFG_ASSERT(define.is_object());
      }
      std::map<std::string, std::string> args = instance_args;
      parameters["__location__"] = location;
      properties["__location__"] = location;
      args["__location__"] = location;
      POST_DEBUG_MSG(3, "Parameter");
      set_config_attribute(object["config_attributes"], module,
                           (std::string)(instance["pre_primitive"]),
                           instance["post_primitives"], parameters,
                           m_config_mapping["parameters"],
                           instance["connectivity"], args, define, python);
      args = instance_args;
      args["__location__"] = location;
      POST_DEBUG_MSG(3, "Property");
      set_config_attribute(object["config_attributes"], module,
                           (std::string)(instance["pre_primitive"]),
                           instance["post_primitives"], properties,
                           m_config_mapping["properties"],
                           instance["connectivity"], args, define, python);
    }
  }
}

/*
  To set configuration attributes for instance
*/
void ModelConfig_IO::set_config_attribute(
    nlohmann::json& config_attributes, const std::string& module,
    const std::string& pre_primitive, const nlohmann::json& post_primitives,
    nlohmann::json inputs, nlohmann::json mapping, nlohmann::json connectivity,
    std::map<std::string, std::string>& args, nlohmann::json define,
    CFG_Python_MGR& python) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(mapping.is_object());
  for (auto& iter : mapping.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    std::string key = std::string(iter.key());
    std::string module_name = key;
    if (key.find(".") != std::string::npos) {
      std::vector<std::string> module_feature =
          CFG_split_string(key, ".", 0, false);
      CFG_ASSERT(module_feature.size() == 2);
      module_name = module_feature[0];
    }
    if (module_name == module &&
        is_siblings_match(iter.value(), pre_primitive, post_primitives)) {
      POST_DEBUG_MSG(4, "Rule %s", key.c_str());
      nlohmann::json rules = iter.value();
      CFG_ASSERT(rules.is_object());
      bool ready = true;
      if (rules.contains("__ready__")) {
        CFG_ASSERT(rules["__ready__"].is_boolean());
        ready = (bool)(rules["__ready__"]);
      }
      if (ready) {
        CFG_ASSERT(rules.contains("rules"));
        CFG_ASSERT(rules.contains("results"));
        nlohmann::json neg_results = nlohmann::json::object();
        if (rules.contains("neg_results")) {
          neg_results = rules["neg_results"];
        }
        set_config_attribute_by_rules(config_attributes, inputs, connectivity,
                                      rules["rules"], rules["results"],
                                      neg_results, args, define, python);
      }
    }
  }
}

/*
  To set configuration attributes for instance by provided rules
*/
void ModelConfig_IO::set_config_attribute_by_rules(
    nlohmann::json& config_attributes, nlohmann::json inputs,
    nlohmann::json connectivity, nlohmann::json rules, nlohmann::json results,
    nlohmann::json neg_results, std::map<std::string, std::string>& args,
    nlohmann::json define, CFG_Python_MGR& python) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(results.is_object());
  CFG_ASSERT(neg_results.is_object());
  size_t expected_match = rules.size();
  CFG_ASSERT(expected_match);
  size_t match = 0;
  for (auto& rule : rules.items()) {
    nlohmann::json key = rule.key();
    CFG_ASSERT(key.is_string());
    if (config_attribute_rule_match(inputs, connectivity, std::string(key),
                                    rule.value(), args)) {
      match++;
    }
  }
  if (expected_match == match) {
    POST_DEBUG_MSG(5, "Match");
    set_config_attribute_by_rule(config_attributes, results, args, define,
                                 python);
  } else {
    POST_DEBUG_MSG(5, "Mismatch");
    set_config_attribute_by_rule(config_attributes, neg_results, args, define,
                                 python);
  }
}

/*
  To set configuration attributes for instance after evaluate if the rule match
*/
void ModelConfig_IO::set_config_attribute_by_rule(
    nlohmann::json& config_attributes, nlohmann::json& results,
    std::map<std::string, std::string>& args, nlohmann::json define,
    CFG_Python_MGR& python) {
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
        for (auto& sub_result : value.items()) {
          CFG_ASSERT(((nlohmann::json)(sub_result.key())).is_string());
          std::string sub_key = (std::string)(sub_result.key());
        }
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
            POST_DEBUG_MSG(6, "Defined function: %s", definition.c_str());
            define_args(define[definition], args, python);
          }
          object.erase("__define__");
        }
        if (value.contains("__location__")) {
          CFG_ASSERT(object.size() == 0);
          std::string location = std::string(value["__location__"]);
          for (auto arg : args) {
            location =
                CFG_replace_string(location, arg.first, arg.second, false);
          }
          object["__location__"] = location;
          value.erase("__location__");
        } else if (object.contains("__mapped_name__")) {
          CFG_ASSERT(!value.contains("__location__"));
          std::string mapping_name = std::string(object["__mapped_name__"]);
          for (auto arg : args) {
            mapping_name =
                CFG_replace_string(mapping_name, arg.first, arg.second, false);
          }
          object["__mapped_name__"] = mapping_name;
        }
        for (auto& sub_result : value.items()) {
          CFG_ASSERT(((nlohmann::json)(sub_result.key())).is_string());
          std::string sub_key = (std::string)(sub_result.key());
          finalize_config_attribute(config_attributes, args, object, sub_key,
                                    sub_result.value());
        }
      }
    } else {
      finalize_config_attribute(config_attributes, args,
                                nlohmann::json::object({}), key,
                                result.value());
    }
  }
}

void ModelConfig_IO::finalize_config_attribute(
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

/*
  To evaluate if the rule match
*/
bool ModelConfig_IO::config_attribute_rule_match(
    nlohmann::json inputs, nlohmann::json connectivity,
    const std::string& input, nlohmann::json options,
    std::map<std::string, std::string>& args) {
  bool match = false;
  CFG_ASSERT(options.is_string() || options.is_array());
  if (input == "__connectivity__") {
    // This special syntax is to check if the request connection is available
    // in the connectivity object If it is string, then it is an AND operation
    // If it is array of string, then it is an OR operation
    CFG_ASSERT(options.is_string() || options.is_array());
    CFG_ASSERT(connectivity.is_object());
    std::vector<std::string> cons =
        options.is_string()
            ? CFG_split_string(std::string(options), ";", 0, false)
            : get_json_string_list(options, args);
    size_t con_count = 0;
    for (auto& con : cons) {
      if (connectivity.contains(con)) {
        con_count++;
      }
    }
    match = (options.is_string() ? (con_count == cons.size()) : con_count != 0);
  } else if (inputs.contains(input)) {
    // This is to check if the "input" (key) exists in parameters or
    // properties
    if (options.is_array()) {
      // If the key-value is defined as an array, it mean you need to meet
      // multiple choice If one of the choice is met, then it is a match
      for (auto& option : options) {
        CFG_ASSERT(option.is_string());
        if (inputs[input] == std::string(option)) {
          match = true;
          break;
        }
      }
    } else {
      // If the key-value is a string:
      //   - it could be __argX__, which normally we have to grab the value
      //   - simple string, if it match parameters or properties
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
    // It does not exists in parameters or properties at all
    // Special case to define default value with __argX__
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

/**********************************
 *
 * Helper functions
 *
 **********************************/
/*
  Assign new value or overwrite existing value
  Called by property merger and re-location
*/
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
    POST_INFO_MSG(1, "Assign %s%s value:%s to \"%s\"", feature.c_str(),
                  key.c_str(), value.c_str(), name.c_str());
    object[key] = value;
  } else if (existing_value != value) {
    POST_INFO_MSG(1, "Overwrite %s%s value:%s to \"%s\" (value existing: %s)",
                  feature.c_str(), key.c_str(), name.c_str(), value.c_str(),
                  existing_value.c_str());
    object[key] = value;
  }
}

/*
  Base on given module name, figure the first location
*/
std::string ModelConfig_IO::get_location(const std::string& name,
                                         std::string* module) {
  std::string location = "";
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if ((bool)(instance["__validation__"])) {
      if (instance["name"] == name) {
        for (auto iter : instance["linked_objects"].items()) {
          nlohmann::json& port = iter.value();
          location = (std::string)(port["location"]);
          if (location.find("__SKIP_LOCATION_CHECK__") == 0) {
            location = location.substr(23);
            if (location.find(":") == 0) {
              location = location.substr(1);
            }
          }
          if (module != nullptr) {
            (*module) = (std::string)(instance["module"]);
          }
          break;
        }
        break;
      }
    }
  }
  return location;
}

/*
  Set the validation message
*/
void ModelConfig_IO::set_validation_msg(bool status, std::string& msg,
                                        const std::string& module,
                                        const std::string& name,
                                        const std::string& location,
                                        const std::string& seq_name) {
  if (status) {
    if (msg.size()) {
      msg = CFG_print("%s,%s", msg.c_str(), seq_name.c_str());
    } else {
      msg = CFG_print("Pass:%s", seq_name.c_str());
    }
  } else {
    if (msg.size()) {
      msg = CFG_print(";Fail:%s", msg.c_str(), seq_name.c_str());
    } else {
      msg = CFG_print("Fail:%s", seq_name.c_str());
    }
    CFG_POST_WARNING(
        "Skip module:%s name:%s location(s):\"%s\" because it failed in %s "
        "validation",
        module.c_str(), name.c_str(), location.c_str(), seq_name.c_str());
  }
}

/*
  Convert JSON array of string to std::vector of string
*/
std::vector<std::string> ModelConfig_IO::get_json_string_list(
    const nlohmann::json& strings, std::map<std::string, std::string>& args) {
  CFG_ASSERT(strings.is_array());
  std::vector<std::string> string_list;
  for (auto s : strings) {
    CFG_ASSERT(s.is_string());
    std::string str = std::string(s);
    for (auto arg : args) {
      str = CFG_replace_string(str, arg.first, arg.second, false);
    }
    string_list.push_back(str);
  }
  return string_list;
}

/*
  Retrieve args from the instance
*/
void ModelConfig_IO::retrieve_instance_args(
    nlohmann::json& instance, std::map<std::string, std::string>& args) {
  validate_instance(instance);
  for (auto& iter : instance.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    std::string key = (std::string)(iter.key());
    if (key.size() > 4 && key.find("__") == 0 &&
        key.rfind("__") == (key.size() - 2)) {
      if (key != "__validation__" && key != "__validation_msg__") {
        CFG_ASSERT(((nlohmann::json)(iter.value())).is_string());
        args[key] = (std::string)(iter.value());
      }
    }
  }
}

/*
  Run mapping JSON equation and retrieve args from the result
*/
void ModelConfig_IO::define_args(nlohmann::json define,
                                 std::map<std::string, std::string>& args,
                                 CFG_Python_MGR& python) {
  CFG_ASSERT(define.is_object());
  CFG_ASSERT(define.contains("__args__"));
  CFG_ASSERT(define.contains("__equation__"));
  nlohmann::json __args__ = define["__args__"];
  nlohmann::json __equation__ = define["__equation__"];
  CFG_ASSERT(__args__.is_array());
  CFG_ASSERT(__equation__.is_array());
  // Undefine all the argument
  std::vector<std::string> arguments;
  for (nlohmann::json arg : __args__) {
    CFG_ASSERT(arg.is_string());
    if (args.find(std::string(arg)) != args.end()) {
      args.erase(std::string(arg));
    }
    arguments.push_back(std::string(arg));
  }
  // Prepare commands
  std::vector<std::string> commands = get_json_string_list(__equation__, args);
  python.run(commands, arguments);
  CFG_ASSERT_MSG(python.results().size() == arguments.size(),
                 "Expect there is %ld Python result, but found %ld",
                 arguments.size(), python.results().size());
  for (auto arg : arguments) {
    args[arg] = python.result_str(arg);
  }
}

/*
  Parsing special __arg????__ string
*/
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

/*
  Post message
*/
void ModelConfig_IO::post_msg(MCIO_MSG_TYPE type, uint32_t space,
                              const std::string& msg) {
  std::string pre = "";
  if (type == MCIO_MSG_TYPE::IS_WARNING) {
    pre = "Warning: ";
  } else if (type == MCIO_MSG_TYPE::IS_ERROR) {
    pre = "Error: ";
  }
  m_messages.push_back(new ModelConfig_IO_MSG(space, pre + msg));
  if (type == MCIO_MSG_TYPE::IS_INFO) {
    CFG_POST_MSG(msg.c_str());
  } else if (type == MCIO_MSG_TYPE::IS_WARNING) {
    CFG_POST_WARNING(msg.c_str());
  } else if (type == MCIO_MSG_TYPE::IS_ERROR) {
    CFG_POST_ERR(msg.c_str());
  }
}

/*
  Generate standard failure message for not able to route CLKBUF to gearbox
*/
std::string ModelConfig_IO::clkbuf_routing_failure_msg(
    const std::string& clkbuf, const std::string& clkbuf_location,
    const std::string& gearbox, const std::string& gearbox_module,
    const std::string& gearbox_location) {
  std::string msg = "";
  if (gearbox_module.size()) {
    CFG_ASSERT(gearbox_location.size());
    msg = CFG_print(
        "Not able to route clock-capable pin %s (location:%s) to gearbox "
        "module %s clock (module:%s) (location:%s)",
        clkbuf.c_str(), clkbuf_location.c_str(), gearbox.c_str(),
        gearbox_module.c_str(), gearbox_location.c_str());
  } else {
    CFG_ASSERT(gearbox_location.size() == 0);
    msg = CFG_print(
        "Not able to route clock-capable pin %s (location:%s) to gearbox "
        "module %s clock",
        clkbuf.c_str(), clkbuf_location.c_str(), gearbox.c_str());
  }
  return msg;
}

/*
  Generate standard failure message for not able to route PLL to gearbox
*/
std::string ModelConfig_IO::pll_routing_failure_msg(
    const std::string& pll, const std::string& pll_port,
    const std::string& pll_location, const std::string& gearbox,
    const std::string& gearbox_module, const std::string& gearbox_location) {
  std::string msg = "";
  if (gearbox_module.size()) {
    CFG_ASSERT(gearbox_location.size());
    msg = CFG_print(
        "Not able to route PLL %s Port %s (location:%s) to gearbox module %s "
        "clock (module:%s) (location:%s)",
        pll.c_str(), pll_port.c_str(), pll_location.c_str(), gearbox.c_str(),
        gearbox_module.c_str(), gearbox_location.c_str());
  } else {
    CFG_ASSERT(gearbox_location.size() == 0);
    msg = CFG_print(
        "Not able to route PLL %s Port %s (location:%s) to gearbox module %s "
        "clock",
        pll.c_str(), pll_port.c_str(), pll_location.c_str(), gearbox.c_str());
  }
  return msg;
}

/**********************************
 *
 * Helper functions: checking sibling rules
 *
 **********************************/
/*
  Entry function to check sibling rules
*/
bool ModelConfig_IO::is_siblings_match(nlohmann::json& rules,
                                       const std::string& pre_primitive,
                                       const nlohmann::json& post_primitives) {
  CFG_ASSERT(rules.is_object());
  bool status = true;
  if (rules.contains("pre_primitive")) {
    status = is_siblings_match(rules["pre_primitive"], pre_primitive, true);
  }
  if (status && rules.contains("not_pre_primitive")) {
    status =
        is_siblings_match(rules["not_pre_primitive"], pre_primitive, false);
  }
  if (status && rules.contains("post_primitive")) {
    status = is_siblings_match(rules["post_primitive"], post_primitives, true);
  }
  if (status && rules.contains("not_post_primitive")) {
    status =
        is_siblings_match(rules["not_post_primitive"], post_primitives, false);
  }
  return status;
}

/*
  Real function to check pre-sibling rules
*/
bool ModelConfig_IO::is_siblings_match(nlohmann::json& primitive,
                                       const std::string& primitive_name,
                                       bool match) {
  bool status = true;
  CFG_ASSERT(primitive.is_string() || primitive.is_array());
  std::map<std::string, std::string> args;
  std::vector<std::string> primitives =
      primitive.is_string()
          ? CFG_split_string(std::string(primitive), ";", 0, false)
          : get_json_string_list(primitive, args);
  size_t match_count = 0;
  for (auto p : primitives) {
    if (match) {
      if (p == primitive_name) {
        match_count++;
      }
    } else {
      if (p != primitive_name) {
        match_count++;
      }
    }
  }
  status = (primitive.is_string() ? (match_count == primitives.size())
                                  : match_count != 0);
  return status;
}

/*
  Real function to check post-sibling rules
*/
bool ModelConfig_IO::is_siblings_match(nlohmann::json& primitive,
                                       const nlohmann::json& postprimitives,
                                       bool match) {
  bool status = true;
  CFG_ASSERT(primitive.is_string() || primitive.is_array());
  CFG_ASSERT(postprimitives.is_array());
  std::map<std::string, std::string> args;
  std::vector<std::string> primitives =
      primitive.is_string()
          ? CFG_split_string(std::string(primitive), ";", 0, false)
          : get_json_string_list(primitive, args);
  std::vector<std::string> post_primitives;
  if (postprimitives.size()) {
    post_primitives = get_json_string_list(postprimitives, args);
  } else {
    post_primitives.push_back("");
  }
  size_t match_count = 0;
  for (auto post : post_primitives) {
    size_t temp_match = 0;
    for (auto p : primitives) {
      if (match) {
        if (p == post) {
          temp_match++;
        }
      } else {
        if (p != post) {
          temp_match++;
        }
      }
    }
    if ((match && temp_match != 0) ||
        (!match && temp_match == primitives.size())) {
      match_count++;
    }
  }
  status = (primitive.is_string() ? (match_count == post_primitives.size())
                                  : match_count != 0);
  return status;
}

/**********************************
 *
 * Helper functions: write JSON
 *
 **********************************/
/*
  Entry function to write instances (message) into JSON
*/
void ModelConfig_IO::write_json(const std::string& file) {
  std::ofstream json(file.c_str());
  size_t index = 0;
  json << "{\n  \"messages\" : [\n";
  for (auto& msg : m_messages) {
    if (index) {
      json << ",\n";
    }
    json << "    \"";
    for (uint32_t i = 0; i < msg->offset; i++) {
      json << "  ";
    }
    write_json_data(msg->msg, json);
    json << "\"";
    json.flush();
    index++;
  }
  if (index) {
    json << "\n";
  }
  json << "  ],\n";
  json << "  \"instances\" : [\n";
  index = 0;
  for (auto& instance : m_instances) {
    if (index) {
      json << ",\n";
    }
    write_json_instance(instance, json);
    json.flush();
    index++;
  }
  if (index) {
    json << "\n";
  }
  json << "  ]\n}\n";
  json.close();
}

/*
  To write single instance into JSON
*/
void ModelConfig_IO::write_json_instance(nlohmann::json& instance,
                                         std::ofstream& json) {
  validate_instance(instance, true);
  std::map<std::string, std::string> args;
  json << "    {\n";
  write_json_object("module", instance["module"], json);
  json << ",\n";
  write_json_object("name", instance["name"], json);
  json << ",\n";
  write_json_object("linked_object", instance["linked_object"], json);
  json << ",\n";
  std::vector<std::string> obj_seq =
      CFG_split_string(std::string(instance["linked_object"]), "+", 0, false);
  nlohmann::json& objects = instance["linked_objects"];
  CFG_ASSERT(obj_seq.size() == objects.size());
  json << "      \"linked_objects\" : {\n";
  size_t index = 0;
  for (auto& obj : obj_seq) {
    if (index) {
      json << ",\n";
    }
    CFG_ASSERT(objects.contains(obj));
    nlohmann::json object = objects[obj];
    json << "        \"" << obj.c_str() << "\" : {\n";
    write_json_object("location", std::string(object["location"]), json, 5);
    json << ",\n";
    json << "          \"properties\" : {\n";
    write_json_map(object["properties"], json, 6);
    json << "          },\n";
    json << "          \"config_attributes\" : [\n";
    size_t attr_index = 0;
    for (auto& cfg_attr : object["config_attributes"]) {
      CFG_ASSERT(cfg_attr.is_object());
      if (attr_index) {
        json << ",\n";
      }
      json << "            {\n";
      write_json_map(cfg_attr, json, 7);
      json << "            }";
      attr_index++;
    }
    if (attr_index) {
      json << "\n";
    }
    json << "          ]\n";
    json << "        }";
    index++;
  }
  json << "\n";
  json << "      },\n";
  json << "      \"connectivity\" : {\n";
  write_json_map(instance["connectivity"], json);
  json << "      },\n";
  json << "      \"parameters\" : {\n";
  write_json_map(instance["parameters"], json);
  json << "      },\n";
  write_json_object("pre_primitive", (std::string)(instance["pre_primitive"]),
                    json);
  json << ",\n";
  json << "      \"post_primitives\" : [\n",
      write_json_array(get_json_string_list(instance["post_primitives"], args),
                       json);
  json << "      ],\n";
  for (auto key :
       std::vector<std::string>({"route_clock_to", "route_clock_result"})) {
    index = 0;
    json << "      \"" << key.c_str() << "\" : {\n";
    for (auto iter : instance[key].items()) {
      if (index) {
        json << ",\n";
      }
      json << "        \"" << std::string(iter.key()).c_str() << "\" : [\n";
      write_json_array(get_json_string_list(iter.value(), args), json, 5);
      json << "        ]";
      index++;
    }
    if (index) {
      json << "\n";
    }
    json << "      },\n";
  }
  for (auto& iter : instance.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    std::string key = (std::string)(iter.key());
    if (key.size() > 4 && key.find("__") == 0 &&
        key.rfind("__") == (key.size() - 2)) {
      if (key != "__validation__" && key != "__validation_msg__") {
        CFG_ASSERT(((nlohmann::json)(iter.value())).is_string());
        write_json_object(key, iter.value(), json);
        json << ",\n";
      }
    }
  }
  write_json_object("__validation__",
                    (bool)(instance["__validation__"]) ? "TRUE" : "FALSE",
                    json);
  json << ",\n";
  write_json_object("__validation_msg__",
                    std::string(instance["__validation_msg__"]), json);
  json << "\n    }";
}

/*
  To write object into JSON
    "string" : "string"
*/
void ModelConfig_IO::write_json_object(const std::string& key,
                                       const std::string& value,
                                       std::ofstream& json, uint32_t space) {
  while (space) {
    json << "  ";
    space--;
  }
  json << "\"";
  write_json_data(key, json);
  json << "\"";
  json << " : ";
  json << "\"";
  write_json_data(value, json);
  json << "\"";
}

/*
  To write object into JSON
    "string" : "string",
    "string" : "string",
    ....
*/
void ModelConfig_IO::write_json_map(nlohmann::json& map, std::ofstream& json,
                                    uint32_t space) {
  size_t index = 0;
  for (auto& iter : map.items()) {
    if (index) {
      json << ",\n";
    }
    write_json_object(std::string(iter.key()), std::string(iter.value()), json,
                      space);
    index++;
  }
  if (index) {
    json << "\n";
  }
}

/*
  To write array into JSON
    "string",
    "string",
    ....
*/
void ModelConfig_IO::write_json_array(std::vector<std::string> array,
                                      std::ofstream& json, uint32_t space) {
  size_t index = 0;
  for (auto& iter : array) {
    if (index) {
      json << ",\n";
    }
    for (uint8_t i = 0; i < space; i++) {
      json << "  ";
    }
    json << "\"" << iter.c_str() << "\"";
    index++;
  }
  if (index) {
    json << "\n";
  }
}

/*
  To write string by taking care '"'
*/
void ModelConfig_IO::write_json_data(const std::string& str,
                                     std::ofstream& json) {
  for (auto& c : str) {
    if (c == '\\') {
      json << '\\';
    } else if (c == '"') {
      json << '\\';
    }
    json << c;
  }
}

/**********************************
 *
 * Static function
 *
 **********************************/
/*
  Entry function to allocate resource
*/
bool ModelConfig_IO::allocate_resource(
    std::vector<MODEL_RESOURCE_INSTANCE*>& instances,
    MODEL_RESOURCE_INSTANCE*& new_instance, bool print_msg) {
  bool status = false;
  // Sanity check, all must have been decided except last one
  // All total must be same
  uint32_t total = new_instance->total;
  for (size_t i = 0; i < (instances.size() + 1); i++) {
    MODEL_RESOURCE_INSTANCE* inst =
        i < instances.size() ? instances[i] : new_instance;
    CFG_ASSERT(total == inst->total);
  }
  for (auto& inst : instances) {
    inst->backup();
  }
  uint32_t allocated_resource_track = 0;
  uint32_t decided_instance_track = 0;
  std::vector<MODEL_RESOURCE_INSTANCE*> new_instances;
  bool stop = false;
  while (!stop) {
    // Very time figure out which instances has least chance (least fortunate)
    uint32_t least_possible = (uint32_t)(-1);
    size_t least_fortunate_instance = (size_t)(-1);
    bool use_shift_method = false;
    for (size_t i = 0; i < (instances.size() + 1); i++) {
      // Only search for instance which has not been decided
      if ((decided_instance_track & (1 << i)) == 0) {
        MODEL_RESOURCE_INSTANCE*& inst =
            i < instances.size() ? instances[i] : new_instance;
        uint32_t possible_count = 0;
        for (uint32_t j = 0; j < inst->total; j++) {
          if (((inst->possible & (1 << j)) != 0) &&
              ((allocated_resource_track & (1 << j)) == 0)) {
            possible_count++;
          }
        }
        if (possible_count == 0) {
          // We cannot find any resource that this instance can useful
          // We try if we can shift around other instances
          for (uint32_t j = 0; j < inst->total; j++) {
            if (inst->possible & (1 << j)) {
              if (shift_instance_resource(j, allocated_resource_track,
                                          new_instances, print_msg)) {
                // shift_instance_resource had updated the decisions of
                // existing instances and allocated_resource_track
                inst->decision = j;
                new_instances.push_back(inst);
                decided_instance_track |= (1 << i);
                use_shift_method = true;
                if (print_msg) {
                  printf(
                      "    Decided instance %d to use resource %d (after "
                      "other "
                      "shift)\n",
                      inst->index, inst->decision);
                }
                break;
              }
            }
          }
          stop = !use_shift_method;
          if (print_msg && stop) {
            printf("  Cannot find resource for instance %d\n", inst->index);
          }
          break;
        } else {
          if (possible_count < least_possible) {
            least_possible = possible_count;
            least_fortunate_instance = i;
          }
        }
      }
    }
    if (!stop && !use_shift_method) {
      CFG_ASSERT(least_fortunate_instance <= instances.size());
      MODEL_RESOURCE_INSTANCE*& inst =
          least_fortunate_instance < instances.size()
              ? instances[least_fortunate_instance]
              : new_instance;
      for (uint32_t j = 0; j < inst->total; j++) {
        if (((inst->possible & (1 << j)) != 0) &&
            ((allocated_resource_track & (1 << j)) == 0)) {
          allocated_resource_track |= (1 << j);
          decided_instance_track |= (1 << least_fortunate_instance);
          inst->decision = j;
          new_instances.push_back(inst);
          if (print_msg) {
            printf("  Decided instance %d to use resource %d\n", inst->index,
                   inst->decision);
          }
          break;
        }
      }
    }
    if (!stop) {
      if ((instances.size() + 1) == new_instances.size()) {
        stop = true;
        status = true;
        instances.push_back(new_instance);
      }
    }
  }
  if (!status) {
    delete new_instance;
    new_instance = nullptr;
    for (auto& inst : instances) {
      inst->restore();
    }
  }
  return status;
}

/*
  Move the resource if it is flexible and give opportunity to those that less
  flexible
*/
bool ModelConfig_IO::shift_instance_resource(
    uint32_t try_resource, uint32_t& allocated_resource_track,
    std::vector<MODEL_RESOURCE_INSTANCE*>& instances, bool print_msg) {
  bool shifted = false;
  for (auto& inst : instances) {
    if (inst->decision == try_resource) {
      for (uint32_t j = 0; j < inst->total; j++) {
        if (j != try_resource && ((inst->possible & (1 << j)) != 0) &&
            ((allocated_resource_track & (1 << j)) == 0)) {
          if (print_msg) {
            printf("  Shift instance %d from resource %d to resource %d\n",
                   inst->index, inst->decision, j);
          }
          inst->decision = j;  // new decision made
          allocated_resource_track |= (1 << j);
          shifted = true;
          break;
        }
      }
      if (shifted) {
        break;
      }
    }
  }
  return shifted;
}

}  // namespace FOEDAG
