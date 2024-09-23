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
const std::string PLLREF_RE = "(*s*)pll_refmux[(*d*)]";
const std::string PLL_RE = "(*s*)pll[(*d*)]";

struct ModelConfig_IO_MSG {
  ModelConfig_IO_MSG(uint32_t o, const std::string& m) : offset(o), msg(m) {
#if ENABLE_DEBUG_MSG
    printf("DEBUG: ");
    for (uint32_t i = 0; i < offset; i++) {
      printf("  ");
    }
    printf("%s\n", msg.c_str());
    fflush(stdout);
#if 0
    std::ofstream outfile;
    outfile.open("debug.txt", std::ios_base::app);
    for (uint32_t i = 0; i < offset; i++) {
      outfile << "  ";
    }
    outfile << msg.c_str() << "\n"
    outfile.close();
#endif
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
    const std::vector<std::string>& flag_options,
    const std::map<std::string, std::string>& options,
    const std::string& output) {
  std::string netlist_ppdb = options.at("netlist_ppdb");
  std::string config_mapping = options.at("config_mapping");
  std::string routing_config = options.find("routing_config") != options.end()
                                   ? options.at("routing_config")
                                   : "";
  std::string routing_config_model =
      options.find("routing_config_model") != options.end()
          ? options.at("routing_config_model")
          : "";
  std::string property_json = options.find("property_json") != options.end()
                                  ? options.at("property_json")
                                  : "";
  bool is_unittest = std::find(flag_options.begin(), flag_options.end(),
                               "is_unittest") != flag_options.end();
  if (options.find("pll_workaround") != options.end()) {
    m_pll_workaround = options.at("pll_workaround");
  }
  if (!is_unittest) {
    POST_INFO_MSG(0, "Netlist PPDB: %s", netlist_ppdb.c_str());
    POST_INFO_MSG(0, "Config Mapping: %s", config_mapping.c_str());
    POST_INFO_MSG(0, "Routing Configurator: %s", routing_config.c_str());
    POST_INFO_MSG(0, "Routing Config Model: %s", routing_config_model.c_str());
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
  CFG_ASSERT(m_config_mapping.contains("__init_file__"));
  // Read the property JSON if it exists
  nlohmann::json property_instances = nlohmann::json::object();
  if (property_json.size()) {
    input.open(property_json.c_str());
    CFG_ASSERT(input.is_open() && input.good());
    property_instances = nlohmann::json::parse(input);
    input.close();
  }
  m_python = new CFG_Python_MGR;
  // Prepare python file
  python_file(is_unittest, routing_config);
  // Validate instances
  validate_instances(netlist_instances);
  // Merge the property
  merge_property_instances(property_instances);
  // Locate the instances
  locate_instances();
  // Prepare for validation for location
  if (g_enable_python) {
    // initialization();
  } else {
    POST_WARN_MSG(0, "Skip pin assignment legality check");
  }
  // Validate the location
  validations(true, "__primary_validation__");
  // Validate internal errors
  internal_error_validations();
  // Invalid children if parent is invalid
  invalidate_childs();
  // Assign location to instance that naturally does not location
  assign_no_location_instance();
  // Prepare object that needed
  prepare_instance_objects();
  // Routing related
  if (!routing_config.empty() && !routing_config_model.empty() &&
      !m_routing_config.empty()) {
    // Prepare routing issue that need to be solved
    nlohmann::json routings = prepare_routing_json();
    // Solve the routing issue
    solve_routing_json(routings, routing_config_model);
  }
  // Finalize the attribute for configuration
  set_config_attributes();
  // Output
  write_json(output);
}

/*
  Destructor
*/
ModelConfig_IO::~ModelConfig_IO() {
  if (m_python != nullptr) {
    delete m_python;
    m_python = nullptr;
  }
  while (m_messages.size()) {
    delete m_messages.back();
    m_messages.pop_back();
  }
}

/*
  Call config mapping initialization if it was defined
*/
void ModelConfig_IO::python_file(bool is_unittest,
                                 const std::string& routing_config) {
  CFG_ASSERT(m_config_mapping.contains("__init_file__"));
  CFG_ASSERT(m_config_mapping["__init_file__"].is_object());
  CFG_ASSERT(m_config_mapping["__init_file__"].contains("__args__"));
  CFG_ASSERT(m_config_mapping["__init_file__"]["__args__"].is_array());
  CFG_ASSERT(m_config_mapping["__init_file__"].contains("__file__"));
  CFG_ASSERT(m_config_mapping["__init_file__"]["__file__"].is_array());
  CFG_ASSERT(m_python != nullptr);
  std::filesystem::path fullpath = std::filesystem::absolute("config.py");
  std::string filename = fullpath.filename().string();
  std::string standard_fullpath =
      CFG_change_directory_to_linux_format(fullpath.string());
  if (is_unittest) {
    POST_DEBUG_MSG(0, "Preparing Python file: %s", filename.c_str());
  } else {
    POST_DEBUG_MSG(0, "Preparing Python file: %s", standard_fullpath.c_str());
  }
  std::vector<std::string> arguments;
  for (auto& str : m_config_mapping["__init_file__"]["__args__"]) {
    CFG_ASSERT(str.is_string());
    arguments.push_back(str);
  }
  std::ofstream output(standard_fullpath.c_str());
  for (auto& str : m_config_mapping["__init_file__"]["__file__"]) {
    CFG_ASSERT(str.is_string());
    output << ((std::string)(str)).c_str() << "\n";
  }
  output.close();
  CFG_ASSERT(m_python->set_file(standard_fullpath, true, arguments) ==
             "config");
  CFG_ASSERT_MSG(
      m_python->results().size() == arguments.size(),
      "Expected python.set_file() results size() == %ld, but found %ld",
      arguments.size(), m_python->results().size());
  for (auto arg : arguments) {
    m_global_args[arg] = m_python->result_str(arg);
  }
  if (routing_config.size()) {
    CFG_ASSERT(routing_config.size() > 3 &&
               (routing_config.rfind(".py") == (routing_config.size() - 3)));
    std::filesystem::path rfullpath = std::filesystem::absolute(routing_config);
    std::string standard_rfullpath =
        CFG_change_directory_to_linux_format(rfullpath.string());
    m_routing_config = rfullpath.filename().string();
    m_routing_config = m_routing_config.substr(0, m_routing_config.size() - 3);
    CFG_ASSERT(m_routing_config.size());
    CFG_ASSERT(m_python->set_file(standard_rfullpath, false) ==
               m_routing_config);
  } else {
    m_routing_config = "";
  }
}

/*
  Entry function to validate JSON format of instances
  Store the instances as m_instances
*/
void ModelConfig_IO::validate_instances(nlohmann::json& instances) {
  POST_INFO_MSG(0, "Validate netlist instances");
  CFG_ASSERT(instances.is_object());
  CFG_ASSERT(instances.contains("status"));
  CFG_ASSERT(instances["status"].is_boolean());
  CFG_ASSERT(instances.contains("instances"));
  CFG_ASSERT(instances["instances"].is_array());
  CFG_ASSERT(m_instances.is_null());
  m_status = bool(instances["status"]);
  m_instances = instances["instances"];
  for (auto& instance : m_instances) {
    validate_instance(instance);
  }
}

/*
  Real function to validate JSON format of instance
*/
void ModelConfig_IO::validate_instance(const nlohmann::json& instance,
                                       bool is_final) {
  CFG_ASSERT(instance.is_object());
  // Check existence
  CFG_ASSERT(instance.contains("module"));
  CFG_ASSERT(instance.contains("name"));
  CFG_ASSERT(instance.contains("location_object"));
  CFG_ASSERT(instance.contains("location"));
  CFG_ASSERT(instance.contains("linked_object"));
  CFG_ASSERT(instance.contains("linked_objects"));
  CFG_ASSERT(instance.contains("connectivity"));
  CFG_ASSERT(instance.contains("parameters"));
  CFG_ASSERT(instance.contains("flags"));
  CFG_ASSERT(instance.contains("pre_primitive"));
  CFG_ASSERT(instance.contains("post_primitives"));
  CFG_ASSERT(instance.contains("route_clock_to"));
  CFG_ASSERT(instance.contains("errors"));
  if (is_final) {
    CFG_ASSERT(instance.contains("route_clock_result"));
    CFG_ASSERT(instance.contains("config_attributes"));
  }
  // Check type
  CFG_ASSERT(instance["module"].is_string());
  CFG_ASSERT(instance["module"].size());
  CFG_ASSERT(instance["name"].is_string());
  CFG_ASSERT(instance["name"].size());
  CFG_ASSERT(instance["location_object"].is_string());
  CFG_ASSERT(instance["location"].is_string());
  CFG_ASSERT(instance["linked_object"].is_string());
  CFG_ASSERT(instance["linked_object"].size());
  CFG_ASSERT(instance["linked_objects"].is_object());
  CFG_ASSERT(instance["connectivity"].is_object());
  CFG_ASSERT(instance["parameters"].is_object());
  CFG_ASSERT(instance["flags"].is_array());
  CFG_ASSERT(instance["pre_primitive"].is_string());
  CFG_ASSERT(instance["post_primitives"].is_array());
  CFG_ASSERT(instance["route_clock_to"].is_object());
  CFG_ASSERT(instance["errors"].is_array());
  if (is_final) {
    CFG_ASSERT(instance["route_clock_result"].is_object());
    CFG_ASSERT(instance["config_attributes"].is_array());
    for (auto& cfg_attr : instance["config_attributes"]) {
      CFG_ASSERT(cfg_attr.is_object());
      for (auto& iter2 : cfg_attr.items()) {
        CFG_ASSERT(((nlohmann::json)(iter2.key())).is_string());
        CFG_ASSERT(((nlohmann::json)(iter2.value())).is_string());
      }
    }
  }
  // Check linked object
  CFG_ASSERT(instance["linked_objects"].size());
  for (auto& iter0 : instance["linked_objects"].items()) {
    CFG_ASSERT(((nlohmann::json)(iter0.key())).is_string());
    const nlohmann::json& object = iter0.value();
    CFG_ASSERT(object.is_object());
    // Check existence
    CFG_ASSERT(object.contains("location"));
    CFG_ASSERT(object.contains("properties"));
    if (is_final) {
      CFG_ASSERT(object.contains("config_attributes"));
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
  // Check flags
  for (auto& flag : instance["flags"]) {
    CFG_ASSERT(flag.is_string());
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
  // Check errors
  for (auto& error : instance["errors"]) {
    CFG_ASSERT(error.is_string());
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
  Validate JSON format of routing
*/
void ModelConfig_IO::validate_routing(const nlohmann::json& routing,
                                      bool is_final) {
  CFG_ASSERT(routing.is_object());
  // Check existence
  CFG_ASSERT(routing.contains("feature"));
  CFG_ASSERT(routing.contains("comments"));
  CFG_ASSERT(routing.contains("source"));
  CFG_ASSERT(routing.contains("destinations"));
  CFG_ASSERT(routing.contains("filters"));
  CFG_ASSERT(routing.contains("flags"));
  if (is_final) {
    CFG_ASSERT(routing.contains("msgs"));
    CFG_ASSERT(routing.contains("potential paths"));
    CFG_ASSERT(routing.contains("config mux"));
    CFG_ASSERT(routing.contains("status"));
  }
  // Check type
  CFG_ASSERT(routing["feature"].is_string());
  CFG_ASSERT(routing["comments"].is_array());
  for (auto& r : routing["comments"]) {
    CFG_ASSERT(r.is_string());
  }
  CFG_ASSERT(routing["source"].is_string());
  CFG_ASSERT(routing["destinations"].is_array());
  for (auto& r : routing["destinations"]) {
    CFG_ASSERT(r.is_string());
  }
  CFG_ASSERT(routing["filters"].is_array());
  for (auto& r : routing["filters"]) {
    CFG_ASSERT(r.is_string());
  }
  CFG_ASSERT(routing["flags"].is_array());
  for (auto& r : routing["flags"]) {
    CFG_ASSERT(r.is_string());
  }
  if (is_final) {
    CFG_ASSERT(routing["msgs"].is_array());
    for (auto& r : routing["msgs"]) {
      CFG_ASSERT(r.is_string());
    }
    CFG_ASSERT(routing["potential paths"].is_array());
    for (auto& r : routing["potential paths"]) {
      CFG_ASSERT(r.is_array());
      for (auto& p : r) {
        CFG_ASSERT(p.is_string());
      }
    }
    CFG_ASSERT(routing["config mux"].is_array());
    for (auto& r : routing["config mux"]) {
      if (r.is_object()) {
        CFG_ASSERT(r.size() == 1);
        for (auto& iter0 : r.items()) {
          CFG_ASSERT(((nlohmann::json)(iter0.key())).is_string());
          std::string key0 = (std::string)(iter0.key());
          CFG_ASSERT(key0.size());
          CFG_ASSERT(iter0.value().is_object());
          CFG_ASSERT(iter0.value().size());
          for (auto& iter1 : iter0.value().items()) {
            CFG_ASSERT(((nlohmann::json)(iter1.key())).is_string());
            std::string key1 = (std::string)(iter1.key());
            CFG_ASSERT(key1.size());
            CFG_ASSERT(iter1.value().is_string());
          }
        }
      } else {
        CFG_ASSERT(r.is_null());
      }
    }
    CFG_ASSERT(routing["status"].is_boolean());
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
      assign_json_object(object, "location", location, name,
                         "Instance-Object:");
      if (object_name == instance["location_object"]) {
        assign_json_object(instance, "location", location, name, "Instance:");
      }
    }
  }
}

/*
  Call config mapping initialization if it was defined
*/
void ModelConfig_IO::initialization() {
  POST_INFO_MSG(0, "Configure Mapping file initialization");
  if (m_config_mapping.contains("__init__")) {
    define_args(m_config_mapping["__init__"], m_global_args);
  }
}

/*
  Entry function to perform validation based on mapping
*/
void ModelConfig_IO::validations(bool init, const std::string& key) {
  POST_INFO_MSG(0, "Validate instances using '%s' rule", key.c_str());
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
        validation(instance, key);
      }
    } else {
      instance["__validation__"] = false;
      instance["__validation_msg__"] = "Skip because Python is ignored";
      m_status = false;
    }
  }
}

void ModelConfig_IO::validation(nlohmann::json& instance,
                                const std::string& key) {
  CFG_ASSERT(m_python != nullptr);
  validate_instance(instance);
  std::string module = std::string(instance["module"]);
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
      args["__primitive_flags__"] =
          CFG_join_strings(get_json_string_list(instance["flags"], args), ",");
      args["__location__"] = instance["location"];
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
            bool parameter_validation_optional = false;
            bool parameter_status = true;
            if (validation_info.contains(
                    "__optional_if_parameter_not_defined__")) {
              CFG_ASSERT(
                  validation_info["__optional_if_parameter_not_defined__"]
                      .is_boolean())
              parameter_validation_optional =
                  (bool)(validation_info
                             ["__optional_if_parameter_not_defined__"]);
            }
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
                                 seq_name, parameter_validation_optional);
              if (parameter_validation_optional) {
                continue;
              } else {
                break;
              }
            }
            std::vector<std::string> equations =
                get_json_string_list(__equation__, args);
            m_python->run(equations, {"validation_result"});
            CFG_ASSERT_MSG(
                m_python->results().size() == 1,
                "Expected python.run() results size() == 1, but found %ld",
                m_python->results().size());
            status = m_python->result_bool("validation_result");
            set_validation_msg(status, msg, module, name, locations, seq_name);
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
  Entry function to perform internal error validation
*/
void ModelConfig_IO::internal_error_validations() {
  POST_INFO_MSG(0, "Validate error from netlist");
  for (auto& instance : m_instances) {
    // This is the most basic validation which will be run first
    // Hence it is impossible there is any object key
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"]) {
      std::string msg = (std::string)(instance["__validation_msg__"]);
      if (msg.find("Skipped.") == 0) {
        // Reset the message since "Skipped" message is meaningless
        msg = "";
      }
      if (instance["errors"].size()) {
        // Extractor already detect error
        std::string module = std::string(instance["module"]);
        std::string name = std::string(instance["name"]);
        set_validation_msg(false, msg, module, name, "", "internal_error");
        instance["__validation__"] = false;
        instance["__validation_msg__"] = msg;
      }
    }
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
      invalidate_chain(linked_object);
    }
  }
}

