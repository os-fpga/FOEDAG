/**
 * @file rs_parameter.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <sstream>

#include "rs_parameter_type.h"

/**
 * @brief A class that represents a named parameter with a specific type and
 * value.
 * @tparam T The type of the parameter value.
 */
template <typename T>
class Parameter {
  std::string name_;
  T value_;
  std::shared_ptr<ParameterType<T>> type_ptr_;
  std::optional<unsigned> address_;

 public:
  /**
   * @brief Construct a new Parameter object
   * @param name The name of the parameter.
   * @param type The type of the parameter.
   * @throws runtime_error If the value doesn't respect the type's bounds or
   * enumeration.
   */
  Parameter(const std::string &name, const T &value,
            const std::shared_ptr<ParameterType<T>> &type)
      : name_(name), type_ptr_(type) {
    if (type_ptr_->is_valid(value)) {
      value_ = value;
    } else {
      throw std::runtime_error(
          "Value doesn't respect the bounds or enumeration");
    }
  }

  /**
   * @brief Construct a new Parameter object
   * @param name The name of the parameter.
   * @param type The type of the parameter.
   * @throws runtime_error If the value doesn't respect the type's bounds or
   * enumeration.
   */
  Parameter(const std::string &name,
            const std::shared_ptr<ParameterType<T>> &type)
      : name_(name), type_ptr_(type) {
    if (type_ptr_->has_default_value()) {
      value_ = type_ptr_->get_default_value();
    }
  }

  /**
   * @brief Construct a new Parameter object "Copy Constructor"
   * @param other A reference to another parameter.
   */
  Parameter(const Parameter &other) {
    this->type_ptr_ = other.type_ptr_;
    this->name_ = other.name_;
    this->value_ = other.value_;
    if (this->has_address()) {
      this->address_ = other.get_address();
    }
  }

  /**
   * @brief Construct a new Parameter object "Copy Constructor"
   * @param other A reference to another parameter.
   */
  Parameter(const std::shared_ptr<Parameter> other) {
    this->type_ptr_ = other->type_ptr_;
    this->name_ = other->name_;
    this->value_ = other->value_;
    if (this->has_address()) {
      this->address_ = other->get_address();
    }
  }

  /**
   * @brief Construct a new Parameter object "default"
   */
  Parameter() = default;

  ~Parameter() = default;

  /**
   * @brief Sets the parameter's value.
   * @param value The new value of the parameter.
   * @throws runtime_error If the value doesn't respect the type's bounds or
   * enumeration.
   */
  void set_value(T value) {
    if (type_ptr_->is_valid(value)) {
      value_ = value;
    } else {
      throw std::runtime_error(
          "Value doesn't respect the bounds or enumeration");
    }
  }

  /**
   * @brief Gets the parameter's value.
   * @return The value of the parameter.
   */
  T get_value() const { return value_; }

  /**
   * @brief Checks if the parameter has an address.
   * @return true if the parameter has an address, false otherwise.
   */
  bool has_address() const { return address_.has_value(); }

  /**
   * @brief Sets the parameter's address.
   * @param value The new address of the parameter.
   * @throws runtime_error If the parameter's type is not int.
   */
  void set_address(unsigned value) {
    if (type_ptr_->is_int()) {
      address_ = value;
      return;
    }
    throw std::runtime_error("Address of non integer parameter can not be set");
  }

  /**
   * @brief Gets the parameter's address.
   * @return The address of the parameter.
   * @throws runtime_error If the parameter doesn't have an address.
   */
  size_t get_address() const {
    if (has_address()) {
      return address_.value();
    }
    throw std::runtime_error("Parameter address is not set");
  }

  /**
   * @brief Sets the parameter's name.
   * @param name The new name of the parameter.
   */
  void set_name(const std::string &name) { name_ = name; }

  /**
   * @brief Gets the parameter's name.
   * @return The name of the parameter.
   */
  std::string get_name() const { return name_; }

  /**
   * @brief Sets the parameter's type.
   * @param type The new type of the parameter.
   */
  void set_type(std::shared_ptr<ParameterType<T>> &type_p) {
    type_ptr_ = type_p;
  }

  /**
   */
  /**
   * @brief Gets the parameter's type.
   * @return The type of the parameter.
   */
  std::shared_ptr<ParameterType<T>> get_type() const { return type_ptr_; }

  /**
   * @brief Returns a string representation of the parameter.
   * @return A string representation of the parameter.
   */
  std::string to_string() const {
    std::ostringstream oss;
    std::string tp;
    std::string tn = std::string(typeid(T).name());
    tp = (tn == "i") ? "int" : ((tn == "d") ? "double" : "string");
    oss << "Parameter " << name_ << ": " << value_ << " of type " << tp;
    if (has_address()) {
      oss << " at address " << get_address();
    }
    return oss.str();
  }
  /**
   * @brief Equality comparison operator for Parameter objects.
   * @param lhs The left-hand side Parameter object.
   * @param rhs The right-hand side Parameter object.
   * @return True if the Parameter objects are equal, false otherwise.
   */
  bool operator==(const Parameter<T> &rhs) const { return (this == &rhs); }
  bool equal(const Parameter<T> &rhs) {
    return name_ == rhs.name_ && value_ == rhs.value_;
  }
  /**
   * @brief Equality comparison operator for Parameter objects.
   * @param lhs The left-hand side Parameter object.
   * @param rhs The right-hand side Parameter object.
   * @return True if the Parameter objects are equal, false otherwise.
   */
  bool operator!=(const Parameter<T> &rhs) { return (*this == rhs); }
  /**
   * @brief Overloads the << operator for printing the parameter.
   * @param os The output stream to print to.
   * @param param The parameter to print.
   * @return The output stream.
   */
  friend std::ostream &operator<<(std::ostream &os, const Parameter<T> &param) {
    os << param.to_string();
    return os;
  }
};
