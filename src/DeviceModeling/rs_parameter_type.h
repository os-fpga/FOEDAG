/**
 * @file rs_parameter_type.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <optional>
#include <unordered_map>

/**
 * @class ParameterType
 * @brief Template class to hold type information of a parameter.
 * @tparam T Parameter type, must be one of {int, double, string}.
 */
template <typename T>
class ParameterType {
  static_assert(
      std::is_same<T, int>::value || std::is_same<T, double>::value ||
          std::is_same<T, std::string>::value,
      "ParameterType can only be instantiated with int, double, or string");

  /**
   * @brief Optional lower bound for the parameter value.
   */
  std::optional<T> lower_bound_;

  /**
   * @brief Optional upper bound for the parameter value.
   */
  std::optional<T> upper_bound_;

  /**
   * @brief Optional default value for the parameter.
   */
  std::optional<T> default_value_;

  /**
   * @brief Optional size of the parameter.
   * Only used if parameter type is int.
   */
  std::optional<size_t> size_;

  /**
   * @brief Optional enumeration for int type parameter.
   * Only used if parameter type is int.
   */
  std::unordered_map<std::string, unsigned int> enum_values_;

 public:
  /**
   * @brief Default constructor.
   */
  ParameterType() = default;

  /**
   * @brief Default destructor.
   */
  ~ParameterType() = default;

  //... remaining methods remain same ...

  /**
   * @brief Check if the parameter type is int.
   * @return True if the parameter type is int, false otherwise.
   */
  bool is_int() const { return std::is_same<T, int>::value; }

  /**
   * @brief Check if the parameter type is double.
   * @return True if the parameter type is double, false otherwise.
   */
  bool is_double() const { return std::is_same<T, double>::value; }

  /**
   * @brief Check if the parameter type is string.
   * @return True if the parameter type is string, false otherwise.
   */
  bool is_string() const { return std::is_same<T, std::string>::value; }

  /**
   * @brief Check if the parameter has a lower bound.
   * @return True if the parameter has a lower bound, false otherwise.
   */
  bool has_lower_bound() const { return lower_bound_.has_value(); }

  /**
   * @brief Check if the parameter has an upper bound.
   * @return True if the parameter has an upper bound, false otherwise.
   */
  bool has_upper_bound() const { return upper_bound_.has_value(); }

  /**
   * @brief Check if the parameter has a default value.
   * @return True if the parameter has a default value, false otherwise.
   */
  bool has_default_value() const { return default_value_.has_value(); }

  /**
   * @brief Check if the parameter has a size.
   * @return True if the parameter has a size, false otherwise.
   */
  bool has_size() const { return size_.has_value(); }

  /**
   * @brief Set the lower bound of the parameter.
   * @param value The value to set as the lower bound.
   * @throws std::runtime_error If the parameter type is not int or double.
   */
  void set_lower_bound(T value) {
    if (is_int() || is_double()) {
      lower_bound_ = value;
      return;
    }
    throw std::runtime_error("Lower bound can not be set");
  }

  /**
   * @brief Get the lower bound of the parameter.
   * @return The lower bound of the parameter.
   * @throws std::runtime_error If the lower bound is not set.
   */
  T get_lower_bound() const {
    if (has_lower_bound()) {
      return lower_bound_.value();
    }
    throw std::runtime_error("Lower bound is not set");
  }

  /**
   * @brief Set the upper bound of the parameter.
   * @param value The value to set as the upper bound.
   * @throws std::runtime_error If the parameter type is not int or double.
   */
  void set_upper_bound(T value) {
    if (is_int() || is_double()) {
      upper_bound_ = value;
      return;
    }
    throw std::runtime_error("Upper bound can not be set");
  }

  /**
   * @brief Get the upper bound of the parameter.
   * @return The upper bound of the parameter.
   * @throws std::runtime_error If the upper bound is not set.
   */
  T get_upper_bound() const {
    if (has_upper_bound()) {
      return upper_bound_.value();
    }
    throw std::runtime_error("Upper bound is not set");
  }

