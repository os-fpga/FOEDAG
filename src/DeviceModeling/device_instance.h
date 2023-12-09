/**
 * @file device_block_instance.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief Contains the device_block_instance class definition.
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 */

#pragma once

#include "device_block.h"
#include "speedlog.h"

/**
 * @class device_block_instance
 * @brief A derived class from device_block that represents an instance of a
 * device block.
 */
class device_block_instance : public device_block {
 public:
  /**
   * @brief Default constructor.
   */
  device_block_instance() : device_block() { block_type_ = "instance"; }

  /**
   * @brief Default constructor.
   */
  device_block_instance(std::shared_ptr<device_block> instaciated_block_ptr)
      : device_block(), instaciated_block_ptr_(instaciated_block_ptr) {
    block_type_ = "instance";
    if (!instaciated_block_ptr_) return;
    for (const auto &pr : instaciated_block_ptr_->attributes()) {
      this->attributes()[pr.first] =
          std::make_shared<Parameter<int>>(pr.second);
    }
    for (const auto &pr : instaciated_block_ptr_->int_parameters()) {
      this->int_parameters()[pr.first] =
          std::make_shared<Parameter<int>>(pr.second);
    }
    for (const auto &pr : instaciated_block_ptr_->double_parameters()) {
      this->double_parameters()[pr.first] =
          std::make_shared<Parameter<double>>(pr.second);
    }
    for (const auto &pr : instaciated_block_ptr_->string_parameters()) {
      this->string_parameters()[pr.first] =
          std::make_shared<Parameter<string>>(pr.second);
    }
  }
  /**
   * @brief constructor.
   */
  device_block_instance(std::shared_ptr<device_block> instaciated_block_ptr,
                        int instance_id, int logic_location_x,
                        int logic_location_y, int logic_address,
                        std::string instance_name = "__default_instance_name__",
                        std::string io_bank = "__default_io_bank_name__",
                        int logic_location_z = -1)
      : device_block_instance(instaciated_block_ptr)

  {
    instance_id_ = instance_id;
    logic_location_x_ = logic_location_x;
    logic_location_y_ = logic_location_y;
    logic_location_z_ = logic_location_z;
    logic_address_ = logic_address;
    instance_name_ = instance_name;
    io_bank_ = io_bank;
  }

  /**
   * @brief Gets the instance name.
   * @return The instance name.
   */
  const std::string &get_instance_name() const { return instance_name_; }

  /**
   * @brief Gets the IO bank.
   * @return The IO bank.
   */
  const std::string &get_io_bank() const { return io_bank_; }

  /**
   * @brief Gets the instance ID.
   * @return The instance ID.
   */
  int get_instance_id() const { return instance_id_; }

  /**
   * @brief Gets the logic location X.
   * @return The logic location X.
   */
  int get_logic_location_x() const { return logic_location_x_; }

  /**
   * @brief Gets the logic location Y.
   * @return The logic location Y.
   */
  int get_logic_location_y() const { return logic_location_y_; }

  /**
   * @brief Gets the logic location Z.
   * @return The logic location Z.
   */
  int get_logic_location_z() const { return logic_location_z_; }

  /**
   * @brief Gets the logic address.
   * @return The logic address.
   */
  int get_logic_address() const { return logic_address_; }

  /**
   * @brief Sets the instance name.
   * @param name The instance name to set.
   */
  void set_instance_name(const std::string &name) { instance_name_ = name; }

  /**
   * @brief Sets the IO bank.
   * @param bank The IO bank to set.
   */
  void set_io_bank(const std::string &bank) { io_bank_ = bank; }

  /**
   * @brief Sets the instance ID.
   * @param id The instance ID to set.
   */
  void set_instance_id(unsigned int id) { instance_id_ = id; }

  /**
   * @brief Sets the logic location X.
   * @param x The logic location X to set.
   */
  void set_logic_location_x(int x) { logic_location_x_ = x; }

  /**
   * @brief Sets the logic location Y.
   * @param y The logic location Y to set.
   */
  void set_logic_location_y(int y) { logic_location_y_ = y; }

  /**
   * @brief Sets the logic location Z.
   * @param z The logic location Z to set.
   */
  void set_logic_location_z(int z) { logic_location_z_ = z; }

  /**
   * @brief Sets the logic address.
   * @param address The logic address to set.
   */
  void set_logic_address(int address) { logic_address_ = address; }

  /**
   * @brief Get instance block
   */
  std::shared_ptr<device_block> get_block() { return instaciated_block_ptr_; }

  /**
   * @brief Overrides the operator << to print a device_block_instance.
   * @param os The output stream to write to.
   * @param instance The device_block_instance to print.
   * @return The modified output stream.
   */
  friend std::ostream &operator<<(std::ostream &os,
                                  const device_block_instance &instance) {
    // Call the base class operator<<
    // os << static_cast<const device_block &>(instance);

    // Print the additional attributes
    os << "Instance Name: " << instance.instance_name_ << std::endl;
    os << "IO Bank: " << instance.io_bank_ << std::endl;
    os << "Instance ID: " << instance.instance_id_ << std::endl;
    os << "Logic Location X: " << instance.logic_location_x_ << std::endl;
    os << "Logic Location Y: " << instance.logic_location_y_ << std::endl;
    os << "Logic Location Z: " << instance.logic_location_z_ << std::endl;
    os << "Logic Address: " << instance.logic_address_ << std::endl;

    return os;
  }
  /**
   * @brief Search for a device block instance by its name within all chains.
   *
   * @param name Name of the desired device block instance.
   * @return Shared pointer to the device block instance if found; nullptr
   * otherwise.
   */
  std::shared_ptr<device_block_instance> findInstanceByName(
      const std::string &name) const override {
    for (const auto &pair : instance_chains_) {
      for (const auto &instance : pair.second) {
        if (instance->get_instance_name() == name) {
          return instance;
        }
      }
    }
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
  bool removeInstanceFromChainByName(const std::string &chainName,
                                     const std::string &instanceName) override {
    auto chainIt = instance_chains_.find(chainName);
    if (chainIt != instance_chains_.end()) {
      auto &vector = chainIt->second;
      auto it = std::remove_if(
          vector.begin(), vector.end(),
          [&instanceName](
              const std::shared_ptr<device_block_instance> &instance) {
            return instance->get_instance_name() == instanceName;
          });
      if (it != vector.end()) {
        vector.erase(it, vector.end());
        return true;
      }
    }
    return false;
  }

 private:
  int instance_id_ = -1;
  int logic_location_x_ = -1;
  int logic_location_y_ = -1;
  int logic_location_z_ = -1;
  int logic_address_ = -1;
  std::shared_ptr<device_block> instaciated_block_ptr_ = nullptr;
  std::string instance_name_ = "__default_instance_name__";
  std::string io_bank_ = "__default_io_bank_name__";
};

// Logging
// std::shared_ptr<spdlog::logger> logger_ =
// spdlog::stdout_logger_mt("device_block_instance");
