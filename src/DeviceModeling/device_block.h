/**
 * @file device_block.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "device_port.h"
#include "rs_expression.h"
#include "rs_parameter.h"
#include "speedlog.h"

using namespace std;
class device_block_instance;
class device_block {
 public:
  /// Default constructor for device_block
  device_block() = default;
  /// Default constructor for device_block
  virtual ~device_block() {}

  /**
   * @brief Parameterized constructor for device_block
   *
   * @param block_name Name of the device block
   */
  device_block(const std::string &block_name) : block_name_(block_name) {}

  /**
   * @brief Getter for the name of the device block
   *
   * @return const std::string& Name of the device block
   */
  const std::string &block_name() const { return block_name_; }

  /**
   * @brief Setter for the name of the device block
   *
   * @return void
   */
  void set_block_name(const std::string &block_name) {
    block_name_ = block_name;
  }

  /**
   * @brief Getter for the type of the device block
   *
   * @return const std::string& Type of the device block
   */
  const std::string &block_type() const { return block_type_; }

  /**
   * @brief Add a port to the device block.
   * @param port_name Name of the port.
   * @param port Pointer to the device_port object to be added.
   */
  void add_port(const std::string &port_name,
                std::shared_ptr<device_port> port) {
    ports_map_[port_name] = port;
  }

  /**
   * @brief Add a port to the device block.
   * @param port Pointer to the device_port object to be added.
   */
  void add_port(std::shared_ptr<device_port> port) {
    const std::string &port_name = port->get_name();
    if (ports_map_.find(port_name) != end(ports_map_)) {
      throw std::runtime_error("Port " + port_name + " does already exist in " +
                               block_name_);
    }
    ports_map_[port_name] = port;
    for (auto &nt : port->get_signal()->get_net_vector()) {
      add_net(nt);
    }
  }

  /**
   * @brief Find a port in the device block.
   * @param port_name Name of the port to find.
   * @return Pointer to the found device_port object. Returns nullptr if the
   * port is not found.
   */
  std::shared_ptr<device_port> get_port(const std::string &port_name) {
    auto it = ports_map_.find(port_name);
    return it != ports_map_.end() ? it->second : nullptr;
  }

  /**
   * @brief Delete a port from the device block.
   * @param port_name Name of the port to delete.
   */
  void delete_port(const std::string &port_name) {
    ports_map_.erase(port_name);
  }

  /**
   * @brief Returns a reference to the map of device ports.
   *
   * This version of the method allows modification of the returned map.
   *
   * @return A non-const reference to the map of device ports.
   */
  std::unordered_map<std::string, std::shared_ptr<device_port>> &ports() {
    return ports_map_;
  }

  /**
   * @brief Returns a reference to the map of device ports.
   *
   * This version of the method does not allow modification of the returned map.
   *
   * @return A const reference to the map of device ports.
   */
  const std::unordered_map<std::string, std::shared_ptr<device_port>> &ports()
      const {
    return ports_map_;
  }

  /**
   * @brief Add a signal to the device block.
   * @param signal_name Name of the signal.
   * @param signal Pointer to the device_signal object to be added.
   */
  void add_signal(const std::string &signal_name,
                  std::shared_ptr<device_signal> signal) {
    signals_map_[signal_name] = signal;
  }

  /**
   * @brief Find a signal in the device block.
   * @param signal_name Name of the signal to find.
   * @return Pointer to the found device_signal object. Returns nullptr if the
   * signal is not found.
   */
  std::shared_ptr<device_signal> get_signal(
      const std::string &signal_name) const {
    auto it = signals_map_.find(signal_name);
    return it != signals_map_.end() ? it->second : nullptr;
  }

  /**
   * @brief Delete a signal from the device block.
   * @param signal_name Name of the signal to delete.
   */
  void delete_signal(const std::string &signal_name) {
    signals_map_.erase(signal_name);
  }

  /**
   * @brief Returns a reference to the map of device signals.
   *
   * This version of the method allows modification of the returned map.
   *
   * @return A non-const reference to the map of device signals.
   */
  std::unordered_map<std::string, std::shared_ptr<device_signal>>
      &device_signals() {
    return signals_map_;
  }

  /**
   * @brief Returns a reference to the map of device signals.
   *
   * This version of the method does not allow modification of the returned map.
   *
   * @return A const reference to the map of device signals.
   */
  const std::unordered_map<std::string, std::shared_ptr<device_signal>>
      &device_signals() const {
    return signals_map_;
  }

  /**
   * @brief Add a net to the device block.
   * @param net_name Name of the net.
   * @param net Pointer to the device_net object to be added.
   */
  void add_net(std::shared_ptr<device_net> net) {
    if (!net) {
      throw std::runtime_error("Can't add a null net");
    }
    std::string net_name = net->get_net_name();
    if (net_name.empty() || nets_map_.find(net_name) != end(nets_map_)) {
      throw std::runtime_error("Net " + net_name + " already exists in block " +
                               block_name_);
    }
    nets_map_[net_name] = net;
  }

  /**
   * @brief Find a net in the device block.
   * @param net_name Name of the net to find.
   * @return Pointer to the found device_net object. Returns nullptr if the net
   * is not found.
   */
  std::shared_ptr<device_net> get_net(const std::string &net_name) const {
    auto it = nets_map_.find(net_name);
    return it != nets_map_.end() ? it->second : nullptr;
  }

  /**
   * @brief Delete a net from the device block.
   * @param net_name Name of the net to delete.
   */
  void delete_net(const std::string &net_name) { nets_map_.erase(net_name); }

  /**
   * @brief Get a reference to the nets map.
   * @return A reference to the nets map.
   */
  std::unordered_map<std::string, std::shared_ptr<device_net>> &nets() {
    return nets_map_;
  }

  /**
   * @brief Get a const reference to the nets map.
   * @return A const reference to the nets map.
   */
  const std::unordered_map<std::string, std::shared_ptr<device_net>> &nets()
      const {
    return nets_map_;
  }

  /**
   * @brief Get a reference to the double parameters map.
   * @return A reference to the double parameters map.
   */
  std::unordered_map<std::string, std::shared_ptr<Parameter<double>>>
      &double_parameters() {
    return double_parameters_map_;
  }

  /**
   * @brief Get a const reference to the double parameters map.
   * @return A const reference to the double parameters map.
   */
  const std::unordered_map<std::string, std::shared_ptr<Parameter<double>>>
      &double_parameters() const {
    return double_parameters_map_;
  }

  /**
   * @brief Add a new double parameter to the double parameters map.
   * @param name The name of the new parameter.
   * @param param The new parameter to add.
   */
  void add_double_parameter(const std::string &name,
                            std::shared_ptr<Parameter<double>> param) {
    if (double_parameters_map_.find(name) != double_parameters_map_.end()) {
      spdlog::warn(
          "Double parameter {} already exists. It will be overwritten.", name);
    }
    double_parameters_map_[name] = std::move(param);
  }

  /**
   * @brief Get a specific double parameter from the double parameters map.
   * @param name The name of the parameter to get.
   * @return A pointer to the requested parameter, or nullptr if the parameter
   * does not exist.
   */
  std::shared_ptr<Parameter<double>> get_double_parameter(
      const std::string &name) {
    if (double_parameters_map_.find(name) != double_parameters_map_.end()) {
      return double_parameters_map_[name];
    } else {
      spdlog::warn("Double parameter {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific double parameter from the double parameters map.
   * @param name The name of the parameter to remove.
   */
  void remove_double_parameter(const std::string &name) {
    if (double_parameters_map_.find(name) != double_parameters_map_.end()) {
      double_parameters_map_.erase(name);
    } else {
      spdlog::warn("Double parameter {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the int parameters map.
   * @return A reference to the int parameters map.
   */
  std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      &int_parameters() {
    return int_parameters_map_;
  }

  /**
   * @brief Get a const reference to the int parameters map.
   * @return A const reference to the int parameters map.
   */
  const std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      &int_parameters() const {
    return int_parameters_map_;
  }

  /**
   * @brief Add a new int parameter to the int parameters map.
   * @param name The name of the new parameter.
   * @param param The new parameter to add.
   */
  void add_int_parameter(const std::string &name,
                         std::shared_ptr<Parameter<int>> param) {
    if (int_parameters_map_.find(name) != int_parameters_map_.end()) {
      spdlog::warn("Int parameter {} already exists. It will be overwritten.",
                   name);
    }
    int_parameters_map_[name] = std::move(param);
  }

  /**
   * @brief Get a specific int parameter from the int parameters map.
   * @param name The name of the parameter to get.
   * @return A pointer to the requested parameter, or nullptr if the parameter
   * does not exist.
   */
  std::shared_ptr<Parameter<int>> get_int_parameter(const std::string &name) {
    if (int_parameters_map_.find(name) != int_parameters_map_.end()) {
      return int_parameters_map_[name];
    } else {
      spdlog::warn("Int parameter {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific int parameter from the int parameters map.
   * @param name The name of the parameter to remove.
   */
  void remove_int_parameter(const std::string &name) {
    if (int_parameters_map_.find(name) != int_parameters_map_.end()) {
      int_parameters_map_.erase(name);
    } else {
      spdlog::warn("Int parameter {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the string parameters map.
   * @return A reference to the string parameters map.
   */
  std::unordered_map<std::string, std::shared_ptr<Parameter<std::string>>>
      &string_parameters() {
    return string_parameters_map_;
  }

  /**
   * @brief Get a const reference to the string parameters map.
   * @return A const reference to the string parameters map.
   */
  const std::unordered_map<std::string, std::shared_ptr<Parameter<std::string>>>
      &string_parameters() const {
    return string_parameters_map_;
  }

  /**
   * @brief Add a new string parameter to the string parameters map.
   * @param name The name of the new parameter.
   * @param param The new parameter to add.
   */
  void add_string_parameter(const std::string &name,
                            std::shared_ptr<Parameter<std::string>> param) {
    if (string_parameters_map_.find(name) != string_parameters_map_.end()) {
      spdlog::warn(
          "String parameter {} already exists. It will be overwritten.", name);
    }
    string_parameters_map_[name] = std::move(param);
  }

  /**
   * @brief Get a specific string parameter from the string parameters map.
   * @param name The name of the parameter to get.
   * @return A pointer to the requested parameter, or nullptr if the parameter
   * does not exist.
   */
  std::shared_ptr<Parameter<std::string>> get_string_parameter(
      const std::string &name) {
    if (string_parameters_map_.find(name) != string_parameters_map_.end()) {
      return string_parameters_map_[name];
    } else {
      spdlog::warn("String parameter {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific string parameter from the string parameters map.
   * @param name The name of the parameter to remove.
   */
  void remove_string_parameter(const std::string &name) {
    if (string_parameters_map_.find(name) != string_parameters_map_.end()) {
      string_parameters_map_.erase(name);
    } else {
      spdlog::warn("String parameter {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the attributes map.
   * @return A reference to the attributes map.
   */
  std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      &attributes() {
    return attributes_map_;
  }

  /**
   * @brief Get a const reference to the attributes map.
   * @return A const reference to the attributes map.
   */
  const std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      &attributes() const {
    return attributes_map_;
  }

  /**
   * @brief Add a new attribute to the attributes map.
   * @param name The name of the new attribute.
   * @param attr The new attribute to add.
   */
  void add_attribute(const std::string &name,
                     std::shared_ptr<Parameter<int>> attr) {
    if (attributes_map_.find(name) != attributes_map_.end()) {
      spdlog::warn("Attribute {} already exists. It will be overwritten.",
                   name);
    }
    attributes_map_[name] = std::move(attr);
  }

  /**
   * @brief Get a specific attribute from the attributes map.
   * @param name The name of the attribute to get.
   * @return A pointer to the requested attribute, or nullptr if the attribute
   * does not exist.
   */
  std::shared_ptr<Parameter<int>> get_attribute(const std::string &name,
                                                bool no_warning = false) {
    if (attributes_map_.find(name) != attributes_map_.end()) {
      return attributes_map_[name];
    } else {
      if (!no_warning) {
        spdlog::warn("Attribute {} does not exist.", name);
      }
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific attribute from the attributes map.
   * @param name The name of the attribute to remove.
   */
  void remove_attribute(const std::string &name) {
    if (attributes_map_.find(name) != attributes_map_.end()) {
      attributes_map_.erase(name);
    } else {
      spdlog::warn("Attribute {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the instance map.
   * @return A reference to the instance map.
   */
  std::unordered_map<std::string, std::shared_ptr<device_block_instance>>
      &instances() {
    return instance_map_;
  }

  /**
   * @brief Get a const reference to the instance map.
   * @return A const reference to the instance map.
   */
  const std::unordered_map<std::string, std::shared_ptr<device_block_instance>>
      &instances() const {
    return instance_map_;
  }

  /**
   * @brief Add a new instance to the instance map.
   * @param name The name of the new instance.
   * @param instance The new instance to add.
   */
  void add_instance(const std::string &name,
                    std::shared_ptr<device_block_instance> instance) {
    if (instance_map_.find(name) != instance_map_.end()) {
      spdlog::warn("Instance {} already exists. It will be overwritten.", name);
    }
    instance_map_[name] = std::move(instance);
  }

  /**
   * @brief Get a specific instance from the instance map.
   * @param name The name of the instance to get.
   * @return A pointer to the requested instance, or nullptr if the instance
   * does not exist.
   */
  std::shared_ptr<device_block_instance> get_instance(const std::string &name) {
    if (instance_map_.find(name) != instance_map_.end()) {
      return instance_map_[name];
    } else {
      spdlog::warn("Instance {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific instance from the instance map.
   * @param name The name of the instance to remove.
   */
  void remove_instance(const std::string &name) {
    if (instance_map_.find(name) != instance_map_.end()) {
      instance_map_.erase(name);
    } else {
      spdlog::warn("Instance {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the constraint map.
   * @return A reference to the constraint map.
   */
  std::unordered_map<std::string, std::shared_ptr<rs_expression<int>>>
      &constraints() {
    return constraint_map_;
  }

  /**
   * @brief Get a const reference to the constraint map.
   * @return A const reference to the constraint map.
   */
  const std::unordered_map<std::string, std::shared_ptr<rs_expression<int>>>
      &constraints() const {
    return constraint_map_;
  }

  /**
   * @brief Add a new constraint to the constraint map.
   * @param name The name of the new constraint.
   * @param constraint The new constraint to add.
   */
  void add_constraint(const std::string &name,
                      std::shared_ptr<rs_expression<int>> constraint) {
    if (constraint_map_.find(name) != constraint_map_.end()) {
      spdlog::warn("Constraint {} already exists. It will be overwritten.",
                   name);
    }
    constraint_map_[name] = std::move(constraint);
  }

  /**
   * @brief Get a specific constraint from the constraint map.
   * @param name The name of the constraint to get.
   * @return A pointer to the requested constraint, or nullptr if the constraint
   * does not exist.
   */
  std::shared_ptr<rs_expression<int>> get_constraint(const std::string &name) {
    if (constraint_map_.find(name) != constraint_map_.end()) {
      return constraint_map_[name];
    } else {
      spdlog::warn("Constraint {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific constraint from the constraint map.
   * @param name The name of the constraint to remove.
   */
  void remove_constraint(const std::string &name) {
    if (constraint_map_.find(name) != constraint_map_.end()) {
      constraint_map_.erase(name);
    } else {
      spdlog::warn("Constraint {} does not exist.", name);
    }
  }

  /**
   * @brief Get a specific enum type from the enum_types_ map.
   * @param name The name of the enum type to get.
   * @return A reference to the requested enum type.
   * @throws std::runtime_error If the enum type does not exist.
   */
  std::shared_ptr<ParameterType<int>> get_enum_type(
      const std::string &name, bool no_runtime_error = false) {
    auto it = enum_types_.find(name);
    if (it == enum_types_.end()) {
      if (no_runtime_error) {
        return nullptr;
      } else {
        throw std::runtime_error("Enum type " + name + " does not exist");
      }
    }
    return it->second;
  }

  /**
   * @brief Add a new enum type to the enum_types_ map.
   * @param name The name of the new enum type.
   * @param enum_type The new enum type to add.
   */
  void add_enum_type(const std::string &name,
                     std::shared_ptr<ParameterType<int>> enum_type) {
    enum_types_[name] = enum_type;
  }

  /**
   * @brief Remove a specific enum type from the enum_types_ map.
   * @param name The name of the enum type to remove.
   */
  void remove_enum_type(const std::string &name) { enum_types_.erase(name); }

  /**
   * @brief Get a reference to the block map.
   * @return A reference to the block map.
   */
  std::unordered_map<std::string, std::shared_ptr<device_block>> &blocks() {
    return block_map_;
  }

  /**
   * @brief Get a const reference to the block map.
   * @return A const reference to the block map.
   */
  const std::unordered_map<std::string, std::shared_ptr<device_block>> &blocks()
      const {
    return block_map_;
  }

  /**
   * @brief Add a new block to the block map.
   * @param name The name of the new block.
   * @param block The new block to add.
   */
  void add_block(const std::string &name, std::shared_ptr<device_block> block) {
    if (block_map_.find(name) != block_map_.end()) {
      spdlog::warn("Block {} already exists. It will be overwritten.", name);
    }
    block_map_[name] = std::move(block);
  }

  /**
   * @brief Add a new block to the block map.
   * @param block The new block to add.
   */
  void add_block(std::shared_ptr<device_block> block) {
    std::string name = block->block_name();
    if (block_map_.find(name) != block_map_.end()) {
      spdlog::warn("Block {} already exists. It will be overwritten.", name);
    }
    block_map_[name] = std::move(block);
  }

  /**
   * @brief Get a specific block from the block map.
   * @param name The name of the block to get.
   * @return A pointer to the requested block, or nullptr if the block does not
   * exist.
   */
  std::shared_ptr<device_block> get_block(const std::string &name,
                                          bool verbouse = true) {
    if (block_map_.find(name) != block_map_.end()) {
      return block_map_[name];
    } else {
      if (verbouse) spdlog::warn("Block {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific block from the block map.
   * @param name The name of the block to remove.
   */
  void remove_block(const std::string &name) {
    if (block_map_.find(name) != block_map_.end()) {
      block_map_.erase(name);
    } else {
      spdlog::warn("Block {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the double parameter types map.
   * @return A reference to the double parameter types map.
   */
  std::unordered_map<std::string, std::shared_ptr<ParameterType<double>>>
      &double_parameter_types() {
    return double_parameter_types_map_;
  }

  /**
   * @brief Get a const reference to the double parameter types map.
   * @return A const reference to the double parameter types map.
   */
  const std::unordered_map<std::string, std::shared_ptr<ParameterType<double>>>
      &double_parameter_types() const {
    return double_parameter_types_map_;
  }

  /**
   * @brief Add a new double parameter type to the double parameter types map.
   * @param name The name of the new parameter type.
   * @param param The new parameter type to add.
   */
  void add_double_parameter_type(
      const std::string &name,
      std::shared_ptr<ParameterType<double>> paramType) {
    if (double_parameter_types_map_.find(name) !=
        double_parameter_types_map_.end()) {
      spdlog::warn(
          "Double parameter type {} already exists. It will be overwritten.",
          name);
    }
    double_parameter_types_map_[name] = std::move(paramType);
  }

  /**
   * @brief Get a specific double parameter type from the double parameter types
   * map.
   * @param name The name of the parameter type to get.
   * @return A pointer to the requested parameter type, or nullptr if the
   * parameter type does not exist.
   */
  std::shared_ptr<ParameterType<double>> get_double_parameter_type(
      const std::string &name) {
    if (double_parameter_types_map_.find(name) !=
        double_parameter_types_map_.end()) {
      return double_parameter_types_map_[name];
    } else {
      // spdlog::warn("Double parameter type {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific double parameter type from the double parameter
   * types map.
   * @param name The name of the parameter type to remove.
   */
  void remove_double_parameter_type(const std::string &name) {
    if (double_parameter_types_map_.find(name) !=
        double_parameter_types_map_.end()) {
      double_parameter_types_map_.erase(name);
    } else {
      spdlog::warn("Double parameter type {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the int parameter types map.
   * @return A reference to the int parameter types map.
   */
  std::unordered_map<std::string, std::shared_ptr<ParameterType<int>>>
      &int_parameter_types() {
    return int_parameter_types_map_;
  }

  /**
   * @brief Get a const reference to the int parameter types map.
   * @return A const reference to the int parameter types map.
   */
  const std::unordered_map<std::string, std::shared_ptr<ParameterType<int>>>
      &int_parameter_types() const {
    return int_parameter_types_map_;
  }

  /**
   * @brief Add a new int parameter type to the int parameter types map.
   * @param name The name of the new parameter type.
   * @param param The new parameter type to add.
   */
  void add_int_parameter_type(const std::string &name,
                              std::shared_ptr<ParameterType<int>> paramType) {
    if (int_parameter_types_map_.find(name) != int_parameter_types_map_.end()) {
      spdlog::warn(
          "Int parameter type {} already exists. It will be overwritten.",
          name);
    }
    int_parameter_types_map_[name] = std::move(paramType);
  }

  /**
   * @brief Get a specific int parameter type from the int parameter types map.
   * @param name The name of the parameter type to get.
   * @return A pointer to the requested parameter type, or nullptr if the
   * parameter type does not exist.
   */
  std::shared_ptr<ParameterType<int>> get_int_parameter_type(
      const std::string &name) {
    if (int_parameter_types_map_.find(name) != int_parameter_types_map_.end()) {
      return int_parameter_types_map_[name];
    } else {
      // spdlog::warn("Int parameter type {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific int parameter type from the int parameter types
   * map.
   * @param name The name of the parameter type to remove.
   */
  void remove_int_parameter_type(const std::string &name) {
    if (int_parameter_types_map_.find(name) != int_parameter_types_map_.end()) {
      int_parameter_types_map_.erase(name);
    } else {
      spdlog::warn("Int parameter type {} does not exist.", name);
    }
  }

  /**
   * @brief Get a reference to the string parameter types map.
   * @return A reference to the string parameter types map.
   */
  std::unordered_map<std::string, std::shared_ptr<ParameterType<std::string>>>
      &string_parameter_types() {
    return string_parameter_types_map_;
  }

  /**
   * @brief Get a const reference to the string parameter types map.
   * @return A const reference to the string parameter types map.
   */
  const std::unordered_map<std::string,
                           std::shared_ptr<ParameterType<std::string>>>
      &string_parameter_types() const {
    return string_parameter_types_map_;
  }

  /**
   * @brief Add a new string parameter type to the string parameter types map.
   * @param name The name of the new parameter type.
   * @param paramType The new parameter type to add.
   */
  void add_string_parameter_type(
      const std::string &name,
      std::shared_ptr<ParameterType<std::string>> paramType) {
    if (string_parameter_types_map_.find(name) !=
        string_parameter_types_map_.end()) {
      spdlog::warn(
          "String parameter type {} already exists. It will be overwritten.",
          name);
    }
    string_parameter_types_map_[name] = std::move(paramType);
  }

  /**
   * @brief Get a specific string parameter type from the string parameter types
   * map.
   * @param name The name of the parameter type to get.
   * @return A pointer to the requested parameter type, or nullptr if the
   * parameter type does not exist.
   */
  std::shared_ptr<ParameterType<std::string>> get_string_parameter_type(
      const std::string &name) {
    if (string_parameter_types_map_.find(name) !=
        string_parameter_types_map_.end()) {
      return string_parameter_types_map_[name];
    } else {
      // spdlog::warn("String parameter type {} does not exist.", name);
      return nullptr;
    }
  }

  /**
   * @brief Remove a specific string parameter type from the string parameter
   * types map.
   * @param name The name of the parameter type to remove.
   */
  void remove_string_parameter_type(const std::string &name) {
    if (string_parameter_types_map_.find(name) !=
        string_parameter_types_map_.end()) {
      string_parameter_types_map_.erase(name);
    } else {
      spdlog::warn("String parameter type {} does not exist.", name);
    }
  }

  std::vector<std::shared_ptr<device_block_instance>> &instance_vector() {
    return instance_vector_;
  }
  const std::vector<std::shared_ptr<device_block_instance>> &instance_vector()
      const {
    return instance_vector_;
  }

  /**
   * @brief Create a new chain associated with a given key.
   *
   * If the key already exists, its associated vector will be overwritten.
   *
   * @param key Key associated with the new chain.
   */
  void createChain(const std::string &key) {
    instance_chains_[key] =
        std::vector<std::shared_ptr<device_block_instance>>();
  }

  /**
   * @brief Add a device block instance to a specific chain.
   *
   * If the key doesn't exist, it creates a new chain with the provided
   * instance.
   *
   * @param key Key associated with the chain.
   * @param instance Shared pointer to the device block instance.
   */
  void addInstanceToChain(const std::string &chain_name,
                          std::shared_ptr<device_block_instance> instance) {
    instance_chains_[chain_name].push_back(instance);
  }

  /**
   * @brief Retrieve the chain associated with a given key.
   *
   * @param key Key associated with the desired chain.
   * @return const reference to the vector of shared pointers associated with
   * the key.
   * @throw std::out_of_range if the key doesn't exist.
   */
  const std::vector<std::shared_ptr<device_block_instance>> &getChain(
      const std::string &chain_name) const {
    return instance_chains_.at(chain_name);
  }

  /**
   * @brief Search for a device block instance by its name within all chains.
   *
   * @param name Name of the desired device block instance.
   * @return Shared pointer to the device block instance if found; nullptr
   * otherwise.
   */
  virtual std::shared_ptr<device_block_instance> findInstanceByName(
      const std::string &name) const {
    return nullptr;
  }

  /**
   * @brief Remove a device block instance by its name from a specified chain.
   *
   * This function searches for a device block instance by its name within the
   * specified chain and removes the first occurrence. After removal, it
   * compacts the vector.
   *
   * @param chainName Name of the chain where the device block instance is
   * located.
   * @param instanceName Name of the device block instance to remove.
   * @return true if the instance was found and removed; false otherwise.
   */
  virtual bool removeInstanceFromChainByName(const std::string &chainName,
                                             const std::string &instanceName) {
    return false;
  }

  /**
   * @brief Check if a chain exists for a given key.
   *
   * @param key Key to check.
   * @return true if the chain exists; false otherwise.
   */
  bool chainExists(const std::string &key) const {
    return instance_chains_.find(key) != instance_chains_.end();
  }

  /**
   * @brief Get the value of a property in the property map.
   *
   * @param key The key of the property.
   * @return The value associated with the key, or an empty string if not found.
   */
  std::string getProperty(const std::string &key) const {
    auto it = property_map_.find(key);
    if (it != property_map_.end()) {
      return it->second;
    }
    return "";  // Property not found
  }

  /**
   * @brief Set the value of a property in the property map.
   *
   * If the property already exists, its value will be updated. If it doesn't
   * exist, a new property will be added to the map.
   *
   * @param key The key of the property.
   * @param value The value to set.
   */
  void setProperty(const std::string &key, const std::string &value) {
    property_map_[key] = value;
  }

  /**
   * @brief Check if a property exists in the property map.
   *
   * @param key The key of the property.
   * @return True if the property exists, false otherwise.
   */
  bool hasProperty(const std::string &key) const {
    return property_map_.find(key) != property_map_.end();
  }

  /**
   * @brief Remove a property from the property map.
   *
   * @param key The key of the property to remove.
   * @return True if the property was removed, false if it didn't exist.
   */
  bool removeProperty(const std::string &key) {
    auto it = property_map_.find(key);
    if (it != property_map_.end()) {
      property_map_.erase(it);
      return true;  // Property removed
    }
    return false;  // Property not found
  }

  // Overload of the operator <<
  friend ostream &operator<<(ostream &os, device_block &block) {
    os << "Device block: " << block.block_name() << "\n";

    os << "Ports:\n";
    for (const auto &port : block.ports()) {
      os << "  " << port.first << " = ";
      if (port.second != nullptr) {
        os << *port.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "Signals:\n";
    for (const auto &signal : block.device_signals()) {
      os << "  " << signal.first << " = ";
      if (signal.second != nullptr) {
        os << *signal.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }
    os << "Nets:\n";
    for (const auto &net : block.nets()) {
      os << "  " << net.first << " = ";
      if (net.second != nullptr) {
        os << *net.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "Double parameters:\n";
    for (const auto &parameter : block.double_parameters()) {
      os << "  " << parameter.first << " = ";
      if (parameter.second != nullptr) {
        os << *parameter.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "Int parameters:\n";
    for (const auto &parameter : block.int_parameters()) {
      os << "  " << parameter.first << " = ";
      if (parameter.second != nullptr) {
        os << *parameter.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "String parameters:\n";
    for (const auto &parameter : block.string_parameters()) {
      os << "  " << parameter.first << " = ";
      if (parameter.second != nullptr) {
        os << *parameter.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "Attributes:\n";
    for (const auto &attribute : block.attributes()) {
      os << "  " << attribute.first << " = ";
      if (attribute.second != nullptr) {
        os << *attribute.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    os << "Instances:\n";
    for (const auto &instance : block.instances()) {
      os << " Instnce : " << instance.first;  // << " = ";
      // if (instance.second != nullptr) {
      //   os << (instance.second);  // Assuming instance.second can't be
      //   // printed directly
      // } else {
      //   os << "nullptr";
      // }
      os << "\n";
    }

    os << "Constraints:\n";
    for (const auto &constraint : block.constraints()) {
      os << "  " << constraint.first << " = ";
      if (constraint.second != nullptr) {
        os << *constraint.second;
      } else {
        os << "nullptr";
      }
      os << "\n";
    }

    return os;
  }
  /**
   * @brief Adds a mapping between a model name and a customer name.
   *
   * @param modelName The name of the model.
   * @param custName The name of the customer.
   *
   * @throws std::runtime_error if a duplicate mapping is detected.
   */
  void addMapping(const std::string &modelName, const std::string &custName) {
    if (modelToCustMap_.count(modelName) > 0) {
      throw std::runtime_error(
          "Duplicate mapping detected.Already existing model name " +
          modelName);
    }
    if (custToModelMap_.count(custName) > 0) {
      throw std::runtime_error(
          "Duplicate mapping detected.Already existing customer name " +
          custName);
    }
    modelToCustMap_[modelName] = custName;
    custToModelMap_[custName] = modelName;
  }

  /**
   * @brief Gets the customer name associated with a model name.
   *
   * @param modelName The name of the model.
   * @return The associated customer name.
   *
   * @throws std::runtime_error if the model name is not found.
   */
  std::string getCustomerName(const std::string &modelName) const {
    auto it = modelToCustMap_.find(modelName);
    if (it != modelToCustMap_.end()) {
      return it->second;
    } else {
      return "";
    }
  }

  /**
   * @brief Gets the model name associated with a customer name.
   *
   * @param custName The name of the customer.
   * @return The associated model name.
   *
   * @throws std::runtime_error if the customer name is not found.
   */
  const std::string &getModelName(const std::string &custName) const {
    auto it = custToModelMap_.find(custName);
    if (it != custToModelMap_.end()) {
      return it->second;
    }
    throw std::runtime_error("Customer name not found.");
  }

  /**
   * @brief Removes a mapping for a given model name.
   *
   * @param modelName The name of the model.
   * @return True if the mapping was removed, false if the model name was not
   * found.
   */
  bool removeMappingByModel(const std::string &modelName) {
    auto it = modelToCustMap_.find(modelName);
    if (it != modelToCustMap_.end()) {
      custToModelMap_.erase(it->second);
      modelToCustMap_.erase(it);
      return true;
    }
    return false;
  }

  /**
   * @brief Removes a mapping for a given customer name.
   *
   * @param custName The name of the customer.
   * @return True if the mapping was removed, false if the customer name was not
   * found.
   */
  bool removeMappingByCustomer(const std::string &custName) {
    auto it = custToModelMap_.find(custName);
    if (it != custToModelMap_.end()) {
      modelToCustMap_.erase(it->second);
      custToModelMap_.erase(it);
      return true;
    }
    return false;
  }

 protected:
  std::unordered_map<std::string, std::string> modelToCustMap_;
  std::unordered_map<std::string, std::string> custToModelMap_;
  /// The name of the block. Defaults to "__default_block_name__".
  std::string block_name_ = "__default_block_name__";

  /// The type of the block. Defaults to "block".
  std::string block_type_ = "block";

  /// Map holding all the ports of the device block.
  std::unordered_map<std::string, std::shared_ptr<device_port>> ports_map_;

  /// Map holding all the signals of the device block.
  std::unordered_map<std::string, std::shared_ptr<device_signal>> signals_map_;

  /// Map holding all the nets of the device block.
  std::unordered_map<std::string, std::shared_ptr<device_net>> nets_map_;

  /// Map holding all the double parameters of the device block.
  std::unordered_map<std::string, std::shared_ptr<Parameter<double>>>
      double_parameters_map_;

  /// Map holding all the integer parameters of the device block.
  std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      int_parameters_map_;

  /// Map holding all the string parameters of the device block.
  std::unordered_map<std::string, std::shared_ptr<Parameter<std::string>>>
      string_parameters_map_;

  /// Map holding all the attributes of the device block.
  std::unordered_map<std::string, std::shared_ptr<Parameter<int>>>
      attributes_map_;

  /// Map holding all the instances of the device block.
  std::unordered_map<std::string, std::shared_ptr<device_block_instance>>
      instance_map_;

  /// Map holding all the constraints of the device block.
  std::unordered_map<std::string, std::shared_ptr<rs_expression<int>>>
      constraint_map_;

  /// Map holding all the ParameterType<int> instances, representing enum types.
  std::unordered_map<std::string, std::shared_ptr<ParameterType<int>>>
      enum_types_;

  /// Map holding all the block definitions of the device block.
  std::unordered_map<std::string, std::shared_ptr<device_block>> block_map_;

  /// Map holding all the double parameter types of the device block.
  std::unordered_map<std::string, std::shared_ptr<ParameterType<double>>>
      double_parameter_types_map_;

  /// Map holding all the int parameter types of the device block.
  std::unordered_map<std::string, std::shared_ptr<ParameterType<int>>>
      int_parameter_types_map_;

  /// Map holding all the string parameter types of the device block.
  std::unordered_map<std::string, std::shared_ptr<ParameterType<std::string>>>
      string_parameter_types_map_;

  /// Vector of instance references
  std::vector<std::shared_ptr<device_block_instance>> instance_vector_;

  /// The instance chains for bitstream
  std::unordered_map<std::string,
                     std::vector<std::shared_ptr<device_block_instance>>>
      instance_chains_;

  /// Map holding all the string properties of the device block.
  std::unordered_map<std::string, std::string> property_map_;

  friend class device_block_factory;
};
