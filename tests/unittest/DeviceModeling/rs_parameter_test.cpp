/**
 * @file rs_parameter_test.cpp
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "DeviceModeling/rs_parameter.h"

#include <gtest/gtest.h>

TEST(ParameterTest, Constructor) {
  auto type = std::make_shared<ParameterType<int>>();
  Parameter<int> param("test", 5, type);

  EXPECT_EQ(param.get_name(), "test");
  EXPECT_EQ(param.get_value(), 5);
}

TEST(ParameterTest, SetValue) {
  auto type = std::make_shared<ParameterType<int>>();
  Parameter<int> param("test", 5, type);
  param.set_value(10);

  EXPECT_EQ(param.get_value(), 10);
}

TEST(ParameterTest, InvalidValue) {
  auto type = std::make_shared<ParameterType<int>>();

  type->set_lower_bound(0);
  type->set_upper_bound(10);
  EXPECT_THROW(Parameter<int>("test", -1, type), std::runtime_error);
}

TEST(ParameterTest, SetAddress) {
  auto type = std::make_shared<ParameterType<int>>();
  Parameter<int> param("test", 5, type);
  param.set_address(123);

  EXPECT_EQ(param.get_address(), 123);
}

TEST(ParameterTest, IntToString) {
  auto type = std::make_shared<ParameterType<int>>();
  Parameter<int> param("test", 5, type);

  EXPECT_EQ(param.to_string(), "Parameter test: 5 of type int");
}

TEST(ParameterTest, DoubleToString) {
  auto type = std::make_shared<ParameterType<double>>();
  Parameter<double> param("test", 5.5, type);

  EXPECT_EQ(param.to_string(), "Parameter test: 5.5 of type double");
}

TEST(ParameterTest, StringToString) {
  auto type = std::make_shared<ParameterType<std::string>>();
  Parameter<std::string> param("test", "Test", type);

  EXPECT_EQ(param.to_string(), "Parameter test: Test of type string");
}

TEST(ParameterTest, InvalidSetAddress) {
  auto type = std::make_shared<ParameterType<double>>();
  Parameter<double> param("test", 5.5, type);

  EXPECT_THROW(param.set_address(123), std::runtime_error);
}