  /**
   * @brief Set the size of the parameter.
   * @param value The value to set as the size.
   * @throws std::runtime_error If the parameter type is not int.
   */
  void set_size(size_t value) {
    if (is_int()) {
      size_ = value;
      return;
    }
    throw std::runtime_error("Size of parameter can not be set");
  }

  /**
   * @brief Get the size of the parameter.
   * @return The size of the parameter.
   * @throws std::runtime_error If the size is not set.
   */
  size_t get_size() const {
    if (has_size()) {
      return size_.value();
    }
    throw std::runtime_error("Parameter size is not set");
  }

  /**
   * @brief Set an enum value for the parameter.
   * @param key The name of the enum value.
   * @param value The integer value of the enum.
   * @throws std::runtime_error If the size is not set or the value does not fit
   * in the size.
   */
  void set_enum_value(std::string key, unsigned int value) {
    if (!has_size()) {
      throw std::runtime_error(
          "Please set the size of a parameter before setting the enum values");
    }
    unsigned mx = 1 << get_size();
    if (value >= mx) {
      throw std::runtime_error("The value " + std::to_string(value) +
                               " can not fit within " +
                               std::to_string(get_size()) + " bits");
    }
    enum_values_[key] = value;
  }

  /**
   * @brief Get an enum value by name.
   * @param key The name of the enum value.
   * @return The integer value of the enum.
   * @throws std::runtime_error If the enum value is not found.
   */
  unsigned int get_enum_value(std::string key) {
    if (enum_values_.find(key) == end(enum_values_)) {
      throw std::runtime_error("Could not find the enumeration key " + key);
    }
    return enum_values_[key];
  }

  /**
   * @brief Check if an enum value exists.
   * @param key The name of the enum value.
   * @return True if the enum value exists, false otherwise.
   */
  bool has_enum_value(std::string key) {
    return (enum_values_.find(key) != end(enum_values_));
  }

  /**
   * @brief Set the default value.
   * @param value The value to set as the default value.
   * @throws std::runtime_error If the value does not fit in the size, or is
   * outside the bounds.
   */
  void set_default_value(T value) {
    if constexpr (std::is_same<T, int>::value) {
      if (has_size()) {
        int mx = 1 << get_size();
        if (value < 0 || value >= mx) {
          throw std::runtime_error(
              "Invalid default value as the parameter size is set to " +
              std::to_string(get_size()) + " bits");
        }
      }
    }
    if constexpr (!std::is_same<T, std::string>::value) {
      if (has_lower_bound() && value < get_lower_bound()) {
        throw std::runtime_error(
            "Invalid default value as the parameter lower bound is set to " +
            std::to_string(get_lower_bound()));
      }
      if (has_upper_bound() && value > get_upper_bound()) {
        throw std::runtime_error(
            "Invalid default value as the parameter upper bound is set to " +
            std::to_string(get_upper_bound()));
      }
    }
    default_value_ = value;
  }

  /**
   * @brief Get the default value of the parameter.
   * @return The default value of the parameter.
   * @throws std::runtime_error If the default value was not set
   * */

  T get_default_value() {
    if (has_default_value()) {
      return default_value_.value();
    }
    throw std::runtime_error("Default_value not set");
  }

  /**
   * @brief Check if a given value is valid for the parameter.
   *
   * For string type parameters, all values are valid.
   * For int and double type parameters, the value is valid if it fits within
   * the size (for int type), and is within the set lower and upper bounds (if
   * they are set).
   *
   * @param value The value to check for validity.
   * @return True if the value is valid for the parameter, false otherwise.
   */

  bool is_valid(T value) {
    if constexpr (std::is_same<T, int>::value) {
      if (has_size()) {
        unsigned mx = 1 << get_size();
        if (value < 0 || value >= static_cast<T>(mx)) {
          return false;
        }
      }
    }
    if (has_lower_bound() && value < get_lower_bound()) {
      return false;
    }
    if (has_upper_bound() && value > get_upper_bound()) {
      return false;
    }
    return true;
  }
};