/*
  Real function to invalidate the chain associated with the same linked_object
  (if any of them is invalid)
*/
void ModelConfig_IO::invalidate_chain(const std::string& linked_object) {
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if ((std::string)(instance["linked_object"]) == linked_object &&
        instance["__validation__"]) {
      instance["__validation__"] = false;
      instance["__validation_msg__"] =
          "Invalidated because other instance in the chain is invalid";
      m_status = false;
    }
  }
}

/*
  Assign location to instance that naturally does not location
*/
void ModelConfig_IO::assign_no_location_instance() {
  POST_INFO_MSG(0, "Assign instance-without-location");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && (instance["module"] == "BOOT_CLOCK" ||
                                       instance["module"] == "FCLK_BUF")) {
      POST_DEBUG_MSG(1, "Instance: %s",
                     ((std::string)(instance["name"])).c_str());
      CFG_ASSERT(((std::string)(instance["location"])).size() == 0);
      instance["location"] =
          CFG_print("__SKIP_LOCATION_CHECK__:%s",
                    ((std::string)(instance["location_object"])).c_str());
      for (auto iter : instance["linked_objects"].items()) {
        POST_DEBUG_MSG(2, "Object: %s", ((std::string)(iter.key())).c_str());
        nlohmann::json& object = iter.value();
        CFG_ASSERT(((std::string)(object["location"])).size() == 0);
        object["location"] = CFG_print("__SKIP_LOCATION_CHECK__:%s",
                                       ((std::string)(iter.key())).c_str());
      }
      assign_no_location_instance_child_location(instance["linked_object"]);
    }
  }
}

