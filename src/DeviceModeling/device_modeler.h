/**
 * @file device_modeler.h
 * @author Manadher Kharroubi (manadher.kharroubi@rapidsilicon.com)
 * @brief
 * @version 0.1
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
          std::string(", should start an alphanumeric character or or \"_\"");
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
        spdlog::error("Enum type {} already exists. Use -force to override.",
                      enumName);
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
      double lower = DBL_MIN;
      double upper = DBL_MAX;
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
    if (!block) return ret;
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
