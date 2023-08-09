/**
 * @file rs_expression.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include "rs_expression_evaluator.h"
/**
 * @class rs_expression
 * @brief Represents a mathematical expression, allowing for evaluation and output.
 */
template <typename T>
class rs_expression
{
    std::string expression_str_; ///< The string representation of the expression.
    T value_;                    ///< The evaluated expression value.
    bool is_evaluated_;          ///< Indicates whether the expression has been successfully evalua

public:
    /**
     * @brief Default constructor.
     */
    rs_expression() {}

    /**
     * @brief Constructor with expression string.
     * @param expr_str The string representation of the expression.
     */
    rs_expression(const std::string &expr) : expression_str_(expr), value_(0.0), is_evaluated_(false) {}

    /**
     * @brief Destructor.
     */
    ~rs_expression() {}

    /**
     * @brief Get the expression string.
     * @return The string representation of the expression.
     */
    const std::string &get_expression_string() const
    {
        return expression_str_;
    }
    /**
     * @brief Set the expression string.
     * @param expr_str The string representation of the expression.
     */
    void set_expression_string(const std::string &expr)
    {
        expression_str_ = expr;
        is_evaluated_ = false;
    }
    /**
     * @brief Get the evaluated expression value.
     * @return The evaluated expression value.
     */
    T get_value() const
    {
        if (!is_evaluated_)
        {
            throw std::runtime_error("Accessing value of not evaluated expression " + expression_str_);
        }
        return value_;
    }
    /**
     * @brief Set the evaluated expression value.
     * @param value The evaluated expression value.
     */
    void set_value(T value)
    {
        value_ = value;
        is_evaluated_ = true;
    }
    /**
     * @brief Evaluate the expression using the given symbol-value map.
     * @param value_map The map containing symbol-value pairs.
     * @return True if the evaluation is successful, false otherwise.
     */
    bool evaluate_expression(std::map<std::string, T> &value_map)
    {
        using evaluator_t = rs_expression_evaluator<double, T>;
        T result;
        bool success = evaluator_t::evaluate_expression(expression_str_, value_map, result);
        if (success)
        {
            set_value(result);
        }
        return success;
    }
    bool operator==( const rs_expression<T> &rhs) const
    {
        return get_expression_string() == rhs.get_expression_string();
    }

    /**
     * @brief Overload the output operator for the rs_expression class.
     * @param os The output stream.
     * @param expr The rs_expression object.
     * @return The modified output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, const rs_expression &expr)
    {
        os << "Expression: " << expr.get_expression_string();
        if (expr.is_evaluated_)
        {
            os << ", Value: " << expr.get_value();
        }
        else
        {
            os << " (Not Evaluated)";
        }
        return os;
    }
};
