/**
 * @file rs_expression_test.cpp
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "DeviceModeling/rs_expression.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

TEST(RsExpressionTest, DoubleExpressionEvaluation) {
  rs_expression<double> expr("3.5 + 2.5");
  std::map<std::string, double> value_map;
  ASSERT_TRUE(expr.evaluate_expression(value_map));
  ASSERT_DOUBLE_EQ(expr.get_value(), 6.0);
}

TEST(RsExpressionTest, IntExpressionEvaluation) {
  rs_expression<int> expr("3 + 2");
  std::map<std::string, int> value_map;
  ASSERT_TRUE(expr.evaluate_expression(value_map));
  ASSERT_EQ(expr.get_value(), 5);
}

TEST(RsExpressionTest, DoubleExpressionWithVariables) {
  rs_expression<double> expr("x + y");
  std::map<std::string, double> value_map;
  value_map["x"] = 3.5;
  value_map["y"] = 2.5;
  // auto buf_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  // auto logger = std::make_shared<spdlog::logger>("null_logger", buf_sink);
  // spdlog::set_default_logger(logger);
  ASSERT_TRUE(expr.evaluate_expression(value_map));
  ASSERT_DOUBLE_EQ(expr.get_value(), 6.0);
  // spdlog::set_default_logger(spdlog::default_logger());
}

TEST(RsExpressionTest, IntExpressionWithVariables) {
  rs_expression<int> expr("x + y");
  std::map<std::string, int> value_map;
  value_map["x"] = 3;
  value_map["y"] = 2;
  // auto buf_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  // auto logger = std::make_shared<spdlog::logger>("null_logger", buf_sink);
  // spdlog::set_default_logger(logger);
  ASSERT_TRUE(expr.evaluate_expression(value_map));
  ASSERT_EQ(expr.get_value(), 5);
  // spdlog::set_default_logger(spdlog::default_logger());
}

TEST(RsExpressionTest, EvaluateExpressionFailed) {
  rs_expression<int> expr("3+");
  std::map<std::string, int> value_map;
  // auto buf_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  // auto logger = std::make_shared<spdlog::logger>("null_logger", buf_sink);
  // spdlog::set_default_logger(logger);
  ASSERT_FALSE(expr.evaluate_expression(value_map));
  // spdlog::set_default_logger(spdlog::default_logger());
}