/*
  Assign location to child from instance that naturally does not location
*/
void ModelConfig_IO::assign_no_location_instance_child_location(
    const std::string& linked_object) {
  POST_DEBUG_MSG(3, "Assign location for child from instance-without-location");
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"] && instance["module"] != "BOOT_CLOCK" &&
        instance["module"] != "FCLK_BUF" &&
        instance["linked_object"] == linked_object) {
      CFG_ASSERT(((std::string)(instance["location"])).size() == 0);
      instance["location"] =
          CFG_print("__SKIP_LOCATION_CHECK__:%s",
                    ((std::string)(instance["location_object"])).c_str());
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
  Prepare empty object for all instance
*/
void ModelConfig_IO::prepare_instance_objects() {
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(!instance.contains("route_clock_result"));
    instance["route_clock_result"] = nlohmann::json::object();
    CFG_ASSERT(!instance.contains("config_attributes"));
    instance["config_attributes"] = nlohmann::json::array();
    for (auto& object_iter : instance["linked_objects"].items()) {
      nlohmann::json& object = object_iter.value();
      CFG_ASSERT(!object.contains("config_attributes"));
      object["config_attributes"] = nlohmann::json::array();
    }
  }
}

/*
  Create empty routing object
*/
nlohmann::json ModelConfig_IO::create_routing_object() {
  nlohmann::json routing = nlohmann::json::object();
  routing["feature"] = "";
  routing["comments"] = nlohmann::json::array();
  routing["source"] = "";
  routing["destinations"] = nlohmann::json::array();
  routing["filters"] = nlohmann::json::array();
  routing["flags"] = nlohmann::json::array();
  routing["parameters"] = nlohmann::json::object();
  return routing;
}

