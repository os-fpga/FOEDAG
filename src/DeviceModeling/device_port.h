/**
 * @file device_port.h
 * @brief Class representing a port in a device.
 * @author Your Name (your-email@example.com)
 * @version 0.2
 * @date 2023-06-01
 * @copyright Copyright (c) 2023
 */

#pragma once

#include <sstream>
#include <string>

#include "device_signal.h"
#include "speedlog.h"

class device_block;

/**
 * @class device_port
 * @brief Class representing a port in a device.
 *
 * A port is a connection point in a block, associated with a signal.
 */
class device_port {
 public:
  /**
   * @brief Constructor that initializes the device_port.
   * @param name The name of the port.
   * @param is_in The direction of the port (true for input, false for output)
   * @param signal_ptr A pointer to the signal associated with the port.
   * @param block_ptr A pointer to the block for which this is a port.
   */
  device_port(const std::string &name, bool is_in = false,
              device_signal *signal_ptr = nullptr,
              device_block *block_ptr = nullptr, unsigned size = 1)
      : name_(name),
        is_in_(is_in),
        signal_ptr_(signal_ptr),
        enclosing_block_ptr_(block_ptr) {
    if (!signal_ptr) {
      signal_ptr_ = new device_signal(name_, size);
    }
  }
  /**
   * @brief Copy Constructor that initializes the device_port.
   * @param other
   */
  device_port(const device_port &other)
      : name_(other.get_name()),
        is_in_(other.is_in_),
        signal_ptr_(other.get_signal()),
        enclosing_block_ptr_(other.get_block()) {}

  /**
   * @brief Default constructor.
   * Initializes the device_port with default values.
   */
  device_port()
      : name_("__default_port_name__"),
        is_in_(false),
        signal_ptr_(nullptr),
        enclosing_block_ptr_(nullptr) {}

  /**
   * @brief Compare the current instance with another device_port instance.
   * @param other The other device_port instance to compare with.
   * @return True if same object.
   */
  bool equal(const device_port &other) const {
    if(&other == this) return true;
    return false;
  }

  /**
   * @brief Get the name of the port.
   * @return The name of the port.
   */
  std::string get_name() const { return name_; }

  /**
   * @brief Set the name of the port.
   * @param name The name of the port.
   */
  void set_name(const std::string &name) { name_ = name; }

  /**
   * @brief Check if the port is an input.
   * @return True if the port is an input, false otherwise.
   */
  bool is_input() const { return is_in_; }

  /**
   * @brief Check if the port is an output.
   * @return True if the port is an output, false otherwise.
   */
  bool is_output() const { return !is_in_; }

  /**
   * @brief Set the port direction.
   * @param is_in The direction of the port (true for input, false for output)
   */
  void set_direction(bool is_in) { is_in_ = is_in; }

  /**
   * @brief Get the signal associated with the port.
   * @return The signal associated with the port.
   */
  device_signal *get_signal() const { return signal_ptr_; }

  /**
   * @brief Set the signal associated with the port.
   * @param signal_ptr The signal associated with the port.
   */
  void set_signal(device_signal *signal_ptr) { signal_ptr_ = signal_ptr; }

  /**
   * @brief Get the block for which this is a port.
   * @return The block for which this is a port.
   */
  device_block *get_block() const { return enclosing_block_ptr_; }

  /**
   * @brief Set the block for which this is a port.
   * @param block_ptr The block for which this is a port.
   */
  void set_block(device_block *block_ptr) { enclosing_block_ptr_ = block_ptr; }

  /**
   * @brief Overload of the stream insertion operator for device_port.
   * @param os The output stream.
   * @param port The device_port to output.
   * @return The updated output stream.
   */
  friend std::ostream &operator<<(std::ostream &os, const device_port &port) {
    os << "Port Name: " << port.name_
       << ", Direction: " << (port.is_in_ ? "Input" : "Output") << ", Signal: ";
    if (port.signal_ptr_ != nullptr) {
      os << port.signal_ptr_->get_name();
    } else {
      os << "nullptr";
    }
    return os;
  }

  void set_enclosing_instance(device_block_instance *enclosing_instance) {
    enclosing_instance_ = enclosing_instance;
    signal_ptr_->set_enclosing_instance(enclosing_instance);
  }

 private:
  std::string name_;   ///< The name of the device_port
  bool is_in_ = true;  ///< The direction of the device_port (true for input,
                       ///< false for output)
  device_signal *signal_ptr_;  ///< A pointer to the signal driven by an input
                               ///< port or driving an output port
  device_block *enclosing_block_ptr_;  ///< A pointer to the block for which
                                       ///< this is a port
  device_block_instance *enclosing_instance_ =
      nullptr;  ///< A pointer to the instance
                ///< for which this is a port
};
