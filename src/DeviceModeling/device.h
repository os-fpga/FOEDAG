/**
 * @file device.h
 * @brief Contains the definition of the device class, which represents a
 * device_block with additional attributes.
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @version 0.1
 * @date 2023-05-18
 *
 * @details The device class is derived from the device_block class and adds
 * additional attributes such as schema version, device name, and device
 * version. It provides getters and setters for these attributes and inherits
 * all the members of the base class.
 */

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "device_instance.h"
#include "rs_expression.h"
#include "speedlog.h"

/**
 * @class device
 * @brief Represents a device with additional attributes.
 * @extends device_block
 */
class device : public device_block {
 public:
  /**
   * @brief Default constructor for the device class.
   */
  device() : device_name_("__default_device_name__") {
    block_type_ = "device";
    add_int_parameter_type("int", std::make_shared<ParameterType<int>>());
    add_double_parameter_type("double",
                              std::make_shared<ParameterType<double>>());
    add_string_parameter_type("string",
                              std::make_shared<ParameterType<std::string>>());
  }

  /**
   * @brief Constructor for the device class with a specified device name.
   * @param device_name The name of the device.
   */
  device(const std::string &device_name) : device_name_(device_name) {
    block_type_ = "device";
    add_int_parameter_type("int", std::make_shared<ParameterType<int>>());
    add_double_parameter_type("double",
                              std::make_shared<ParameterType<double>>());
    add_string_parameter_type("string",
                              std::make_shared<ParameterType<std::string>>());
  }

  virtual ~device() {}
  /**
   * @brief Get the schema version of the device.
   * @return The schema version of the device.
   */
  const std::string &schema_version() const { return schema_version_; }

  /**
   * @brief Set the schema version of the device.
   * @param schema_version The schema version to set.
   */
  void set_schema_version(const std::string &schema_version) {
    schema_version_ = schema_version;
  }

  /**
   * @brief Get the name of the device.
   * @return The name of the device.
   */
  const std::string &device_name() const { return device_name_; }

  /**
   * @brief Set the name of the device.
   * @param device_name The name to set for the device.
   */
  void set_device_name(const std::string &device_name) {
    device_name_ = device_name;
  }

  /**
   * @brief Get the version of the device.
   * @return The version of the device.
   */
  const std::string &device_version() const { return device_version_; }

  /**
   * @brief Set the version of the device.
   * @param device_version The version to set for the device.
   */
  void set_device_version(const std::string &device_version) {
    device_version_ = device_version;
  }

  /**
   * @brief Equality operator for the device class.
   *
   * This method is used to compare two device objects for equality. It checks
   * each relevant attribute of the device class. If all attributes are equal,
   * the two devices are considered to be the same.
   *
   * @param rhs The right hand side device object to be compared with the
   * current device object.
   * @return True if the schema version, device name, and device version are the
   * same in both objects, false otherwise.
   */
  bool operator==(const device &rhs) const {
    return schema_version_ == rhs.schema_version_ &&
           device_name_ == rhs.device_name_ &&
           device_version_ == rhs.device_version_
        // Add additional comparisons for all relevant attributes.
        ;
  }

  /**
   * @brief Add a block name to the set of instantiated blocks.
   *
   * This method allows to record a block as being instantiated,
   * so it should remain unchanged during the device lifecycle.
   *
   * @param block_name The name of the block to add to the set.
   */
  void add_instantiated_block(const std::string &block_name) {
    instanciated_blocks_.insert(block_name);
  }

  /**
   * @brief Check if a block is already instantiated.
   *
   * This method checks if a block with the provided name is already recorded
   * as instantiated in the device.
   *
   * @param block_name The name of the block to check.
   * @return True if the block is recorded as instantiated, false otherwise.
   */
  bool is_block_instantiated(const std::string &block_name) const {
    return instanciated_blocks_.find(block_name) != instanciated_blocks_.end();
  }

  /**
   * @brief Remove a block name from the set of instantiated blocks.
   *
   * This method allows to remove a block from being recorded as instantiated,
   * possibly because it has been de-instantiated or modified.
   *
   * @note Use this method with caution as it can lead to inconsistencies if a
   * block is still in use.
   *
   * @param block_name The name of the block to remove from the set.
   */
  void remove_instantiated_block(const std::string &block_name) {
    instanciated_blocks_.erase(block_name);
  }

  /**
   * @brief Get the set of instantiated blocks.
   *
   * This method returns a constant reference to the set of instantiated blocks,
   * to allow inspection but not modification.
   *
   * @return A constant reference to the set of instantiated blocks.
   */
  const std::unordered_set<std::string> &get_instantiated_blocks() const {
    return instanciated_blocks_;
  }

  /**
   * @brief Overload of the operator << to print the device details.
   * @param os The output stream.
   * @param device The device to print.
   * @return The output stream after printing the device details.
   */
  friend std::ostream &operator<<(std::ostream &os, const device &device) {
    os << "Device:" << std::endl;
    os << "  Schema Version: " << device.schema_version_ << std::endl;
    os << "  Device Name: " << device.device_name_ << std::endl;
    os << "  Device Version: " << device.device_version_ << std::endl;
    // os << static_cast<const device_block &>(device); // Print base class
    // (device_block) details
    return os;
  }
  /**
   * @brief Sets a mapping from a user-visible name to its corresponding RTL
   * name.
   *
   * @param user_name The user-visible name.
   * @param rtl_name The corresponding RTL name.
   */
  void setUserToRtlMapping(const std::string &user_name,
                           const std::string &rtl_name) {
    user_to_rtl_map_[user_name] = rtl_name;
  }
  /**
   * @brief Gets the RTL name mapped to the provided user-visible name.
   *
   * If the user name is not found, an empty string is returned. Consider adding
   * more error handling based on the requirements (e.g., exceptions, optional
   * return values).
   *
   * @param user_name The user-visible name.
   * @return The corresponding RTL name or an empty string if not found.
   */
  std::string getRtlNameFromUser(const std::string &user_name) const {
    auto it = user_to_rtl_map_.find(user_name);
    if (it != user_to_rtl_map_.end()) {
      return it->second;
    }
    return "";  // or consider throwing an exception or using std::optional
  }

 private:
  std::string schema_version_;  ///< The schema version of the device.
  std::string device_name_;     ///< The name of the device.
  std::string device_version_;  ///< The version of the device.
  std::unordered_set<std::string>
      instanciated_blocks_;  ///< The set od used blocks that should stay
                             ///< unchanged
  std::unordered_map<std::string, std::string>
      user_to_rtl_map_;  ///< Mapping the user names to the RTL names
};
