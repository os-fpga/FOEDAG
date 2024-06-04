/**
 * @file   device_modeler.h
 * @author Manadher Kharroubi (manadher@gmail.com)
 * @brief
 * @version 0.9
 * @date 2023-06-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif
#include <cfloat>
#include <climits>
#include <limits>
#include <memory>
#include <regex>
#include <unordered_map>

#include "Configuration/CFGCommon/CFGCommon.h"
#include "Utils/StringUtils.h"
#include "device.h"
#include "speedlog.h"

/**
 * @class device_modeler
 * @brief Singleton class representing a device modeler.
 *
 * A device modeler is responsible for managing device objects. It ensures that
 * each device, identified by a unique combination of name and version, is only
 * created once.
 */
class device_modeler {
 public:
  /**
   * @brief Get the singleton device_modeler instance.
   * @return A reference to the singleton device_modeler.
   */
  static device_modeler &instance() {
    static device_modeler instance_;
    return instance_;
  }

  /**
   * @brief Add a new device to the device map.
   * @param name The name of the device.
   * @param version The version of the device.
   * @param device The new device to add.
   * @return  A boolean indicating whether the addition was successful.
   *          False means a device with the same key already exists.
   */
  bool add_device(const std::string &name, std::shared_ptr<device> device) {
    std::string key = name;
    auto result = (devices_.find(key) == end(devices_));
    if (result) devices_[key] = device;
    return result;
  }

  /**
   * @brief Get a specific device from the device map.
   * @param name The name of the device.
   * @param version The version of the device.
   * @return A shared pointer to the requested device, or nullptr if the device
   * does not exist.
   */
  std::shared_ptr<device> get_device(const std::string &name) {
    std::string key = name;
    auto it = devices_.find(key);
    if (it != devices_.end())
      return it->second;
    else {
      return nullptr;
    }
  }

  device_modeler(device_modeler const &) = delete;  // Prevent copying
  void operator=(device_modeler const &) = delete;  // Prevent assignment

  /**
   * @brief Update or create a device with a specified name.
   * @param argc The number of arguments.
   * @param argv The arguments array.
   * @return A boolean indicating whether the operation was successful.
   * @throws std::invalid_argument if the device name does not start with an
   * alphanumeric character or '_'.
   */
  bool device_name(int argc, const char **argv) {
    if (argc < 2 || (!std::isalnum(argv[1][0]) && argv[1][0] != '_')) {
      std::string s =
          std::string("Invalid device name: ") + std::string(argv[1]) +
          std::string(", should start an alphanumeric character or \"_\"");
      throw std::invalid_argument(s.c_str());
    }
    std::string name = argv[1];
    current_device_ = get_device(name);
    if (!current_device_) {
      current_device_ = std::make_shared<device>(name);
      devices_[name] = current_device_;
    }
    add_default_muxes();
    return true;
  }

  /**
   * @brief Undefine device if the name exists
   * @param argc The number of arguments.
   * @param argv The arguments array.
   * @return A boolean indicating whether the operation was successful.
   * @throws std::invalid_argument if the device name does not start with an
   * alphanumeric character or '_'.
   */
  bool undefine_device(int argc, const char **argv) {
    if (argc < 2 || (!std::isalnum(argv[1][0]) && argv[1][0] != '_')) {
      std::string s =
          std::string("Invalid device name: ") + std::string(argv[1]) +
          std::string(" should start an alphanumeric character or or \"_\"");
      throw std::invalid_argument(s.c_str());
    }
    std::string name = argv[1];
    if (devices_.find(name) != devices_.end()) {
      devices_.erase(name);
    }
    return true;
  }

  /**
   * @brief Set the version of the current device.
   * @param argc The number of arguments.
   * @param argv The arguments array.
   * @return A boolean indicating whether the operation was successful.
   * @throws std::runtime_error if there is no current device.
   * @throws std::invalid_argument if the version is not a valid string.
   */
  bool device_version(int argc, const char **argv) {
    if (!current_device_) {
      throw std::runtime_error("No current device");
    }
    if (argc < 2 || argv[1] == nullptr || strlen(argv[1]) == 0) {
      throw std::invalid_argument("Invalid version string");
    }
    current_device_->set_device_version(argv[1]);
    return true;
  }

  /**
   * @brief Set the schema version of the current device.
   * @param argc The number of arguments.
   * @param argv The arguments array.
   * @return A boolean indicating whether the operation was successful.
   * @throws std::runtime_error if there is no current device.
   * @throws std::invalid_argument if the schema version is not a valid string.
   */
  bool schema_version(int argc, const char **argv) {
    if (!current_device_) {
      throw std::runtime_error("No current device");
    }
    if (argc < 2 || argv[1] == nullptr || strlen(argv[1]) == 0) {
      throw std::invalid_argument("Invalid schema version string");
    }
    current_device_->set_schema_version(argv[1]);
    return true;
  }
  void reset_current_device() {
    current_device_ = nullptr;  // method to reset the state
  }
  std::shared_ptr<device> get_current_device() { return current_device_; }
  std::vector<std::string> split_string_by_space(
      const std::string &inputString) {
    std::vector<std::string> result;
    std::istringstream iss(inputString);
    std::string token;

    while (iss >> token) {
      result.push_back(token);
    }

    return result;
  }

  void add_default_muxes() {
    if (!current_device_)
      throw std::runtime_error(
          "No current device when adding default mux types");
    std::vector<std::pair<std::string, std::string>> ports = {
        {"out", "o"},  {"in", "i0"},  {"in", "i1"},  {"in", "i2"},
        {"in", "i3"},  {"in", "i4"},  {"in", "i5"},  {"in", "i6"},
        {"in", "i7"},  {"in", "i8"},  {"in", "i9"},  {"in", "i10"},
        {"in", "i11"}, {"in", "i12"}, {"in", "i13"}, {"in", "i14"},
        {"in", "i15"}, {"in", "i16"}, {"in", "i17"}, {"in", "i18"},
        {"in", "i19"}, {"in", "i20"}, {"in", "i21"}, {"in", "i22"},
        {"in", "i23"}, {"in", "i24"}, {"in", "i25"}, {"in", "i26"},
        {"in", "i27"}, {"in", "i28"}, {"in", "i29"}, {"in", "i30"},
        {"in", "i31"}};
    std::vector<int> number_of_inputs = {2, 4, 8, 16, 32};
    for (int in_cnt : number_of_inputs) {
      std::stringstream ss;
      ss << "MUX" << in_cnt << "X1";
      std::string blockName = ss.str();
      auto block = std::make_shared<device_block>(blockName);
      for (int k = 0; k <= in_cnt; k++) {
        block->add_port(std::make_shared<device_port>(ports[k].second,
                                                      ports[k].first == "in"));
      }
      current_device_->add_block(block);
      const int argc = 11;
      const char *argv[argc] = {"define_attr", "-block",   "MUX2X1", "-name",
                                "sel",         "-width",   "1",      "-addr",
                                "0",           "-default", "0"};
      argv[2] = blockName.c_str();
      int sz = 0;
      while ((in_cnt = in_cnt >> 1)) {
        sz++;
      }
      std::string str;
      std::stringstream snm;
      snm << sz;
      snm >> str;
      argv[6] = str.c_str();
      this->define_attr(argc, argv, block.get());
      const int argc1 = 5;
      const char *argv1[argc1] = {"define_properties", "-block", argv[2],
                                  "-no_configuration", "on"};
      this->define_properties(argc1, argv1);
    }
  }
  std::unordered_map<std::string, int> parse_enum_values(
      const std::string &str) {
    std::unordered_map<std::string, int> result;
    std::vector<std::string> enums = CFG_split_string(str, ",", 0, false);
    for (auto &e : enums) {
      std::vector<std::string> pairs = CFG_split_string(e, " ", 0, false);
      CFG_ASSERT(pairs.size() == 2);
      CFG_ASSERT(result.find(pairs[0]) == result.end());
      result[pairs[0]] = convert_string_to_integer(pairs[1]);
    }
    CFG_ASSERT(result.size());
    return result;
  }

  std::unordered_map<std::string, int> parse_values(const std::string &str) {
    if (str.find_first_of("{}") == std::string::npos) {
      return parse_enum_values(str);
    }
    std::unordered_map<std::string, int> result;
    std::regex re("\\{([^}]+)\\}");  // Matches anything inside curly braces
    std::smatch match;

    std::string::const_iterator search_start(str.cbegin());
    while (regex_search(search_start, str.cend(), match, re)) {
      std::string pair = match[1];
      size_t delimiter_pos = pair.find(",");
      if (delimiter_pos != std::string::npos) {
        std::string name = pair.substr(0, delimiter_pos);
        std::string value_str = pair.substr(delimiter_pos + 1);
        int value = convert_string_to_integer(value_str);
        result[name] = value;
      }
      search_start += match.position() + match.length();
    }
    return result;
  }

  std::string get_argument_value(const std::string &arg_name, int argc,
                                 const char **argv, bool required = false) {
    for (int i = 0; i < argc; i++) {
      if (arg_name == argv[i] && i + 1 < argc) {
        return argv[i + 1];
      }
    }

    if (required) {
      throw std::invalid_argument("Missing necessary argument: " + arg_name);
    }

    return "";
  }
  std::string get_argument_values(const std::string &arg_name, int argc,
                                  const char **argv, bool required = false) {
    bool found = false;
    int idx = argc;
    for (int i = 0; i < argc; i++) {
      if (arg_name == argv[i] && i + 1 < argc) {
        found = true;
        idx = i + 1;
        break;
      }
    }
    std::string ret = "";
    if (found) {
      while (idx < argc && argv[idx][0] != '-') {
        bool alpha = isalpha(argv[idx][0]) || argv[idx][0] == '_';
        ret += argv[idx++];
        if (alpha) ret += ",";
      }
      return ret;
    }
    if (!found && required) {
      throw std::invalid_argument("Missing necessary argument: " + arg_name);
    }
    return "";
  }

  bool argument_exists(const std::string &arg_name, int argc,
                       const char **argv) {
    for (int i = 0; i < argc; i++) {
      if (arg_name == argv[i]) {
        return true;
      }
    }

    return false;
  }