/*
  Prepare routing issue that need to be solved
*/
nlohmann::json ModelConfig_IO::prepare_routing_json() {
  nlohmann::json routings = nlohmann::json::array();
  POST_INFO_MSG(0, "Prepare routing resource info for fast clock");
  uint32_t instance_index = 0;
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"]) {
      std::string src_module = instance["module"];
      nlohmann::json routes = instance["route_clock_to"];
      if ((src_module == "CLK_BUF" || src_module == "PLL") &&
          (routes.size() > 0)) {
        // Only this two primitives can route clock to fast_clk
        std::string src_name = instance["name"];
        std::string src_location = get_location(src_name);
        PIN_INFO src_pin_info = get_pin_info(src_location);
        CFG_ASSERT(CFG_find_string_in_vector({"BOOT_CLOCK", "HP", "HVL", "HVR"},
                                             src_pin_info.type) >= 0);
        uint32_t fast_clock_index = 0;
        for (auto& iter : routes.items()) {
          CFG_ASSERT(fast_clock_index <= 0xFF);
          CFG_ASSERT(instance_index <= 0xFFFF);
          std::string port = iter.key();
          nlohmann::json gearboxes = iter.value();
          for (std::string dest_name : gearboxes) {
            uint32_t routing_index = (instance_index << 16) | fast_clock_index;
            CFG_ASSERT(m_routing_instance_tracker.find(routing_index) ==
                       m_routing_instance_tracker.end());
            std::string dest_module = "";
            uint8_t dest_status = 0;
            std::string dest_location =
                get_location(dest_name, &dest_module, &dest_status);
            CFG_ASSERT(dest_status & 1);
            if (dest_status & 2) {
              PIN_INFO dest_pin_info = get_pin_info(dest_location);
              CFG_ASSERT(CFG_find_string_in_vector({"HP", "HVL", "HVR"},
                                                   dest_pin_info.type) >= 0);
              std::string feature = CFG_print(
                  "Fast Clock: module %s %s port %s (location: %s) -> module "
                  "%s %s "
                  "(location: %s)",
                  src_module.c_str(), src_name.c_str(), port.c_str(),
                  src_location.c_str(), dest_module.c_str(), dest_name.c_str(),
                  dest_location.c_str());
              nlohmann::json routing = create_routing_object();
              routing["feature"] = feature;
              routing["comments"].push_back((std::string)(instance["name"]));
              routing["comments"].push_back(port);
              routing["comments"].push_back(dest_name);
              routing["parameters"][src_module] = instance["parameters"];
              if (src_pin_info.type == "BOOT_CLOCK") {
                routing["source"] =
                    CFG_print("%s->osc", src_pin_info.model_name.c_str());
              } else {
                routing["source"] = src_location;
              }
              if (src_module == "PLL") {
                routing["destinations"].push_back(
                    CFG_print("RE:%s->%s", PLL_RE.c_str(),
                              get_pll_port_name(port).c_str()));
                routing["parameters"][src_module]["__pll_enable__"] =
                    m_pll_workaround.size() ? m_pll_workaround : "1";
              } else {
                routing["filters"].push_back("partial:pll_refmux");
              }
              routing["destinations"].push_back(
                  CFG_print("%s->fast_clk", dest_pin_info.model_name.c_str()));
              if (dest_module == "O_SERDES_CLK") {
                routing["destinations"].push_back(
                    CFG_print("%s->tx_clk", dest_pin_info.model_name.c_str()));
              }
              m_routing_instance_tracker[routing_index] = int(routings.size());
              routings.push_back(routing);
            } else {
              m_routing_instance_tracker[routing_index] = -1;
              m_status = false;
            }
            fast_clock_index++;
          }
        }
      } else {
        // The rest of the primitives should have empty "route_clock_to"
        CFG_ASSERT(routes.size() == 0);
      }
    }
    instance_index++;
  }
  POST_INFO_MSG(0, "Prepare routing resource info for core clock");
  instance_index = 0;
  for (auto& instance : m_instances) {
    validate_instance(instance);
    // basic validate_locations should have been called
    // Object key must be there
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["__validation__"]) {
      std::string src_module = instance["module"];
      if (CFG_find_string_in_vector(
              {"CLK_BUF", "BOOT_CLOCK", "FCLK_BUF", "PLL", "I_SERDES"},
              src_module) >= 0) {
        bool is_pll = src_module == "PLL";
        for (uint32_t core_clock_index = 0; core_clock_index < (is_pll ? 4 : 1);
             core_clock_index++) {
          CFG_ASSERT(instance_index <= 0xFFFF);
          uint32_t routing_index =
              (instance_index << 16) | 0x0100 | core_clock_index;
          CFG_ASSERT(m_routing_instance_tracker.find(routing_index) ==
                     m_routing_instance_tracker.end());
          std::string parameter_name =
              is_pll ? CFG_print("OUT%d_ROUTE_TO_FABRIC_CLK", core_clock_index)
                     : "ROUTE_TO_FABRIC_CLK";
          if (instance["parameters"].contains(parameter_name)) {
            std::string src_name = instance["name"];
            std::string src_location =
                src_module == "FCLK_BUF"
                    ? std::string(instance["location_object"])
                    : get_location(src_name);
            PIN_INFO src_pin_info = get_pin_info(src_location);
            std::string fabric_slot = instance["parameters"][parameter_name];
            std::string port = "O";
            if (src_module == "I_SERDES") {
              port = "CLK_OUT";
            } else if (src_module == "PLL") {
              port = (core_clock_index == 0
                          ? "CLK_OUT"
                          : CFG_print("CLK_OUT_DIV%d", core_clock_index + 1));
            }
            std::string feature = CFG_print(
                "Core Clock: module %s %s port %s (location: %s) -> core "
                "clock "
                "slot[%s]",
                src_module.c_str(), src_name.c_str(), port.c_str(),
                src_location.c_str(), fabric_slot.c_str());
            nlohmann::json routing = create_routing_object();
            routing["feature"] = feature;
            routing["parameters"][src_module] = instance["parameters"];
            if (src_pin_info.type == "BOOT_CLOCK") {
              routing["source"] =
                  CFG_print("%s->osc", src_pin_info.model_name.c_str());
            } else if (src_pin_info.type == "FABRIC_CLKBUF") {
              CFG_ASSERT(
                  instance["parameters"].contains("ROUTE_FROM_FABRIC_CLK"));
              std::string fclk_buf_source =
                  instance["parameters"]["ROUTE_FROM_FABRIC_CLK"];
              routing["source"] =
                  CFG_print("%s[%s]", src_pin_info.model_name.c_str(),
                            fclk_buf_source.c_str());
            } else if (src_module == "I_SERDES") {
              routing["source"] =
                  CFG_print("%s->fast_clk", src_pin_info.model_name.c_str());
            } else {
              routing["source"] = src_location;
            }
            routing["filters"].push_back("partial:_fclk_mux_");
            if (src_module == "PLL") {
              routing["destinations"].push_back(
                  CFG_print("RE:%s->%s", PLL_RE.c_str(),
                            get_pll_port_name(port).c_str()));
              routing["parameters"][src_module]["__pll_enable__"] =
                  m_pll_workaround.size() ? m_pll_workaround : "1";
            } else {
              if (src_module == "I_SERDES") {
                routing["destinations"].push_back(
                    CFG_print("partial:root_bank_clkmux->%s_clk",
                              get_iserdes_clk_mode(instance).c_str()));
              }
              routing["filters"].push_back("partial:pll_refmux");
            }
            routing["destinations"].push_back(
                CFG_print("fabric_clk[%s]", fabric_slot.c_str()));
            m_routing_instance_tracker[routing_index] = int(routings.size());
            routings.push_back(routing);
          }
        }
      }
    }
    instance_index++;
  }
  return routings;
}

