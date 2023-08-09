/**
 * @file device_pin.h
 * @brief Contains the declaration of the device_pin class.
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @version 0.1
 * @date 2023-05-18
 *
 * This file defines the device_pin class, which represents a pin in a device.
 * A device_pin establishes a connection between two device_nets.
 */

#pragma once

#include <string>
#include <memory>
#include "device_net.h"
#include "spdlog/fmt/bundled/core.h"

/**
 * @class device_pin
 * @brief Class representing a pin in a device.
 *
 * A device_pin establishes a connection between two device_nets.
 */
class device_pin
{
public:
    /**
     * @brief Constructor that initializes the device_pin.
     * @param name The pin name.
     * @param is_in The pin direction.
     */
    device_pin(const std::string &name, bool is_in)
        : name_(name), is_in_(is_in)
    {
        sink_net_ = new device_net(name, "");
    }

    /**
     * @brief Copy constructor for the device_pin.
     * @param other The device_pin to copy.
     */
    device_pin(const device_pin &other)
        : name_(other.get_name()), is_in_(other.is_input())
    {
        sink_net_ = new device_net(other.get_name(), "");
    }

    /**
     * @brief Get the pin name.
     * @return The pin name.
     */
    std::string get_name() const { return name_; }

    /**
     * @brief Get the pin direction.
     * @return The pin direction.
     */
    std::string direction() const { return is_in_ ? "in" : "out"; }

    /**
     * @brief Check if the pin is an input.
     * @return True if the pin is an input, false otherwise.
     */
    bool is_input() const { return is_in_; }

    /**
     * @brief Check if the pin is an output.
     * @return True if the pin is an output, false otherwise.
     */
    bool is_output() const { return !is_in_; }

    /**
     * @brief Get the net driven by the pin.
     * @return The net driven by the pin.
     */
    device_net* get_sink_net() const { return sink_net_; }

    /**
     * @brief Set the net driving the pin.
     * @param net The net driving the pin.
     */
    void set_source_net(device_net* net)
    {
        source_net_ = net;
        source_net_->add_sink(sink_net_);
        sink_net_->set_source(source_net_);
    }

    /**
     * @brief Convert the device_pin to a string.
     * @return A string representation of the device_pin.
     */
    std::string to_string() const
    {
        std::ostringstream oss;
        oss << "Pin Name: " << name_ << ", Direction: " << (is_in_ ? "in" : "out");
        return oss.str();
    }

    /**
     * @brief Equality comparison operator for device_pin objects.
     * @param rhs The right-hand side device_pin object.
     * @return True if the device_pin objects are equal, false otherwise.
     */
    bool equal(const device_pin &rhs)
    {
        return get_name() == rhs.get_name() &&
               is_input() == rhs.is_input() &&
               get_sink_net() == rhs.get_sink_net();
    }

    /**
     * @brief Overload the << operator for device_pin.
     * @param os The output stream.
     * @param pin The device_pin.
     * @return The output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, const device_pin &pin)
    {
        os << pin.to_string();
        return os;
    }

    /**
     * @brief Equality comparison operator for device_pin objects.
     * @param lhs The left-hand side device_pin object.
     * @param rhs The right-hand side device_pin object.
     * @return True if the device_pin objects are equal, false otherwise.
     */
    bool operator==(const device_pin &rhs) const
    {
        return (this == &rhs);
    }

    /**
     * @brief Inequality comparison operator for device_pin objects.
     * @param lhs The left-hand side device_pin object.
     * @param rhs The right-hand side device_pin object.
     * @return True if the device_pin objects are not equal, false otherwise.
     */
    bool operator!=(const device_pin &rhs)
    {
        return !(*this == rhs);
    }

private:
    std::string name_;                       ///< The name of the pin.
    bool is_in_;                             ///< The direction of the pin.
    device_net* source_net_; ///< The net driving the pin.
    device_net* sink_net_;   ///< The net driven by the pin.
};

namespace fmt
{
    /**
     * @brief Formatter specialization for device_pin>.
     */
    template <>
    struct formatter<device_pin*>
    {
        /**
         * @brief Parse the format specifier.
         * @param ctx The format context.
         * @return An iterator past the parsed format specifier.
         */
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        /**
         * @brief Format the device_pin.
         * @param pin_ptr The shared pointer to the device_pin.
         * @param ctx The format context.
         * @return The formatted string.
         */
        template <typename FormatContext>
        auto format(const device_pin* pin_ptr, FormatContext &ctx)
        {
            return format_to(ctx.out(), "{}", pin_ptr->to_string());
        }
    };
}