  bool define_enum_type(int argc, const char **argv) {
    if (!current_device_) {
      throw std::runtime_error("No current device");
    }
    std::string enumName = get_argument_value("-name", argc, argv, true);
    std::string sz = get_argument_value("-size", argc, argv);
    std::string block_name = get_argument_value("-block", argc, argv);
    // block_name is optional. If it's empty, use current_device scope
    device_block *block;
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }
    unsigned size = 0;
    if ("" != sz) {
      size = convert_string_to_integer(sz);
    }
    std::unordered_map<std::string, int> values;
    bool force = argument_exists("-force", argc, argv);
    std::string enum_vals = get_argument_values("-values", argc, argv, true);
    values = parse_values(enum_vals);
    // Create the new enum type
    auto newEnum = make_shared<ParameterType<int>>();
    if (size)
      newEnum->set_size(size);
    else
      newEnum->set_size(10);

    for (auto &p : values) {
      newEnum->set_enum_value(p.first, p.second);
    }
    try {
      block->get_enum_type(enumName);
      if (!force) {
        std::string err = "Enum type " + enumName +
                          " already exists. Use -force to override.";
        spdlog::warn(err.c_str());
        return false;
      }
      block->add_enum_type(enumName, newEnum);
    } catch (...) {
      block->add_enum_type(enumName, newEnum);
    }
    return true;
  }
  bool define_block(int argc, const char **argv) {
    if (!current_device_) {
      std::string dname = "__auto_generated_device__";
      current_device_ = std::make_shared<device>(dname);
      devices_[dname] = current_device_;
      add_default_muxes();
    }

    std::string blockName;
    std::vector<std::pair<std::string, std::string>> ports;

    for (int i = 0; i < argc; ++i) {
      if (strcmp(argv[i], "-name") == 0 && i + 1 < argc) {
        blockName = argv[++i];
      } else if (strcmp(argv[i], "-in") == 0 || strcmp(argv[i], "-out") == 0) {
        std::string direction = (strcmp(argv[i], "-in") == 0) ? "in" : "out";
        i++;  // skip the "-in" or "-out"

        while (i < argc && argv[i][0] != '-') {
          std::string portName = argv[i++];
          ports.emplace_back(direction, portName);
        }
        if (i < argc) i--;  // back to the last read not a port value
      }
    }

    if (blockName.empty()) {
      throw std::invalid_argument("Block name is not provided or invalid");
    }

    // Create a new block with the given name and ports
    auto block = std::make_shared<device_block>(blockName);

    for (auto &p : ports) {
      block->add_port(std::make_shared<device_port>(p.second, p.first == "in"));
    }

    current_device_->add_block(block);
    return true;
  }

  /**
   * @brief Adds ports to an already defined block.
   *
   * This function adds additional ports to a previously defined block. The
   * ports are specified as pairs of direction and port names. The function
   * modifies the specified block within the current device being worked on.
   *
   * Example command: define_ports -block kkk -ports  in a  out aa  in ss
   *
   * @param argc The number of arguments.
   * @param argv The arguments array.
   * @return A boolean indicating whether the operation was successful.
   * @throws std::invalid_argument if the block name is not provided or invalid
   * or if the ports are not properly formatted.
   * @throws std::runtime_error if there is no current device, or the block does
   * not exist in current device.
   */
  bool define_ports_old(int argc, const char **argv) {
    if (!current_device_) {
      throw std::runtime_error("No current device");
    }
    std::string blockName;
    std::vector<std::pair<std::string, std::string>> ports;
    for (int i = 0; i < argc; ++i) {
      if (strcmp(argv[i], "-block") == 0 && i + 1 < argc) {
        blockName = argv[++i];
      } else if (strcmp(argv[i], "-ports") == 0) {
        i++;  // skip the "-ports"
        while (i < argc && argv[i][0] != '-') {
          std::string direction = argv[i++];

          if (i >= argc || argv[i][0] == '-') {
            throw std::invalid_argument("Missing port name for direction: " +
                                        direction);
          }

          std::string portName = argv[i++];
          ports.emplace_back(direction, portName);
        }
        if (i < argc) i--;  // back to the last read not a port value
      }
    }

    if (blockName.empty()) {
      throw std::invalid_argument("Block name is not provided or invalid");
    }

    // Fetch the block from the current device.
    auto block = current_device_->get_block(blockName);
    if (!block) {
      throw std::runtime_error("Block with name " + blockName +
                               " does not exist in current device");
    }

    if (!ports.size()) {
      throw std::invalid_argument("No ports specified in add_ports");
    }
    // Add new ports to the fetched block.
    for (auto &p : ports) {
      block->add_port(std::make_shared<device_port>(p.second, p.first == "in"));
    }

    return true;
  }

  bool define_ports(int argc, const char **argv) {
    if (!current_device_) {
      throw std::runtime_error("No current device");
    }
    std::string blockName;
    std::vector<std::pair<std::string, std::string>> ports;

    for (int i = 0; i < argc; ++i) {
      if (strcmp(argv[i], "-block") == 0 && i + 1 < argc) {
        blockName = argv[++i];
      } else if (strcmp(argv[i], "-in") == 0 || strcmp(argv[i], "-out") == 0) {
        std::string direction = (strcmp(argv[i], "-in") == 0) ? "in" : "out";
        i++;  // skip the "-in" or "-out"
        while (i < argc && argv[i][0] != '-') {
          std::string portName = argv[i++];
          ports.emplace_back(direction, portName);
        }
        if (i < argc) i--;  // back to the last read not a port value
      }
    }

    if (blockName.empty()) {
      throw std::invalid_argument("Block name is not provided or invalid");
    }
    // Fetch the block from the current device.
    auto block = current_device_->get_block(blockName);
    if (!block) {
      throw std::runtime_error("Block with name " + blockName +
                               " does not exist in current device");
    }

    if (!ports.size()) {
      throw std::invalid_argument("No ports specified in add_ports");
    }

    // Add new ports to the fetched block.
    for (auto &p : ports) {
      block->add_port(std::make_shared<device_port>(p.second, p.first == "in",
                                                    nullptr, block.get()));
      // std::cout << "Done creating port " << p.second << std::endl;
    }
    for (auto &p : ports) {
      auto sg = block->get_port(p.second)->get_signal();
      block->add_signal(p.second, std::shared_ptr<device_signal>(sg));
    }
    return true;
  }

  /**
   * @brief Defines a new parameter type.
   * The function uses the provided arguments to create a parameter type with
   * the specified properties. This parameter type can be used later to define
   * parameters.
   *
   * @param argc The number of arguments provided.
   * @param argv An array of strings containing the arguments. The expected
   * arguments are:
   *  - -block <block_name>: (Optional) The name of a defined block to insert
   * this attribute in, if None passed we use the current_device scope.
   *  - -name <par_name>: The name of the currently defined parameter.
   *  - -width <nb_bits>: (Optional) Number of bits intended to represent the
   * values of this parameter.
   *  - -base_type <type_name>: The base type of the parameter.
   *  - -lower_bound <l_bound>: (Optional) The lower bound for the parameter
   * value.
   *  - -upper_bound <u_bound>: (Optional) The upper bound for the parameter
   * value.
   *  - -force: (Optional) If present, any existing attribute with the same name
   * will be overwritten.
   *
   * @return True if the parameter type was successfully defined, false
   * otherwise.
   *
   * @throws std::invalid_argument If a necessary argument is missing, or if an
   * argument has an invalid value.
   * @throws std::runtime_error If there is an error during the creation of the
   * parameter type.
   */
  bool define_param_type(int argc, const char **argv) {
    // Check at least two parameters (command name and -name <par_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_param_type.");
    }
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string par_name = get_argument_value("-name", argc, argv, true);
    std::string width = get_argument_value("-width", argc, argv);
    std::string base_type = get_argument_value("-base_type", argc, argv, true);
    std::string l_bound = get_argument_value("-lower_bound", argc, argv);
    std::string u_bound = get_argument_value("-upper_bound", argc, argv);
    std::string default_value = get_argument_value("-default", argc, argv);
    bool force = argument_exists("-force", argc, argv);
    // block_name is optional. If it's empty, use current_device scope
    device_block *block;
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }

    if (base_type == "int") {
      if ((block->get_int_parameter_type(par_name) ||
           block->get_double_parameter_type(par_name) ||
           block->get_string_parameter_type(par_name)) &&
          !force) {
        throw std::runtime_error("Parameter " + par_name +
                                 " already exists. Use -force to overwrite.");
      }
      auto paramType = std::make_shared<ParameterType<int>>();
      int num_width = -1;
      int lower = INT_MIN;
      int upper = INT_MAX;
      if ("" != width) {
        num_width = convert_string_to_integer(width);
      }
      if ("" != l_bound) {
        lower = convert_string_to_integer(l_bound);
      }
      if ("" != u_bound) {
        upper = convert_string_to_integer(u_bound);
      }
      if ("" != default_value) {
        int default_int = convert_string_to_integer(default_value);
        paramType->set_default_value(default_int);
      } else {
        paramType->set_default_value(0);
      }
      if (num_width > 0) {
        paramType->set_size((size_t)num_width);
      } else {
        paramType->set_size(__SIZEOF_INT__ * 8);
      }
      paramType->set_lower_bound(lower);
      paramType->set_upper_bound(upper);
      block->add_int_parameter_type(par_name, paramType);
    } else if (base_type == "double") {
      if ((block->get_int_parameter_type(par_name) ||
           block->get_double_parameter_type(par_name) ||
           block->get_string_parameter_type(par_name)) &&
          !force) {
        throw std::runtime_error("Parameter " + par_name +
                                 " already exists. Use -force to overwrite.");
      }
      auto paramType = std::make_shared<ParameterType<double>>();
      const double highest_double = std::numeric_limits<double>::max();
      constexpr double lowest_double = std::numeric_limits<double>::lowest();
      double lower = lowest_double;
      double upper = highest_double;
      if ("" != l_bound) {
        lower = convert_string_to_double(l_bound);
      }
      if ("" != u_bound) {
        upper = convert_string_to_double(u_bound);
      }
      if ("" != default_value) {
        double default_double = convert_string_to_double(default_value);
        paramType->set_default_value(default_double);
      } else {
        paramType->set_default_value(0.0);
      }
      paramType->set_lower_bound(lower);
      paramType->set_upper_bound(upper);
      block->add_double_parameter_type(par_name, paramType);
    } else if (base_type == "string") {
      /// TO_DO note that if you force , you may define the same name as int AND
      /// double AND string
      if ((block->get_int_parameter_type(par_name) ||
           block->get_double_parameter_type(par_name) ||
           block->get_string_parameter_type(par_name)) &&
          !force) {
        throw std::runtime_error("Parameter " + par_name +
                                 " already exists. Use -force to overwrite.");
      }
      auto paramType = std::make_shared<ParameterType<std::string>>();
      paramType->set_default_value(default_value);
      block->add_string_parameter_type(par_name, paramType);
    } else {
      throw std::invalid_argument("Invalid base_type: " + base_type);
    }

    return true;
  }

  /**
   * @brief Defines a new parameter.
   *
   * This function uses the provided arguments to define a parameter with the
   * specified properties. The parameter can later be used in blocks or devices.
   *
   * The parameter type can be either a basic type (string, double, int) or a
   * type defined by a previously defined `define_param_type` command.
   *
   * @param argc The number of arguments provided.
   * @param argv An array of strings containing the arguments. The expected
   * arguments are:
   *  - -block <block_name>: (Optional) The name of a defined block to insert
   * this parameter in, if None passed we use the current_device scope.
   *  - -name <param_name>: The name of the parameter to define.
   *  - -type <type_name>: The name of the type to use for this parameter.
   *  - -addr <offset_address>: (Optional) Relative address of the attribute in
   * the block config space.
   *  - -force: (Optional) If present, any existing parameter with the same name
   * will be overwritten.
   *
   * Example command: define_param -block GEARBOX -name P1 -addr 0x0 -type
   * integer
   *
   * @return True if the parameter was successfully defined, false otherwise.
   *
   * @throws std::invalid_argument If a necessary argument is missing, or if an
   * argument has an invalid value.
   * @throws std::runtime_error If there is an error during the creation of the
   * parameter.
   */
  bool define_param(int argc, const char **argv) {
    // Check at least two parameters (command name and -name <par_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_param.");
    }
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string par_name = get_argument_value("-name", argc, argv, true);
    std::string width = get_argument_value("-width", argc, argv);
    std::string type_name = get_argument_value("-type", argc, argv, true);
    std::string addr = get_argument_value("-addr", argc, argv);
    // block_name is optional. If it's empty, use current_device scope
    device_block *block;
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }
    if (!block) {
      {
        throw std::runtime_error("In the definition of Parameter " + par_name +
                                 ", could not find block " + block_name);
      }
    }
    if (type_name == "int") {
      auto tp = current_device_->get_int_parameter_type("int");
      auto par_t = std::make_shared<Parameter<int>>(par_name, 0, tp);
      if (!width.empty()) {
        unsigned size = (unsigned)(convert_string_to_integer(width));
        par_t->set_size(size);
      }
      if (!addr.empty()) {
        unsigned addr_num = (unsigned)(convert_string_to_integer(addr));
        par_t->set_address(addr_num);
      }
      block->add_int_parameter(par_name, par_t);
      return true;
    } else if (type_name == "double") {
      auto tp = current_device_->get_double_parameter_type("double");
      block->add_double_parameter(
          par_name, std::make_shared<Parameter<double>>(par_name, 0, tp));
      return true;
    } else if (type_name == "string") {
      auto tp = current_device_->get_string_parameter_type("string");
      block->add_string_parameter(
          par_name, std::make_shared<Parameter<std::string>>(par_name, "", tp));
      return true;
    }
    auto tpint = block->get_int_parameter_type(type_name);
    if (tpint.get()) {
      auto par_t = std::make_shared<Parameter<int>>(par_name, 0, tpint);
      if (!addr.empty()) {
        unsigned addr_num = convert_string_to_integer(addr);
        par_t->set_address(addr_num);
      }
      block->add_int_parameter(par_name, par_t);
      return true;
    }
    auto tpdouble = block->get_double_parameter_type(type_name);
    if (tpdouble.get()) {
      block->add_double_parameter(par_name, std::make_shared<Parameter<double>>(
                                                par_name, 0.0, tpdouble));
      return true;
    }
    auto tpstring = block->get_string_parameter_type(type_name);
    if (tpstring.get()) {
      block->add_string_parameter(
          par_name,
          std::make_shared<Parameter<std::string>>(par_name, "", tpstring));
      return true;
    }
    // error out if something is invalid
    return false;
  }

  /**
   * @brief Defines a new attribute.
   *
   * This function uses the provided arguments to define a new attribute with
   * specified properties. The attribute can later be used in the context of the
   * block in which it was defined.
   *
   * @param argc The number of arguments provided.
   * @param argv An array of strings containing the arguments. The expected
   * arguments are:
   * - -block <block_name>: (Optional) The name of a defined block to insert
   * this attribute in, if None passed we use the current_device scope.
   * - -name <attr_name>: The name of the attribute being defined.
   * - -width <nb_bits>: The bit-width of the attribute.
   * - -enum <enum_values>: (Optional) Enumeration values for the attribute.
   * - -enumname <enum_name>: The name of the enumeration.
   * - -addr <address>: The memory address of the attribute.
   * - -upper_bound <u_bound>: (Optional) The upper bound for the attribute
   * value.
   * - -force: (Optional) If present, any existing attribute with the same name
   * will be overwritten.
   *
   * @return True if the attribute was successfully defined, false otherwise.
   *
   * @throws std::invalid_argument If a necessary argument is missing, or if an
   * argument has an invalid value.
   * @throws std::runtime_error If there is an error during the creation of the
   * attribute, such as if the block specified does not exist or if the enum
   * type specified cannot be found or defined.
   */
  bool define_attr(int argc, const char **argv, device_block *bl = nullptr) {
    // Check at least two parameters (command name and -name <par_name>)
    if (argc < 4) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_attr.");
    }
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string attr_name = get_argument_value("-name", argc, argv, true);
    std::string width = get_argument_value("-width", argc, argv,
                                           true);  // must have width defined
    std::string enums = get_argument_values("-enum", argc, argv);
    std::string enum_name = get_argument_value("-enumname", argc, argv);
    std::string addr = get_argument_value("-addr", argc, argv, true);
    std::string u_bound = get_argument_value("-upper_bound", argc, argv);
    std::string default_value = get_argument_value("-default", argc, argv);
    if (enum_name.empty()) {
      enum_name = attr_name + "_ENUM";
    }
    // block_name is optional. If it's empty, use current_device scope
    device_block *block = bl;
    if (!block) {
      if (block_name.empty()) {
        block = current_device_.get();
      } else {
        block = current_device_->get_block(block_name).get();
      }
    }
    if (!block) {
      throw std::runtime_error("In the definition of Attribute " + attr_name +
                               ", could not find block " + block_name);
    }
    if (block->get_attribute(attr_name, true) != nullptr) {
      // Do not allow duplicated attribution definition
      throw std::runtime_error("In the definition of Attribute " + attr_name +
                               ", found duplication attribute in block " +
                               block_name);
    }
    if (block->get_enum_type(enum_name, true) != nullptr) {
      // Do not allow duplicated enum definition
      throw std::runtime_error("In the definition of Attribute " + attr_name +
                               ", found duplication enumname " + enum_name +
                               " in block " + block_name);
    }
    if ("" == width) {
      throw std::runtime_error("In the definition of Attribute " + attr_name +
                               ", width input is empty");
    }
    int size = convert_string_to_integer(width);
    if (size <= 0 || size > 32) {
      throw std::runtime_error("Illegal size (" + width +
                               ") when defining atttibute " + attr_name);
    }
    // Create the new enum type
    auto type = make_shared<ParameterType<int>>();
    type->set_size(size);
    std::unordered_map<std::string, int> values;
    if (enums != "") {
      values = parse_values(enums);
      if (values.size() == 0) {
        throw std::runtime_error("Fail to parse enum input (" + enums +
                                 ") when defining atttibute " + attr_name);
      }
      for (auto &p : values) {
        type->set_enum_value(p.first, p.second);
      }
    }
    // If enums is defined, then default value can be ENUM or INTEGER
    if (default_value != "") {
      if (values.size() > 0 && values.find(default_value) != values.end()) {
        type->set_default_value(values[default_value]);
      } else {
        uint32_t dv = (uint32_t)(convert_string_to_integer(default_value));
        if (size != 32 && (dv >= ((uint32_t)(1) << (uint32_t)(size)))) {
          throw std::runtime_error("The value " + default_value +
                                   " can not fit within " + width + " bits");
        }
        type->set_default_value(int(dv));
      }
    } else {
      // Commenting out the warning.
      // std::cerr << "Setting, missing, default vlaue to ZERO for attribute
      // type "
      //           << attr_name << std::endl;
      type->set_default_value(0);
    }
    if ("" != u_bound) {
      int upper = convert_string_to_integer(u_bound);
      type->set_upper_bound(upper);
    }
    block->add_enum_type(enum_name, type);
    auto attr = std::make_shared<Parameter<int>>(attr_name, 0, type);
    if (!addr.empty()) {
      int address = convert_string_to_integer(addr);
      attr->set_address(address);
    }
    block->add_attribute(attr_name, attr);
    return true;
  }

  /**
   * @brief Defines a new constraint.
   *
   * This function uses the provided arguments to define a new constraint with a
   * specified name and constraint expression. The constraint can later be used
   * in the context of the block in which it was defined.
   *
   * @param argc The number of arguments provided.
   * @param argv An array of strings containing the arguments. The expected
   * arguments are:
   * - -block <block_name>: (Optional) The name of a defined block to insert
   * this constraint in, if None passed we use the current_device scope.
   * - -name <constraint_name>: (Optional) The name of the constraint being
   * defined. If none is provided, a default name is generated.
   * - -constraint <constraint>: The constraint expression to be defined.
   *
   * @return True if the constraint was successfully defined, false otherwise.
   *
   * @throws std::invalid_argument If a necessary argument is missing, or if an
   * argument has an invalid value.
   * @throws std::runtime_error If there is an error during the creation of the
   * constraint, such as if the block specified does not exist or if no device
   * is currently defined.
   */
  bool define_constraint(int argc, const char **argv) {
    // Check at least two parameters (command name and -name <par_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_constraint.");
    }
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string contraint_name = get_argument_value("-name", argc, argv);
    std::string contraint = get_argument_value("-constraint", argc, argv, true);
    device_block *block;
    if (!current_device_.get()) {
      throw std::runtime_error(
          "Need to define a device before calling \"define_constraint\"");
    }
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }
    std::string nm;
    if ("" == contraint_name) {
      auto constraint_map = block->constraints();
      unsigned idx = constraint_map.size();
      nm = block->block_name() + "_constraint_" + std::to_string(idx);
      while (constraint_map.find(nm) != end(constraint_map)) {
        ++idx;
        nm = block->block_name() + "_constraint_" + std::to_string(idx);
      }
    } else {
      nm = contraint_name;
    }
    block->add_constraint(nm, std::make_shared<rs_expression<int>>(contraint));
    return true;
  }
  /**
   * @brief Creates a new instance of a device block.
   *
   * This function allows the creation of a new instance of a device block. It
   * requires several parameters to define the instance's properties and
   * attributes. The following parameters are supported:
   * - `-block`: The name of the block to create an instance from.
   * - `-parent`: The name of the parent block to which this instance belongs.
   * If not provided, the current device scope is used.
   * - `-name`: The name of the instance to be created.
   * - `-id`: An optional unique identifier for the instance.
   * - `-io_bank`: The IO bank associated with the instance.
   * - `-logic_address`: The logic address associated with the instance.
   * - `-logic_location`: The logic X|Y|Z location of the instance which has
   * priority than logic_location_x|y|z
   * - `-logic_location_x`: The logic X location of the instance.
   * - `-logic_location_y`: The logic Y location of the instance.
   * - `-logic_location_z`: The logic Z location of the instance.
   *
   * After specifying the necessary parameters, this function creates a new
   * instance of the specified block, sets its attributes, and adds it to the
   * appropriate data structures within the device block.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the instance was successfully created, false otherwise.
   * @throws std::invalid_argument If insufficient arguments are provided.
   * @throws std::runtime_error If the block or parent block cannot be found, or
   * if there are issues with converting the provided values to integers.
   */
  bool create_instance(int argc, const char **argv) {
    // Check at least two parameters (command name and -name <par_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to create_instance.");
    }
    std::string block_name = get_argument_value("-block", argc, argv, true);
    std::string parent = get_argument_value("-parent", argc, argv);
    std::string name = get_argument_value("-name", argc, argv, true);
    std::string id = get_argument_value("-id", argc, argv);
    std::string io_bank = get_argument_value("-io_bank", argc, argv);
    std::string logic_address =
        get_argument_value("-logic_address", argc, argv);
    /* Accept a list of "X Y Z" location
        It has higher priority to overwrite -logic_location_[x|y|z] */
    std::string logic_location =
        get_argument_value("-logic_location", argc, argv);
    std::string logic_location_x =
        get_argument_value("-logic_location_x", argc, argv);
    std::string logic_location_y =
        get_argument_value("-logic_location_y", argc, argv);
    std::string logic_location_z =
        get_argument_value("-logic_location_z", argc, argv);

    // block_name is optional. If it's empty, use current_device scope
    auto block = current_device_->get_block(block_name);
    if (!block) {
      {
        throw std::runtime_error("In the definition of Instance " + name +
                                 ", could not find block " + block_name);
      }
    }
    device_block *parent_block = nullptr;
    if (parent.empty()) {
      parent_block = current_device_.get();
    } else {
      parent_block = current_device_->get_block(parent).get();
    }
    if (!parent_block) {
      {
        throw std::runtime_error("In the definition of Instance " + name +
                                 ", could not find parent block " + parent);
      }
    }
    int logic_address_i = -1;
    if ("" != logic_address) {
      logic_address_i = convert_string_to_integer(logic_address);
    }
    int logic_location_x_i = -1;
    int logic_location_y_i = -1;
    int logic_location_z_i = -1;
    if ("" != logic_location) {
      std::vector<std::string> tokens;
      FOEDAG::StringUtils::tokenize(logic_location, " ", tokens);
      if (tokens.size() >= 1) {
        logic_location_x = tokens[0];
      }
      if (tokens.size() >= 2) {
        logic_location_y = tokens[1];
      }
      if (tokens.size() >= 3) {
        logic_location_z = tokens[2];
      }
    }
    if ("" != logic_location_x) {
      logic_location_x_i = convert_string_to_integer(logic_location_x);
    }
    if ("" != logic_location_y) {
      logic_location_y_i = convert_string_to_integer(logic_location_y);
    }
    if ("" != logic_location_z) {
      logic_location_z_i = convert_string_to_integer(logic_location_z);
    }
    parent_block->instance_vector().push_back(
        std::make_shared<device_block_instance>(
            block, parent_block->instance_vector().size(), logic_location_x_i,
            logic_location_y_i, logic_address_i, name, io_bank,
            logic_location_z_i));
    parent_block->add_instance(name, parent_block->instance_vector().back());
    return true;
  }
  /**
   * @brief Maps RTL names to user names.
   *
   * This function allows mapping RTL names to user names. It takes two
   * arguments:
   * - `-user_name`: The user name to be mapped.
   * - `-rtl_name`: The RTL name to be associated with the user name.
   *
   * After specifying the user name and RTL name, this function creates a
   * mapping between them in the device block. This mapping is used to associate
   * user-friendly names with RTL names.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the mapping was successfully created, false otherwise.
   * @throws std::invalid_argument If insufficient arguments are provided.
   */
  bool map_rtl_user_names(int argc, const char **argv) {
    // map_rtl_user_names -user_name CLK_OUT0_DIV -rtl_name pll_POSTDIV0
    // Check at least five parameters (command name and -name <par_name>)
    if (argc < 5) {
      throw std::invalid_argument(
          "Insufficient arguments passed to map_rtl_user_names.");
    }
    std::string user_name = get_argument_value("-user_name", argc, argv, true);
    std::string rtl_name = get_argument_value("-rtl_name", argc, argv, true);
    current_device_->setUserToRtlMapping(user_name, rtl_name);
    return true;
  }
  /**
   * @brief Retrieves the RTL (Register Transfer Level) name associated with a
   * given user name.
   *
   * This function retrieves the RTL name corresponding to a specified user name
   * from the command line arguments. The user name is identified by the
   * `-user_name` argument. If the command line arguments are insufficient, or
   * the user name is missing, appropriate exceptions are thrown.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return The RTL name corresponding to the specified user name.
   * @throws std::invalid_argument If there are not enough command line
   * arguments.
   * @throws std::runtime_error If the specified user name does not correspond
   * to a valid RTL name.
   */
  std::string get_rtl_name(int argc, const char **argv) {
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to get_rtl_name.");
    }
    std::string user_name = get_argument_value("-user_name", argc, argv, true);
    return current_device_->getRtlNameFromUser(user_name);
  }
  /**
   * @brief Maps model names to user names.
   *
   * This function allows mapping model names to user names. It takes two
   * arguments:
   * - `-user_name`: The user name to be mapped.
   * - `-model_name`: The model name to be associated with the user name.
   *
   * After specifying the user name and model name, this function creates a
   * mapping between them in the device block. This mapping is used to associate
   * user-friendly names with model names.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the mapping was successfully created, false otherwise.
   * @throws std::invalid_argument If insufficient arguments are provided.
   * @throws std::runtime_error If the current device is not defined.
   */
  bool map_model_user_names(int argc, const char **argv) {
    // map_model_user_names -user_name USER_NAME -model_name MODEL_NAME
    // Check at least five parameters (command name and -name <par_name>)
    if (argc < 5) {
      throw std::invalid_argument(
          "Insufficient arguments passed to map_model_user_names.");
    }

    std::string user_name = get_argument_value("-user_name", argc, argv, true);
    std::string model_name =
        get_argument_value("-model_name", argc, argv, true);

    if (!current_device_.get()) {
      throw std::runtime_error(
          "Need to define a device before calling \"map_model_user_names\"");
    }
    current_device_->addMapping(model_name, user_name);
    return true;
  }
  /**
   * @brief Retrieves the user name associated with a given model name.
   *
   * This function fetches the user name corresponding to a specified model
   * name. The model name is identified through the `-model_name` argument in
   * the command line. If the command line arguments are insufficient, or if the
   * model name is missing, an exception is thrown.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A string containing the user name associated with the specified
   * model name.
   * @throws std::invalid_argument If there are fewer than 3 command line
   * arguments.
   * @throws std::runtime_error If the specified model name does not correspond
   * to a valid user name.
   */
  std::string get_user_name(int argc, const char **argv) {
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to get_user_name.");
    }
    std::string model_name =
        get_argument_value("-model_name", argc, argv, true);
    return current_device_->getCustomerName(model_name);
  }
  /**
   * @brief Retrieves the model name associated with a given user name.
   *
   * This function fetches the model name corresponding to a specified user
   * name. The user name is retrieved from the `-user_name` argument in the
   * command line. If the command line arguments are insufficient, or if the
   * user name is missing, an exception is thrown.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A string containing the model name associated with the specified
   * user name.
   * @throws std::invalid_argument If there are fewer than 3 command line
   * arguments.
   * @throws std::runtime_error If the specified user name does not correspond
   * to a valid model name.
   */
  std::string get_model_name(int argc, const char **argv) {
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to get_user_name.");
    }
    std::string user_name = get_argument_value("-user_name", argc, argv, true);
    return current_device_->getModelName(user_name);
  }

  /**
   * @brief Define properties for a device block.
   *
   * This function defines a set of properties for a device block. Each property
   * consists of a property name and an associated value. The property names are
   * specified with the -property_name flag followed by the property name
   * itself.
   *
   * @param argc Number of command line arguments.
   * @param argv Array of command line arguments.
   * @return True if the properties were successfully defined, false otherwise.
   * @throws std::invalid_argument if insufficient arguments are passed.
   * @throws std::runtime_error if no device is defined before calling the
   * function.
   */
  bool define_properties(int argc, const char **argv) {
    // Check for at least two parameters (command name and -block <block_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_properties.");
    }
    std::string block_name = get_argument_value("-block", argc, argv, true);
    device_block *block;
    if (!current_device_.get()) {
      throw std::runtime_error(
          "Need to define a device before calling \"define_properties\"");
    }
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }
    // Iterate through the command line arguments to extract property names and
    // values
    for (int i = 1; i < argc - 1; ++i) {
      std::string arg = argv[i];
      if (arg[0] == '-' && i + 1 < argc && argv[i + 1][0] != '-') {
        std::string property_name = arg.substr(1);  // Remove the leading "-"
        std::string property_value = argv[i + 1];

        // Set the property in the block's property map
        block->setProperty(property_name, property_value);
        ++i;  // Skip the property value
      }
    }
    return true;
  }

  /**
   * @brief Define a net in a device block.
   *
   * This function defines a net
   *
   * @param argc Number of command line arguments.
   * @param argv Array of command line arguments.
   * @return True if the net was successfully defined, false otherwise.
   * @throws std::invalid_argument if insufficient arguments are passed.
   * @throws std::runtime_error if no device is defined before calling the
   * function.
   */
  bool define_net(int argc, const char **argv) {
    static int cnt = 0;
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_net.");
    }
    std::string block_name = get_argument_value("-parent", argc, argv, true);
    std::string driver_name = get_argument_value("-drive", argc, argv);
    std::string load_names = get_argument_value("-load", argc, argv);
    std::string net_name = get_argument_value("-name", argc, argv);
    device_block *block = nullptr;

    if (net_name.empty()) {
      stringstream ss;
      ss << cnt++;
      net_name = "__DEFAULT__NET__NAME__" + ss.str();
    }

    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }

    if (!block) {
      throw std::runtime_error("In the definition of net " + net_name +
                               ", could not find block " + block_name);
    }
    auto v = split_string_by_space(load_names);
    block->add_net(std::make_shared<device_net>(net_name));
    auto net_ptr = block->get_net(net_name);
    if (!driver_name.empty()) {
      std::vector<std::string> xmr_refs =
          FOEDAG::StringUtils::tokenize(driver_name, ".", false);
      std::shared_ptr<device_net> drv = nullptr;
      if (xmr_refs.size() == 2) {
        auto ins = block->get_instance(xmr_refs[0]);
        if (ins) drv = ins->get_net(xmr_refs[1]);
      } else {
        drv = block->get_net(driver_name);
      }
      if (drv)
        net_ptr->set_source(drv);
      else
        std::cout << "WANRN: The driver \"" << driver_name << "\" for net \""
                  << net_name << "\" does not exis in block \"" << block_name
                  << "\"" << std::endl;
    }
    for (auto &ld_n : v) {
      std::vector<std::string> xmr_refs =
          FOEDAG::StringUtils::tokenize(ld_n, ".", false);
      std::shared_ptr<device_net> load = nullptr;
      if (xmr_refs.size() == 2) {
        auto ins = block->get_instance(xmr_refs[0]);
        if (ins) load = ins->get_net(xmr_refs[1]);
      } else {
        load = block->get_net(ld_n);
      }
      if (load)
        net_ptr->add_sink(load);
      else
        std::cout << "WANRN: The load \"" << ld_n << "\" for net \"" << net_name
                  << "\" does not exis in block \"" << block_name << "\""
                  << std::endl;
    }
    return true;
  }

  /**
   * @brief Define properties for a device block.
   *
   * This function defines a set of properties for a device block. Each property
   * consists of a property name and an associated value. The property names are
   * specified with the -property_name flag followed by the property name
   * itself.
   *
   * @param argc Number of command line arguments.
   * @param argv Array of command line arguments.
   * @return The string value of the property if defined.
   * @throws std::invalid_argument if insufficient arguments are passed.
   * @throws std::runtime_error if no device is defined before calling the
   * function.
   */
  std::string get_property(int argc, const char **argv) {
    // Check for at least two parameters (command name and -block <block_name>)
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to define_properties.");
    }
    std::string block_name = get_argument_value("-block", argc, argv, true);
    std::string property_name =
        get_argument_value("-property", argc, argv, true);
    device_block *block;
    if (!current_device_.get()) {
      throw std::runtime_error(
          "Need to define a device before calling \"define_properties\"");
    }
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      block = current_device_->get_block(block_name).get();
    }
    std::string res = block->getProperty(property_name);
    return res;
  }

  device *get_device_model(const std::string &name) {
    auto device = get_device(name);
    if (device != nullptr && device.get() != nullptr) {
      return device.get();
    } else {
      return nullptr;
    }
  }
  /**
   * @brief Retrieves the block names for a specified device.
   *
   * This function extracts the names of all blocks from a specific device. If a
   * device name is provided through the command line arguments, it will attempt
   * to retrieve the corresponding device. If no name is provided, it uses the
   * current device. In the event that the named device doesn't exist, an
   * exception is thrown.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of blocks within the
   * identified device.
   * @throws std::runtime_error If the device specified by name is not found.
   *
   * @note The function expects a "-device" argument in `argv` that specifies
   * the device name. If this argument is missing or empty, it uses the current
   * device. The function returns the names of all blocks associated with the
   * identified device.
   */
  std::vector<std::string> get_block_names(int argc, const char **argv) {
    std::string device_name = get_argument_value("-device", argc, argv);
    device_block *device;
    if (device_name.empty()) {
      device = current_device_.get();
    } else {
      if (get_device(device_name))
        device = get_device(device_name).get();
      else {
        std::string err = "Could not find device named " + device_name;
        throw std::runtime_error(err.c_str());
      }
    }
    std::vector<std::string> ret;
    for (const auto &b : device->blocks()) {
      ret.push_back(b.first);
    }
    return ret;
  }

  /**
   * @brief Retrieves a list of port names for a specified block, optionally
   * filtered by direction.
   *
   * This function retrieves the list of port names from a specific block within
   * a device. The block is identified by the `-block` argument in the command
   * line. Optionally, the list can be filtered by port direction, which is
   * specified by the `-dir` argument, with possible values "in" or "out".
   *
   * If no direction is specified, all port names from the block are returned.
   * If the required arguments are not provided, or the specified block does not
   * exist, appropriate exceptions are thrown.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of ports within the
   * specified block, optionally filtered by direction.
   * @throws std::invalid_argument If there are not enough command line
   * arguments.
   * @throws std::runtime_error If the block name is not provided or if the
   * specified block does not exist.
   *
   * @note The function expects at least 3 arguments: the block name and the
   * optional direction
   *       ("-dir"). If the block name is empty, a runtime error is thrown.
   */
  std::vector<std::string> get_port_list(int argc, const char **argv) {
    if (argc < 3) {
      throw std::invalid_argument(
          "Insufficient arguments passed to get_port_list.");
    }
    std::string block_name = get_argument_value("-block", argc, argv, true);
    std::string dir = get_argument_value("-dir", argc, argv);
    device_block *block;
    if (block_name.empty()) {
      throw std::runtime_error("Need to define a block in  \"get_port_list\"");
    } else {
      block = current_device_->get_block(block_name).get();
    }
    std::vector<std::string> ret;
    if (!block) {
      std::string err =
          "Block " + block_name + " does not exist for get_port_list.";
      throw std::runtime_error(err.c_str());
    }
    for (auto &p : block->ports()) {
      if (dir.empty()) {
        ret.push_back(p.first);
      } else if ("in" == dir) {
        if (p.second->is_input()) {
          ret.push_back(p.first);
        }
      } else {
        if (!p.second->is_input()) {
          ret.push_back(p.first);
        }
      }
    }
    return ret;
  }

  /**
   * @brief Retrieves the names of instances in a specified block.
   *
   * This function returns a list of instance names within a specified block in
   * a device. The block is identified using the `-block` command line argument.
   * If the argument is missing or empty, the current device is used as the
   * block.
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of instances in the
   * specified block.
   * @throws std::runtime_error If the block specified by name does not exist.
   *
   * @example
   * ```
   * const char* argv[] = {"program", "-block", "BlockX"};
   * std::vector<std::string> instance_names = get_instance_names(3, argv);
   * for (const auto &name : instance_names) {
   *   std::cout << name << std::endl;
   * }
   * ```
   */
  std::vector<std::string> get_instance_names(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv);
    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // Vector to store instance names
    std::vector<std::string> ret;
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    // Add instance names to the vector
    for (auto &p : block->instances()) {
      ret.push_back(p.first);
    }
    // Return the vector of instance names
    return ret;
  }

  /**
   * @brief Retrieves the names of instance chains based on command-line
   * arguments.
   *
   * This function processes the command-line arguments to retrieve the names of
   * instance chains. It validates the number of arguments, retrieves the values
   * for block and device names, and then returns a vector containing the names
   * of the instance chains for the specified or current device and block.
   *
   * @param argc The number of command-line arguments.
   * @param argv The array of command-line argument strings.
   * @return A vector of strings containing the names of the instance chains.
   * @throws std::runtime_error if the number of arguments is less than
   * required, or if the specified device is not found.
   */
  std::vector<std::string> get_instance_chains_names(int argc,
                                                     const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 1 argument (block and or device) for command "
          "get_instance_chains_names");
    }

    // Retrieve the value of the block name from the command-line arguments.
    std::string block_name = get_argument_value("-block", argc, argv);
    // Retrieve the value of the device name from the command-line arguments.
    std::string device_name = get_argument_value("-device", argc, argv);

    // Get the current device or the specified device.
    device_block *device = current_device_.get();
    if (!device_name.empty()) {
      device = get_device(device_name).get();
    }

    // Check if the device is valid.
    if (!device) {
      throw std::runtime_error(
          "No device found in the command get_instance_chains_names");
    }

    // Get the block from the device, if specified.
    device_block *block = nullptr;
    if (!block_name.empty()) {
      block = device->get_block(block_name).get();
    }

    // If no block is specified, use the device itself as the block.
    if (!block) {
      block = device;
    }

    // Vector to store instance chain names.
    std::vector<std::string> ret;

    // If the block doesn't exist, return an empty vector.
    if (!block) {
      return ret;
    }

    // Add instance chain names to the vector.
    for (auto &p : block->get_chains()) {
      ret.push_back(p.first);
    }

    // Return the vector of instance chain names.
    return ret;
  }

  /**
   * @brief Retrieves the names of instances in a specified chain based on
   * command-line arguments.
   *
   * This function processes the command-line arguments to retrieve the names of
   * instances in a specified chain. It validates the number of arguments,
   * retrieves the values for chain, block, and device names, and then returns a
   * vector containing the names of the instances in the specified chain for the
   * specified or current device and block.
   *
   * @param argc The number of command-line arguments.
   * @param argv The array of command-line argument strings.
   * @return A vector of strings containing the names of the instances in the
   * chain.
   * @throws std::runtime_error if the number of arguments is less than
   * required, if no chain name is specified, or if the specified device is not
   * found.
   */
  std::vector<std::string> get_instance_chain(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 1 argument (block and or device) for command "
          "get_instance_chain");
    }

    // Retrieve the value of the chain name from the command-line arguments.
    std::string chain_name = get_argument_value("-chain", argc, argv, true);
    // Retrieve the value of the block name from the command-line arguments.
    std::string block_name = get_argument_value("-block", argc, argv);
    // Retrieve the value of the device name from the command-line arguments.
    std::string device_name = get_argument_value("-device", argc, argv);

    // Get the current device or the specified device.
    device_block *device = current_device_.get();
    if (!device_name.empty()) {
      device = get_device(device_name).get();
    }

    // Check if the device is valid.
    if (!device) {
      throw std::runtime_error(
          "No device found in the command get_instance_chain");
    }

    // Get the block from the device, if specified.
    device_block *block = nullptr;
    if (!block_name.empty()) {
      block = device->get_block(block_name).get();
    }

    // If no block is specified, use the device itself as the block.
    if (!block) {
      block = device;
    }

    // Vector to store instance names.
    std::vector<std::string> ret;

    // If the block doesn't exist, return an empty vector.
    if (!block) {
      return ret;
    }

    // Add instance names to the vector from the specified chain.
    for (auto &p : block->get_chain(chain_name)) {
      ret.push_back(p);
    }

    // Return the vector of instance names.
    return ret;
  }

  /**
   * @brief Retrieves the names of attributes in a specified block.
   *
   * This function returns a list of attribute names for a specified block in a
   device. The block
   * name is specified via the `-block` command line argument. If this argument
   is empty or missing,
   * the function defaults to the current device. If the specified block does
   not exist, an empty
   * vector is returned.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of attributes in the
   specified block.
   * @throws std::runtime_error If the block specified by name does not exist.
   *
   * @example
   * ```
   * const char* argv[] = {"program", "-block", "BlockY"};
   * std::vector<std::string> attribute_names = get_attributes(2, argv);
   * for (const auto &name : attribute_names) {
   *   std::cout << name << std::endl;
   * }
   * ```
   * */
  std::vector<std::string> get_attributes(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv);
    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // Vector to store instance names
    std::vector<std::string> ret;
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    // Add attribute names to the vector
    for (auto &p : block->attributes()) {
      ret.push_back(p.first);
    }
    return ret;
  }

  /**
   * @brief Retrieves the names of parameters in a specified block, optionally
   * filtered by type.
   *
   * This function retrieves a list of parameter names from a specified block in
   * a device. The block name is provided via the `-block` command line
   * argument. If this argument is empty, the current device is used as the
   * block. The list can be filtered by parameter type, specified by the
   * `-base_type` argument, which can be "int", "double", or "string".
   *
   * If the block does not exist, the function returns an empty vector.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of parameters in the
   * specified block, optionally filtered by parameter type.
   * @throws std::runtime_error If the specified block does not exist.
   *
   * @example
   * ```
   * const char* argv[] = {"program", "-block", "BlockZ", "-base_type", "int"};
   * std::vector<std::string> parameter_names = get_parameters(4, argv);
   * for (const auto &name : parameter_names) {
   *   std::cout << name << std::endl;
   * }
   * ```
   */
  std::vector<std::string> get_parameters(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string base_type = get_argument_value("-base_type", argc, argv);

    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // Vector to store instance names
    std::vector<std::string> ret;
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    // Add parameter names to the vector
    if (base_type.empty() || "int" == base_type) {
      for (auto &p : block->int_parameters()) {
        ret.push_back(p.first);
      }
    }
    if (base_type.empty() || "double" == base_type) {
      for (auto &p : block->double_parameters()) {
        ret.push_back(p.first);
      }
    }
    if (base_type.empty() || "string" == base_type) {
      for (auto &p : block->string_parameters()) {
        ret.push_back(p.first);
      }
    }
    return ret;
  }

  std::vector<std::string> get_parameter_types(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv);
    std::string base_type = get_argument_value("-base_type", argc, argv);

    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // Vector to store instance names
    std::vector<std::string> ret;
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    // Add parameter type names to the vector
    if (base_type.empty() || "int" == base_type) {
      for (auto &p : block->int_parameter_types()) {
        ret.push_back(p.first);
      }
    }
    // Add parameter type names to the vector
    if (base_type.empty() || "double" == base_type) {
      for (auto &p : block->double_parameter_types()) {
        ret.push_back(p.first);
      }
    }
    // Add parameter type names to the vector
    if (base_type.empty() || "string" == base_type) {
      for (auto &p : block->string_parameter_types()) {
        ret.push_back(p.first);
      }
    }
    return ret;
  }
  /**
   * @brief Retrieves the names of constraints in a specified block.
   *
   * This function returns a list of constraint names for a given block in a
   * device. The block name is specified via the `-block` command line argument.
   * If no block name is provided, the function defaults to the current device.
   * If the specified block does not exist, an empty vector is returned.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A vector of strings containing the names of constraints in the
   * specified block.
   * @throws std::runtime_error If the block specified by name does not exist.
   */
  std::vector<std::string> get_constraint_names(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv);
    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // Vector to store instance names
    std::vector<std::string> ret;
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    // Add constraint names to the vector
    for (auto &p : block->constraints()) {
      ret.push_back(p.first);
    }
    return ret;
  }

  /**
   * @brief Retrieves a constraint's expression by its name within a specified
   * block.
   *
   * This function fetches the expression string for a specific constraint in a
   * given block. The block name is obtained from the `-block` command line
   * argument, and the constraint name is obtained from the `-name` argument. If
   * the block or constraint name is not provided, an invalid argument exception
   * is thrown. If the specified block does not exist, a default constraint
   * expression is returned.
   *
   * @param argc The count of command line arguments.
   * @param argv Array of command line arguments.
   * @return A string containing the expression of the specified constraint in
   * the identified block.
   * @throws std::invalid_argument If the block name or constraint name is not
   * provided.
   * @throws std::runtime_error If the specified block or constraint does not
   * exist.
   */
  std::string get_constraint_by_name(int argc, const char **argv) {
    // Retrieve the block name from command line arguments
    std::string block_name = get_argument_value("-block", argc, argv, true);
    std::string const_name = get_argument_value("-name", argc, argv, true);
    // Pointer to hold the identified block
    device_block *block;
    // If no block name is provided, use the current device
    if (block_name.empty()) {
      block = current_device_.get();
    } else {
      // If a block name is provided, retrieve the corresponding block
      block = current_device_->get_block(block_name).get();
    }
    // String, a default string holder for a costraint description
    std::string ret = "__ANY_CONSTRAINT__ ";
    // If the block doesn't exist, return an empty vector
    if (!block) {
      return ret;
    }
    if (block->constraints().find(const_name) != end(block->constraints())) {
      ret = block->constraints()[const_name]->get_expression_string();
    }
    return ret;
  }

  /**
   * @brief Retrieve the block type of a given instance by its hierarchical
   * name.
   *
   * This function finds the type of a block in a device's instance tree given a
   * set of command-line arguments. The first argument specifies the device or
   * block name, and the subsequent arguments specify the hierarchical path to
   * the instance.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return A string representing the block type of the specified instance.
   * @throws std::runtime_error if there are insufficient arguments or the
   * instance cannot be found.
   *
   * @details
   * This function retrieves the block type for a specific instance within a
   * device or block, based on a hierarchical instance name provided via
   * command-line arguments. The expected arguments are as follows:
   * - At least three arguments, where the first is a command identifier, the
   * second is a flag (e.g., "-name"), and the third is the hierarchical
   * instance name.
   *
   * The instance name is a dot-separated string representing a hierarchy of
   * device and instance names. The first component represents a device name, or
   * a block/instance within the current device. Subsequent components represent
   * deeper instances within the instance tree.
   *
   * The function first checks whether there are enough arguments. If not, it
   * throws a runtime error. It then extracts the instance name and splits it
   * into its hierarchical components.
   *
   * Depending on the first part of the instance name, the function finds the
   * relevant device block or instance. If it does not find a root block or
   * instance, it throws an error. If the hierarchy path does not lead to a
   * valid instance, it throws an error. If the instance is found, the function
   * returns the name of the block it refers to.
   */
  std::string get_instance_block_type(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 2 arguments for command get_instance_block_type");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-name", argc, argv);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command get_instance_block_type");
    }
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    return inst->get_block()->block_name();
  }

  /**
   * @brief Retrieves the IO bank name for a specified instance.
   *
   * This function retrieves the IO (Input/Output) bank for a device block
   * instance identified by a hierarchical instance name. It requires
   * command-line arguments specifying the instance name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return A string representing the name of the IO bank.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name is empty, or if the specified
   * instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   *
   * If the number of command-line arguments is less than 3, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It retrieves the instance name from the arguments.
   *
   * If the instance name is empty, the function throws a runtime error. The
   * function then finds the specified instance using its hierarchical name. If
   * the instance is not found, a runtime error is thrown. If the instance is
   * found, the function retrieves and returns the name of the IO bank.
   */
  std::string get_io_bank(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 3 arguments for command get_io_bank");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error("Instance name needed for command get_io_bank");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Return the name of the IO bank for the found instance.
    return inst->get_io_bank();
  }

  /**
   * @brief Sets the IO bank for a specified instance.
   *
   * This function sets the IO (Input/Output) bank for a device block instance
   * identified by a hierarchical instance name. It requires command-line
   * arguments specifying the instance name and the IO bank name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the IO bank was successfully set, false otherwise.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name or IO bank name is empty, or if the
   * specified instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   * - A flag for the IO bank name (e.g., "-io_bank"), with the corresponding
   * name.
   *
   * If the number of command-line arguments is less than 5, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It retrieves the instance name and the IO bank name from the
   * arguments.
   *
   * If the instance name is empty, the function throws a runtime error. If the
   * IO bank name is empty, a runtime error is also thrown. The function then
   * finds the specified instance using its hierarchical name. If the instance
   * is not found, a runtime error is thrown. If the instance is found, the
   * function sets the specified IO bank for the instance.
   */
  bool set_io_bank(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 5 arguments for command set_io_bank");
    }

    // Get the instance name and IO bank name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);
    std::string io_bank = get_argument_value("-io_bank", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error("Instance name needed for command set_io_bank");
    }

    // Ensure the IO bank name is not empty.
    if (io_bank.empty()) {
      throw std::runtime_error("IO bank name needed for command set_io_bank");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Set the IO bank for the found instance.
    inst->set_io_bank(io_bank);

    return true;
  }

  /**
   * @brief Retrieves the logical (x, y, z) location of a specified instance.
   *
   * This function retrieves the logical location in terms of x, y, and z
   * coordinates for a device block instance identified by a hierarchical
   * instance name. It requires command-line arguments specifying the instance
   * name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return A vector of three integers representing the x, y, and z coordinates
   * of the logical location.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name is empty, or if the specified
   * instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   *
   * If the number of command-line arguments is less than 3, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It then retrieves the instance name from the arguments. If the
   * instance name is empty, the function throws a runtime error.
   *
   * The function finds the specified instance using its hierarchical name and
   * verifies its existence. If the instance is not found, a runtime error is
   * thrown. If the instance is found, the function retrieves its logical
   * location coordinates (x, y, z) and returns them as a vector of integers.
   */
  std::vector<int> get_logic_location(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 3 arguments for command get_logic_location");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command get_logic_location");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Create a vector to hold the x, y, and z logical location coordinates.
    std::vector<int> ret;

    // Retrieve and add the x, y, and z coordinates to the vector.
    ret.push_back(inst->get_logic_location_x());
    ret.push_back(inst->get_logic_location_y());
    ret.push_back(inst->get_logic_location_z());

    // Return the vector containing the logical location.
    return ret;
  }

  /**
   * @brief Sets the logical location of a specified instance.
   *
   * This function sets the logical (x, y, z) location for a device block
   * instance identified by a hierarchical instance name. It requires
   * command-line arguments specifying the instance name and at least one axis
   * value (-x, -y, or -z).
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the logical location was successfully set, false otherwise.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name is empty, or if no axis values are
   * provided, or if the specified instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   * - Flags for the x, y, and z coordinates (e.g., "-x", "-y", "-z"), with the
   * corresponding values.
   *
   * If the number of command-line arguments is less than 5, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It then retrieves the instance name and the x, y, z coordinates
   * from the arguments.
   *
   * If the instance name is empty, the function throws a runtime error. If none
   * of the axis values
   * (-x, -y, -z) are provided, a runtime error is also thrown. The function
   * converts the x, y, and z coordinate strings to integers and initializes
   * them with `INT32_MIN` to indicate unset values.
   *
   * The function finds the specified instance from its hierarchical name. If
   * the instance is not found, a runtime error is thrown. If the instance is
   * found, the function sets the logical location based on the given axis
   * values. Only the non-empty axis values are set. If successful, the function
   * returns true.
   */
  bool set_logic_location(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 5 arguments for command set_logic_location");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);
    std::string axis_x = get_argument_value("-x", argc, argv);
    std::string axis_y = get_argument_value("-y", argc, argv);
    std::string axis_z = get_argument_value("-z", argc, argv);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command set_logic_location");
    }

    // Ensure at least one axis value is provided.
    if (axis_x.empty() && axis_y.empty() && axis_z.empty()) {
      throw std::runtime_error(
          "At least one of (-x, -y, or -z) must be provided for command "
          "set_logic_location");
    }

    // Convert the axis strings to integers, initializing with INT32_MIN to
    // represent unset values.
    int loc_x = INT32_MIN;
    int loc_y = INT32_MIN;
    int loc_z = INT32_MIN;

    if (!axis_x.empty()) {
      loc_x = convert_string_to_integer(axis_x);
    }

    if (!axis_y.empty()) {
      loc_y = convert_string_to_integer(axis_y);
    }

    if (!axis_z.empty()) {
      loc_z = convert_string_to_integer(axis_z);
    }

    // Find the specified instance from its hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Set the logical location for each non-empty axis.
    if (loc_x > INT32_MIN) {
      inst->set_logic_location_x(loc_x);
    }

    if (loc_y > INT32_MIN) {
      inst->set_logic_location_y(loc_y);
    }

    if (loc_z > INT32_MIN) {
      inst->set_logic_location_z(loc_z);
    }

    return true;
  }

  /**
   * @brief Retrieves the physical address of a specified instance.
   *
   * This function retrieves the physical (phy) address of a device block
   * instance identified by a hierarchical instance name. The function requires
   * command-line arguments specifying the instance name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return The physical address of the specified instance.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance cannot be found or is invalid.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   *
   * If the number of command-line arguments is less than 3, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It then retrieves the instance name from the arguments. If the
   * instance name is empty, the function throws a runtime error.
   *
   * The function finds the specified instance using its hierarchical name and
   * verifies its existence. If the instance is not found, a runtime error is
   * thrown. If the instance is found, the function retrieves and returns its
   * physical address.
   */
  int get_phy_address(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 3 arguments for command get_phy_address");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command get_phy_address");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Return the physical address of the found instance.
    return inst->get_phy_address();
  }

  /**
   * @brief Sets the physical address of a specified instance.
   *
   * This function sets the physical (phy) address for a device block instance
   * identified by a hierarchical instance name. The function requires
   * command-line arguments specifying the instance and the desired physical
   * address.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the physical address was successfully set, false otherwise.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance or physical address cannot be found or is
   * invalid.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   * - A flag for the physical address (e.g., "-address"), followed by the
   * desired physical address.
   *
   * If the number of command-line arguments is less than 5, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It then retrieves the instance name and physical address from the
   * arguments. If either is empty, a runtime error is raised.
   *
   * The function finds the specified instance using its hierarchical name and
   * verifies its existence. If the instance is not found, it throws a runtime
   * error. If the instance is found, the function converts the provided
   * physical address from a string to an integer and sets the physical address
   * for the instance. If successful, the function returns true.
   */
  bool set_phy_address(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 5 arguments for command set_phy_address");
    }

    // Get the instance name and physical address from the command-line
    // arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);
    std::string phy_address = get_argument_value("-address", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command set_phy_address");
    }

    // Ensure the physical address is not empty.
    if (phy_address.empty()) {
      throw std::runtime_error(
          "Physical address needed for command set_phy_address");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Convert the physical address from string to integer and set it for the
    // instance.
    int addr_i = convert_string_to_integer(phy_address);
    inst->set_phy_address(addr_i);

    return true;
  }

  /**
   * @brief Retrieves the logical address of a specified instance.
   *
   * This function retrieves the logical address for a device block instance
   * identified by a hierarchical instance name. It requires command-line
   * arguments specifying the instance name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return An integer representing the logical address of the specified
   * instance.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name is empty, or if the specified
   * instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   *
   * If the number of command-line arguments is less than 3, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It retrieves the instance name from the arguments.
   *
   * If the instance name is empty, the function throws a runtime error. The
   * function then finds the specified instance using its hierarchical name. If
   * the instance is not found, a runtime error is thrown. If the instance is
   * found, the function retrieves and returns its logical address.
   */
  int get_logic_address(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 3 arguments for command get_logic_address");
    }

    // Get the instance name from the command-line arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command get_logic_address");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Return the logical address of the found instance.
    return inst->get_logic_address();
  }

  /**
   * @brief Sets the logical address for a specified instance.
   *
   * This function sets the logical address for a device block instance
   * identified by a hierarchical instance name. It requires command-line
   * arguments specifying the instance name and the logical address.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the logical address was successfully set, false otherwise.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the instance name or logical address is empty, or if
   * the specified instance cannot be found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the instance name (e.g., "-inst"), followed by the
   * hierarchical instance name.
   * - A flag for the logical address (e.g., "-address"), with the corresponding
   * value.
   *
   * If the number of command-line arguments is less than 5, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It retrieves the instance name and the logical address from the
   * arguments.
   *
   * If the instance name is empty, the function throws a runtime error. If the
   * logical address is empty, a runtime error is also thrown. The function then
   * finds the specified instance using its hierarchical name. If the instance
   * is not found, a runtime error is thrown. If the instance is found, the
   * function converts the logical address from a string to an integer and sets
   * it for the instance.
   */
  bool set_logic_address(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 5 arguments for command set_logic_address");
    }

    // Get the instance name and logical address from the command-line
    // arguments.
    std::string instance_name = get_argument_value("-inst", argc, argv, true);
    std::string logic_address =
        get_argument_value("-address", argc, argv, true);

    // Ensure the instance name is not empty.
    if (instance_name.empty()) {
      throw std::runtime_error(
          "Instance name needed for command set_logic_address");
    }

    // Ensure the logical address is not empty.
    if (logic_address.empty()) {
      throw std::runtime_error(
          "Logical address needed for command set_logic_address");
    }

    // Find the specified instance from the hierarchical name.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(instance_name);

    if (!inst) {
      std::string err = "Could not find instance " + instance_name;
      throw std::runtime_error(err.c_str());
    }

    // Convert the logical address from string to integer and set it for the
    // instance.
    int addr_i = convert_string_to_integer(logic_address);
    inst->set_logic_address(addr_i);

    return true;
  }

  /**
   * @brief Defines a new block chain in a specified device.
   *
   * This function creates a new block chain in a specified device or in the
   * current device if no device name is provided. It requires command-line
   * arguments specifying the chain name and optionally the device name.
   *
   * @param argc The number of command-line arguments.
   * @param argv An array of command-line arguments.
   * @return True if the block chain was successfully created, false otherwise.
   * @throws std::runtime_error if the number of command-line arguments is
   * insufficient, or if the specified device is not found.
   *
   * @details
   * The command-line arguments must include:
   * - A flag for the chain name (e.g., "-name"), followed by the name of the
   * block chain.
   * - An optional flag for the device name (e.g., "-device").
   *
   * If the number of command-line arguments is less than 3, the function throws
   * a runtime error, indicating that there are insufficient arguments for the
   * command. It retrieves the chain name and device name from the arguments.
   *
   * The function identifies the device in which the block chain should be
   * created. If the device name is empty, it uses the current device. If a
   * specific device name is provided, it retrieves that device. If the device
   * is not found, a runtime error is thrown.
   *
   * Once the device is identified, the function creates a new block chain with
   * the specified name. If successful, it returns true.
   */
  bool define_chain(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 3) {
      throw std::runtime_error(
          "Need at least 3 arguments for command define_chain");
    }

    // Get the chain name and optional device name from the command-line
    // arguments.
    std::string chaine_name = get_argument_value("-name", argc, argv, true);
    std::string device_name = get_argument_value("-device", argc, argv);

    // Identify the device in which to create the block chain.
    device_block *device = nullptr;

    if (device_name.empty()) {
      device = current_device_.get();
    } else {
      device = get_device(device_name).get();
    }

    // Ensure a valid device is identified.
    if (!device) {
      std::string err = "No device found: " + device_name;
      throw std::runtime_error(err.c_str());
    }

    // Create the block chain in the specified device.
    device->createBlockChain(chaine_name);

    return true;
  }
  /**
   * @brief Adds a specified block to a given chain in a device.
   *
   * This function adds a block to a specified chain within a given device. The
   * chain name and block name are provided through command-line arguments. An
   * optional device name can be specified. If the chain or block name is not
   * provided, or if the block or device does not exist, an exception is thrown.
   *
   * @param argc The count of command-line arguments.
   * @param argv Array of command-line arguments.
   * @return `true` if the block was successfully added to the specified chain,
   * `false` if the chain does not exist.
   * @throws std::runtime_error If the number of command-line arguments is fewer
   * than 5, if the chain name is missing, if the device or block does not
   * exist, or if the specified block or chain cannot be located.
   *
   * @example
   * ```
   * const char* argv[] = {"program", "-chain", "Chain1", "-block", "BlockA",
   * "-device", "DeviceX"}; bool success = add_block_to_chain_type(6, argv); if
   * (success) { std::cout << "Block added to chain successfully." << std::endl;
   * } else {
   *   std::cout << "Chain does not exist." << std::endl;
   * }
   * ```
   * This example demonstrates how to add a block "BlockA" to a specified chain
   * "Chain1" in the device "DeviceX".
   *
   * @details
   * The function operates as follows:
   * - Validates that there are at least 5 command-line arguments.
   * - Retrieves the chain name and block name from the command-line arguments.
   * - Retrieves the device name, if specified.
   * - If the chain name is empty, throws a runtime error.
   * - Identifies the appropriate device; if no device name is provided, uses
   * the current device.
   * - If the device does not exist, throws a runtime error.
   * - Retrieves the specified block from the identified device; if the block
   * does not exist, throws a runtime error.
   * - Checks if the specified chain exists in the device.
   * - If the chain exists, adds the block to the chain and returns `true`.
   * - If the chain does not exist, returns `false`.
   */
  bool add_block_to_chain_type(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 2 arguments for command add_block_to_chain_type");
    }

    // Retrieve chain name, block name, and device name from the command-line
    // arguments.
    std::string chain_name = get_argument_value("-chain", argc, argv, true);
    std::string block_name = get_argument_value("-block", argc, argv, true);
    std::string device_name = get_argument_value("-device", argc, argv);

    // Ensure chain name is provided.
    if (chain_name.empty()) {
      throw std::runtime_error(
          "No chain specified in the command add_block_to_chain_type");
    }

    // Identify the correct device.
    device_block *device = nullptr;
    if (device_name.empty()) {
      device = current_device_.get();
    } else {
      device = get_device(device_name).get();
    }

    // Check if a valid device is identified.
    if (!device) {
      throw std::runtime_error(
          "No device found in the command add_block_to_chain_type " +
          device_name);
    }

    // Retrieve the specified block from the device.
    auto block = device->get_block(block_name);
    if (!block) {
      throw std::runtime_error("No block " + block_name + " found in " +
                               device_name +
                               " in the command add_block_to_chain_type");
    }

    // If the chain exists, add the block to the chain.
    if (device->blockChainExists(chain_name)) {
      device->addBlockToChain(chain_name, block);
      return true;
    }

    // If the chain doesn't exist, return false.
    return false;
  }
  /**
   * Finds a device block instance from its hierarchical name.
   *
   * @param instance_name The hierarchical name of the instance.
   * @return A pointer to the found device block instance, or nullptr if not
   * found.
   *
   * @throws std::runtime_error if the instance name is empty or if the
   * specified instance cannot be found.
   *
   * @details
   * This function splits the instance name into its hierarchical components and
   * determines the root device or block. It then attempts to find the
   * corresponding instance in the instance tree. If the instance is not found,
   * nullptr is returned.
   */
  device_block_instance *find_instance_from_hierarchical_name(
      const std::string &instance_name) {
    // Split the instance name into its hierarchical components.
    std::vector<std::string> xmr_refs =
        FOEDAG::StringUtils::tokenize(instance_name, ".", false);

    // Determine the root device or block.
    std::string curr = current_device_->device_name();
    device_block *device = nullptr;

    if (curr == xmr_refs[0]) {
      device = current_device_.get();
    } else {
      device = get_device(xmr_refs[0]).get();
    }

    device_block *root_block = nullptr;

    if (device) {
      root_block = device;
    } else {
      root_block = current_device_->get_block(xmr_refs[0], false).get();
    }

    // Attempt to find the corresponding instance in the instance tree.
    unsigned int idx = 1;
    device_block_instance *inst = nullptr;

    if (!root_block) {
      // First component might be an instance name.
      inst = current_device_->get_instance(xmr_refs[0]).get();
    } else {
      if (xmr_refs.size() < 2) {
        throw std::runtime_error(
            "Instance name needed for command get_instance_block_type \n You "
            "named either a device or a block in the current device");
      }

      inst = root_block->get_instance(xmr_refs[idx++]).get();
    }

    while (inst && idx < xmr_refs.size()) {
      inst = inst->findInstanceByName(xmr_refs[idx]).get();
      ++idx;
    }

    return inst;
  }

  /**
   * @brief Creates an instance chain based on command-line arguments.
   *
   * This function processes the command-line arguments to create an instance
   * chain. It validates the number of arguments, retrieves the values for
   * chain, block, and device names, and then creates the chain on the specified
   * or current device.
   *
   * @param argc The number of command-line arguments.
   * @param argv The array of command-line argument strings.
   * @return true if the chain creation is successful.
   * @throws std::runtime_error if the number of arguments is less than
   * required, if no chain name is specified, or if the specified device or
   * block is not found.
   */
  bool create_instance_chain(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 2 arguments for command create_instance_chain");
    }

    // Retrieve the value of the chain name from the command-line arguments.
    std::string chain_name = get_argument_value("-chain", argc, argv, true);
    // Retrieve the value of the block name from the command-line arguments.
    std::string block_name = get_argument_value("-block", argc, argv);
    // Retrieve the value of the device name from the command-line arguments.
    std::string device_name = get_argument_value("-device", argc, argv);

    // Check if the chain name is specified.
    if (chain_name.empty()) {
      throw std::runtime_error(
          "No chain specified in the command create_instance_chain");
    }

    // Get the current device or the specified device.
    device_block *device = current_device_.get();
    if (!device_name.empty()) {
      device = get_device(device_name).get();
    }

    // Check if the device is valid.
    if (!device) {
      throw std::runtime_error(
          "No device found in the command create_instance_chain");
    }

    // Get the block from the device, if specified.
    device_block *block = nullptr;
    if (!block_name.empty()) {
      block = device->get_block(block_name).get();
    }

    // If no block is specified, use the device itself as the block.
    if (!block) {
      block = device;
    }

    // Create the chain on the specified block.
    device->createChain(chain_name, block);

    // Return true indicating the chain creation was successful.
    return true;
  }

  /**
   * @brief Appends an instance to an existing chain based on command-line
   * arguments.
   *
   * This function processes the command-line arguments to append an instance to
   * an existing chain. It validates the number of arguments, retrieves the
   * values for chain, block, instance, and device names, and then appends the
   * instance to the specified chain on the specified or current device.
   *
   * @param argc The number of command-line arguments.
   * @param argv The array of command-line argument strings.
   * @return true if the instance is successfully appended to the chain.
   * @throws std::runtime_error if the number of arguments is less than
   * required, if no chain name is specified, or if the specified device, block,
   * or instance is not found.
   */
  bool append_instance_to_chain(int argc, const char **argv) {
    // Validate the number of command-line arguments.
    if (argc < 5) {
      throw std::runtime_error(
          "Need at least 2 arguments for command append_instance_to_chain");
    }

    /// The chain can be at device model level or at block level.
    std::string chain_name = get_argument_value("-chain", argc, argv, true);
    /// If no block is specified, the chain is at device level.
    std::string block_name = get_argument_value("-block", argc, argv);
    /// If no device is specified, the chain is within current_device_
    std::string inst_name = get_argument_value("-instance", argc, argv);
    std::string device_name = get_argument_value("-device", argc, argv);

    // Check if the chain name is specified.
    if (chain_name.empty()) {
      throw std::runtime_error(
          "No chain specified in the command append_instance_to_chain");
    }

    // Get the current device or the specified device.
    device_block *device = current_device_.get();
    if (!device_name.empty()) {
      device = get_device(device_name).get();
    }

    // Check if the device is valid.
    if (!device) {
      throw std::runtime_error(
          "No device found in the command append_instance_to_chain");
    }

    // Get the block from the device, if specified.
    device_block *block = nullptr;
    if (!block_name.empty()) {
      block = device->get_block(block_name).get();
    }

    // If no block is specified, use the device itself as the block.
    if (!block) {
      block = device;
    }

    /// Use find_instance_from_hierarchical_name to check if the instance
    /// exists.
    device_block_instance *inst =
        find_instance_from_hierarchical_name(inst_name);
    if (!inst) {
      throw std::runtime_error(
          "No instance found in the command append_instance_to_chain");
    }

    // Append the instance to the chain on the specified block.
    block->append_instance_to_chain(chain_name, inst_name);

    // Return true indicating the instance was successfully appended to the
    // chain.
    return true;
  }

 private:
  int convert_string_to_integer(const std::string &str) {
    int value = 0;
    try {
      value = std::stoi(str, nullptr, 0);
    } catch (std::invalid_argument const &e) {
      std::string err =
          "Bad input: std::invalid_argument thrown when converting string '" +
          str + "' to integer\n";
      throw std::runtime_error(err.c_str());
    } catch (std::out_of_range const &e) {
      std::string err =
          "Bad input: std::out_of_range thrown when converting string '" + str +
          "' to integer\n";
      throw std::runtime_error(err.c_str());
    }
    return value;
  }
  double convert_string_to_double(const std::string &str) {
    double value = 0;
    try {
      value = std::stod(str);
    } catch (std::invalid_argument const &e) {
      std::string err =
          "Bad input: std::invalid_argument thrown when converting string '" +
          str + "' to double\n";
      throw std::runtime_error(err.c_str());
    } catch (std::out_of_range const &e) {
      std::string err =
          "Bad input: std::out_of_range thrown when converting string '" + str +
          "' to double\n";
      throw std::runtime_error(err.c_str());
    }
    return value;
  }

  std::shared_ptr<device> current_device_ =
      nullptr;  ///< The current device being worked on.
  /**
   * @brief Private constructor for the singleton device_modeler.
   */
  device_modeler() = default;

  /**
   * @brief Map holding all the devices managed by the device_modeler.
   * The keys are a combination of the device name and version.
   */
  std::unordered_map<std::string, std::shared_ptr<device>> devices_;
};