/*
  Solve routing issue
*/
void ModelConfig_IO::solve_routing_json(nlohmann::json& routings,
                                        const std::string& route_model) {
  POST_INFO_MSG(0, "Solve routing");
  CFG_ASSERT(routings.is_array());
  if (routings.size()) {
    // Write it to file
    std::filesystem::path fullpath =
        std::filesystem::absolute("io_routing.json");
    std::string json_fullpath =
        CFG_change_directory_to_linux_format(fullpath.string());
    std::ofstream io_routing_json(json_fullpath);
    CFG_ASSERT(io_routing_json.is_open());
    io_routing_json << std::setw(2) << routings << std::endl;
    io_routing_json.close();
    // Solve it
    std::vector<CFG_Python_OBJ> results = m_python->run_file(
        m_routing_config, "cpp_entry",
        std::vector<CFG_Python_OBJ>(
            {CFG_Python_OBJ((std::string)(route_model.c_str())),
             CFG_Python_OBJ(json_fullpath)}));
    CFG_ASSERT(results.size() == 1);
    CFG_ASSERT(results[0].type == CFG_Python_OBJ::TYPE::BOOL);
    CFG_ASSERT(results[0].get_bool());
    // Retrieve it
    std::ifstream input(json_fullpath.c_str());
    CFG_ASSERT(input.is_open() && input.good());
    nlohmann::json solved_routings = nlohmann::json::parse(input);
    input.close();
    // Real work
    CFG_ASSERT(solved_routings.is_array());
    CFG_ASSERT(routings.size() == solved_routings.size());
    std::map<std::string, std::map<std::string, std::string>> routing_results;
    for (auto& routing : solved_routings) {
      POST_DEBUG_MSG(1, "Feature: %s",
                     ((std::string)(routing["feature"])).c_str());
      POST_DEBUG_MSG(2, "Status: %s",
                     ((bool)(routing["status"]) ? "True" : "False"));
      for (std::string msg : routing["msgs"]) {
        POST_DEBUG_MSG(2, "Msg: %s", msg.c_str());
      }
      std::map<std::string, std::map<std::string, std::string>> rresults;
      validate_routings_result(routing, rresults);
      validate_routings_result(routing, routing_results);
      for (auto iter0 : rresults) {
        POST_DEBUG_MSG(3, "TCL Block: %s", iter0.first.c_str());
        for (auto iter1 : iter0.second) {
          POST_DEBUG_MSG(4, "%s: %s", iter1.first.c_str(),
                         iter1.second.c_str());
        }
      }
    }
    //
    for (auto iter : m_routing_instance_tracker) {
      uint32_t instance_index = iter.first >> 16;
      uint8_t type = (iter.first >> 8) & 0xFF;
      uint32_t clk_index = iter.first & 0xFF;
      CFG_ASSERT(instance_index < uint32_t(m_instances.size()));
      CFG_ASSERT(type == 0 || type == 1);
      nlohmann::json& instance = m_instances[instance_index];
      CFG_ASSERT((bool)(instance["__validation__"]));
      if (iter.second >= 0) {
        CFG_ASSERT(iter.second < int(solved_routings.size()));
      }
      if (type == 0) {
        CFG_ASSERT(iter.second == -1 || iter.second >= 0);
        nlohmann::json routes = instance["route_clock_to"];
        CFG_ASSERT(routes.size());
        uint32_t fast_clock_index = 0;
        bool found = false;
        for (auto& riter : routes.items()) {
          std::string port = riter.key();
          if (!instance["route_clock_result"].contains(port)) {
            instance["route_clock_result"][port] = nlohmann::json::array();
          }
          nlohmann::json gearboxes = riter.value();
          for (std::string dest_name : gearboxes) {
            if (fast_clock_index == clk_index) {
              // Found, double check everything match updated
              if (iter.second == -1) {
                instance["route_clock_result"][port].push_back(
                    "Error: Destination gearbox is invalid");
              } else {
                nlohmann::json& routing = solved_routings[iter.second];
                CFG_ASSERT((std::string)(routing["comments"][0]) ==
                           (std::string)(instance["name"]));
                CFG_ASSERT((std::string)(routing["comments"][1]) == port);
                CFG_ASSERT((std::string)(routing["comments"][2]) == dest_name);
                std::string msgs;
                for (std::string m : routing["msgs"]) {
                  if (msgs.size()) {
                    msgs = CFG_print("%s; %s", msgs.c_str(), m.c_str());
                  } else {
                    msgs = m;
                  }
                }
                if ((bool)(routing["status"])) {
                  instance["route_clock_result"][port].push_back(
                      CFG_print("Pass: %s", msgs.c_str()));
                } else {
                  instance["route_clock_result"][port].push_back(
                      CFG_print("Error: %s", msgs.c_str()));
                }
                get_routing_result(instance, routing, false);
              }
              found = true;
              break;
            }
            fast_clock_index++;
          }
          if (found) {
            break;
          }
        }
        CFG_ASSERT(found);
      } else {
        CFG_ASSERT(iter.second >= 0);
        nlohmann::json& routing = solved_routings[iter.second];
        get_routing_result(instance, routing, true);
      }
    }
  }
}

/*
  Determine the PLL model port name from primitive port name
*/
std::string ModelConfig_IO::get_pll_port_name(
    std::string& pritimive_port_name) {
  std::string name = "";
  if (pritimive_port_name == "FAST_CLK") {
    name = "foutvco";
  } else if (pritimive_port_name == "CLK_OUT") {
    name = "fout[0]";
  } else if (pritimive_port_name == "CLK_OUT_DIV2") {
    name = "fout[1]";
  } else if (pritimive_port_name == "CLK_OUT_DIV3") {
    name = "fout[2]";
  } else if (pritimive_port_name == "CLK_OUT_DIV4") {
    name = "fout[3]";
  }
  CFG_ASSERT_MSG(name.size(), "Unknown PLL primitive port name %s",
                 pritimive_port_name.c_str());
  return name;
}

/*
  Determine the I_SERDES clock mode
*/
std::string ModelConfig_IO::get_iserdes_clk_mode(nlohmann::json& instance) {
  validate_instance(instance);
  CFG_ASSERT(instance["__validation__"]);
  std::string mode = "core";
  if (instance["parameters"].contains("DPA_MODE") &&
      CFG_find_string_in_vector({"DPA", "CDR"},
                                instance["parameters"]["DPA_MODE"]) >= 0) {
    mode = "cdr";
  }
  return mode;
}

/*
  Vaidate the routings result
*/
void ModelConfig_IO::validate_routings_result(
    nlohmann::json& routing,
    std::map<std::string, std::map<std::string, std::string>>& results) {
  validate_routing(routing, true);
  if ((bool)(routing["status"])) {
    CFG_ASSERT(routing["config mux"].size());
    bool solved = false;
    for (auto& r : routing["config mux"]) {
      if (r.is_object()) {
        CFG_ASSERT(r.size() == 1);
        for (auto& iter0 : r.items()) {
          CFG_ASSERT(((nlohmann::json)(iter0.key())).is_string());
          std::string key0 = (std::string)(iter0.key());
          if (results.find(key0) == results.end()) {
            results[key0] = (std::map<std::string, std::string>){};
          }
          for (auto& iter1 : iter0.value().items()) {
            std::string key1 = (std::string)(iter1.key());
            std::string value = std::string(iter1.value());
            if (results[key0].find(key1) == results[key0].end()) {
              results[key0][key1] = value;
            } else {
              CFG_ASSERT(results[key0][key1] == value);
            }
            solved = true;
          }
        }
      } else {
        CFG_ASSERT(r.is_null());
      }
    }
    CFG_ASSERT(solved);
  }
}

/*
  Retrieve result from routing to instance
*/
void ModelConfig_IO::get_routing_result(nlohmann::json& instance,
                                        nlohmann::json& routing,
                                        bool update_error) {
  validate_instance(instance);
  validate_routing(routing, true);
  if ((bool)(routing["status"])) {
    CFG_ASSERT(routing["config mux"].size());
    std::map<std::string, std::map<std::string, std::string>> results;
    validate_routings_result(routing, results);
    CFG_ASSERT(results.size());
    std::vector<std::string> locations;
    for (auto& iter0 : results) {
      bool configured = false;
      for (auto& config : instance["config_attributes"]) {
        if (config["__location__"] == iter0.first) {
          configured = true;
          break;
        }
      }
      if (!configured) {
        nlohmann::json attributes = nlohmann::json::object();
        attributes["__location__"] = iter0.first;
        instance["config_attributes"].push_back(attributes);
        if (std::find(locations.begin(), locations.end(), iter0.first) ==
            locations.end()) {
          locations.push_back(iter0.first);
        }
      }
      configured = false;
      for (auto& config : instance["config_attributes"]) {
        if (config["__location__"] == iter0.first) {
          for (auto& iter1 : iter0.second) {
            if (!config.contains(iter1.first)) {
              config[iter1.first] = iter1.second;
            } else {
              CFG_ASSERT(config[iter1.first] == iter1.second);
            }
          }
          configured = true;
          break;
        }
      }
      CFG_ASSERT(configured);
    }
  } else {
    if (update_error) {
      for (std::string m : routing["msgs"]) {
        instance["errors"].push_back(
            CFG_print("ModelConfigError: %s", m.c_str()));
      }
    }
    m_status = false;
  }
}

/**********************************
 *
 * Fucntions to set configure attributes
 *
 **********************************/
/*
  Entry function to set configuration attributes
*/
void ModelConfig_IO::set_config_attributes() {
  POST_INFO_MSG(0, "Set configuration attributes");
  for (auto& instance : m_instances) {
    validate_instance(instance, true);
    if ((bool)(instance["__validation__"])) {
      std::string module = std::string(instance["module"]);
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
                             instance["connectivity"], args, define);
        args = instance_args;
        args["__location__"] = location;
        POST_DEBUG_MSG(3, "Property");
        set_config_attribute(object["config_attributes"], module,
                             (std::string)(instance["pre_primitive"]),
                             instance["post_primitives"], properties,
                             m_config_mapping["properties"],
                             instance["connectivity"], args, define);
      }
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
    std::map<std::string, std::string>& args, nlohmann::json define) {
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
        nlohmann::json neg_results = rules.contains("neg_results")
                                         ? rules["neg_results"]
                                         : nlohmann::json::array();
        set_config_attribute_by_rules(config_attributes, inputs, connectivity,
                                      rules["rules"], rules["results"],
                                      neg_results, args, define);
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
    nlohmann::json define) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(results.is_object() || results.is_array());
  CFG_ASSERT(neg_results.is_object() || neg_results.is_array());
  size_t expected_match = rules.size();
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
    set_config_attribute_by_rule(config_attributes, results, args, define);
  } else {
    POST_DEBUG_MSG(5, "Mismatch");
    set_config_attribute_by_rule(config_attributes, neg_results, args, define);
  }
}

/*
  To set configuration attributes for instance after evaluate if the rule
  match
*/
void ModelConfig_IO::set_config_attribute_by_rule(
    nlohmann::json& config_attributes, nlohmann::json& results,
    std::map<std::string, std::string>& args, nlohmann::json define) {
  CFG_ASSERT(config_attributes.is_array());
  CFG_ASSERT(results.is_object() || results.is_array());
  if (results.is_object()) {
    finalize_config_result(config_attributes, results, define, args);
  } else {
    for (auto result : results) {
      finalize_config_result(config_attributes, result, define, args);
    }
  }
}

void ModelConfig_IO::finalize_config_result(
    nlohmann::json& config_attributes, nlohmann::json result,
    nlohmann::json define, std::map<std::string, std::string>& args) {
  CFG_ASSERT(result.is_object());
  for (auto iter : result.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    CFG_ASSERT(iter.value().is_string());
    std::string key = (std::string)(iter.key());
    std::string value = (std::string)(iter.value());
  }
  if (result.contains("__define__")) {
    CFG_ASSERT(result["__define__"].is_string());
    std::vector<std::string> definitions =
        CFG_split_string(std::string(result["__define__"]), ";", 0, false);
    for (auto definition : definitions) {
      CFG_ASSERT(define.size());
      CFG_ASSERT(define.contains(definition));
      POST_DEBUG_MSG(6, "Defined function: %s", definition.c_str());
      define_args(define[definition], args);
    }
    result.erase("__define__");
  }
  nlohmann::json final_result = nlohmann::json::object();
  for (auto iter : result.items()) {
    CFG_ASSERT(((nlohmann::json)(iter.key())).is_string());
    CFG_ASSERT(iter.value().is_string());
    std::string value = (std::string)(iter.value());
    for (auto arg : args) {
      value = CFG_replace_string(value, arg.first, arg.second, false);
    }
    final_result[(std::string)(iter.key())] = value;
  }
  if (final_result.size()) {
    config_attributes.push_back(final_result);
  }
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
                                         std::string* module, uint8_t* status) {
  if (status != nullptr) {
    *status = 0;
  }
  std::string location = "";
  for (auto& instance : m_instances) {
    validate_instance(instance);
    CFG_ASSERT(instance.contains("__validation__"));
    CFG_ASSERT(instance.contains("__validation_msg__"));
    CFG_ASSERT(instance["__validation__"].is_boolean());
    CFG_ASSERT(instance["__validation_msg__"].is_string());
    if (instance["name"] == name) {
      if (status != nullptr) {
        *status = 1;
      }
      if ((bool)(instance["__validation__"])) {
        location = (std::string)(instance["location"]);
        if (location.find("__SKIP_LOCATION_CHECK__") == 0) {
          location = location.substr(23);
          if (location.find(":") == 0) {
            location = location.substr(1);
          }
        }
        if (module != nullptr) {
          (*module) = (std::string)(instance["module"]);
        }
        if (status != nullptr) {
          *status |= 2;
        }
      } else {
      }
      break;
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
                                        const std::string& seq_name,
                                        bool skip) {
  if (status || skip) {
    if (msg.size()) {
      msg = CFG_print("%s,%s%s", msg.c_str(), seq_name.c_str(),
                      skip ? "[skip]" : "");
    } else {
      msg = CFG_print("Pass:%s%s", seq_name.c_str(), skip ? "[skip]" : "");
    }
  } else {
    if (msg.size()) {
      msg = CFG_print("%s;Fail:%s", msg.c_str(), seq_name.c_str());
    } else {
      msg = CFG_print("Fail:%s", seq_name.c_str());
    }
#if 0
    POST_ERROR_MSG(
        1,
        "Skip module:%s name:%s location(s):\"%s\" because it failed in %s "
        "validation",
        module.c_str(), name.c_str(), location.c_str(), seq_name.c_str());
#else
    std::string msg = CFG_print(
        "Error: Skip module:%s name:%s location(s):\"%s\" because it failed "
        "in "
        "%s validation",
        module.c_str(), name.c_str(), location.c_str(), seq_name.c_str());
    m_messages.push_back(new ModelConfig_IO_MSG(1, msg));
#endif
    m_status = false;
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
                                 std::map<std::string, std::string>& args) {
  CFG_ASSERT(m_python != nullptr);
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
  m_python->run(commands, arguments);
  CFG_ASSERT_MSG(m_python->results().size() == arguments.size(),
                 "Expect there is %ld Python result, but found %ld",
                 arguments.size(), m_python->results().size());
  for (auto arg : arguments) {
    args[arg] = m_python->result_str(arg);
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
  Getting pin information from Python
*/
PIN_INFO ModelConfig_IO::get_pin_info(const std::string& name) {
  CFG_ASSERT(m_python != nullptr);
  std::vector<CFG_Python_OBJ> results =
      m_python->run_file("config", "get_pin_info",
                         std::vector<CFG_Python_OBJ>({CFG_Python_OBJ(name)}));
  CFG_ASSERT_MSG(results.size() == 8,
                 "Expect Python get_pin_info(%s) function return 8 arguments, "
                 "but found %ld",
                 name.c_str(), results.size());
  CFG_ASSERT(results[0].type == CFG_Python_OBJ::TYPE::STR);
  CFG_ASSERT(results[1].type == CFG_Python_OBJ::TYPE::INT);
  CFG_ASSERT(results[2].type == CFG_Python_OBJ::TYPE::BOOL);
  CFG_ASSERT(results[3].type == CFG_Python_OBJ::TYPE::INT);
  CFG_ASSERT(results[4].type == CFG_Python_OBJ::TYPE::INT);
  CFG_ASSERT(results[5].type == CFG_Python_OBJ::TYPE::INT);
  CFG_ASSERT(results[6].type == CFG_Python_OBJ::TYPE::STR);
  CFG_ASSERT(results[7].type == CFG_Python_OBJ::TYPE::STR);
  ;
  return PIN_INFO(results[0].get_str(), results[1].get_u32(),
                  results[2].get_bool(), results[3].get_u32(),
                  results[4].get_u32(), results[5].get_u32(),
                  results[6].get_str(), results[7].get_str());
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
  nlohmann::json messages = nlohmann::json::array();
  for (auto& msg : m_messages) {
    std::string message = msg->msg;
    for (uint32_t i = 0; i < msg->offset; i++) {
      message = "  " + message;
    }
    messages.push_back(message);
  }
  write_json(file, m_status, "IO", messages, m_instances);
}

void ModelConfig_IO::write_json(const std::string& file, bool status,
                                const std::string& feature,
                                nlohmann::json& messages,
                                nlohmann::json& instances) {
  CFG_ASSERT(messages.is_array());
  std::ofstream json(file.c_str());
  size_t index = 0;
  json << "{\n";
  json << "    \"status\": " << (status ? "true" : "false") << ",\n";
  json << "    \"feature\": \"" << feature.c_str() << "\",\n";
  json << "    \"messages\": [\n";
  for (auto& msg : messages) {
    CFG_ASSERT(msg.is_string());
    if (index) {
      json << ",\n";
    }
    json << "    \"";
    write_json_data(msg, json);
    json << "\"";
    json.flush();
    index++;
  }
  if (index) {
    json << "\n";
  }
  json << "  ],\n";
  json << "  \"instances\": [\n";
  index = 0;
  for (auto& instance : instances) {
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
  write_json_object("location_object", instance["location_object"], json);
  json << ",\n";
  write_json_object("location", instance["location"], json);
  json << ",\n";
  write_json_object("linked_object", instance["linked_object"], json);
  json << ",\n";
  std::vector<std::string> obj_seq =
      CFG_split_string(std::string(instance["linked_object"]), "+", 0, false);
  nlohmann::json& objects = instance["linked_objects"];
  CFG_ASSERT(obj_seq.size() == objects.size());
  json << "      \"linked_objects\": {\n";
  size_t index = 0;
  for (auto& obj : obj_seq) {
    if (index) {
      json << ",\n";
    }
    CFG_ASSERT(objects.contains(obj));
    nlohmann::json object = objects[obj];
    json << "        \"" << obj.c_str() << "\": {\n";
    write_json_object("location", std::string(object["location"]), json, 5);
    json << ",\n";
    json << "          \"properties\": {\n";
    write_json_map(object["properties"], json, 6);
    json << "          },\n";
    json << "          \"config_attributes\": [\n";
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
  json << "      \"connectivity\": {\n";
  write_json_map(instance["connectivity"], json);
  json << "      },\n";
  json << "      \"parameters\": {\n";
  write_json_map(instance["parameters"], json);
  json << "      },\n";
  json << "      \"flags\": [\n";
  write_json_array(get_json_string_list(instance["flags"], args), json);
  json << "      ],\n";
  write_json_object("pre_primitive", (std::string)(instance["pre_primitive"]),
                    json);
  json << ",\n";
  json << "      \"post_primitives\": [\n",
      write_json_array(get_json_string_list(instance["post_primitives"], args),
                       json);
  json << "      ],\n";
  for (auto key :
       std::vector<std::string>({"route_clock_to", "route_clock_result"})) {
    index = 0;
    json << "      \"" << key.c_str() << "\": {\n";
    for (auto iter : instance[key].items()) {
      if (index) {
        json << ",\n";
      }
      json << "        \"" << std::string(iter.key()).c_str() << "\": [\n";
      write_json_array(get_json_string_list(iter.value(), args), json, 5);
      json << "        ]";
      index++;
    }
    if (index) {
      json << "\n";
    }
    json << "      },\n";
  }
  json << "      \"errors\": [\n";
  write_json_array(get_json_string_list(instance["errors"], args), json, 4);
  json << "      ],\n";
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
  if ((bool)(instance["__validation__"])) {
    json << "      \"__validation__\": true,\n";
  } else {
    json << "      \"__validation__\": false,\n";
  }
  write_json_object("__validation_msg__",
                    std::string(instance["__validation_msg__"]), json);
  json << ",\n";
  json << "      \"config_attributes\": [\n";
  size_t attr_index = 0;
  for (auto& cfg_attr : instance["config_attributes"]) {
    CFG_ASSERT(cfg_attr.is_object());
    if (attr_index) {
      json << ",\n";
    }
    json << "        {\n";
    write_json_map(cfg_attr, json, 5);
    json << "        }";
    attr_index++;
  }
  if (attr_index) {
    json << "\n";
  }
  json << "      ]";
  json << "\n    }";
}

/*
  To write object into JSON
    "string": "string"
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
  json << ": ";
  json << "\"";
  write_json_data(value, json);
  json << "\"";
}

/*
  To write object into JSON
    "string": "string",
    "string": "string",
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
    json << "\"";
    write_json_data(iter, json);
    json << "\"";
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

}  // namespace FOEDAG
