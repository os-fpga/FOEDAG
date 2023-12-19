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

TEST(ParameterTest, TestConstructor) {
  auto type = std::make_shared<ParameterType<int>>();
  Parameter<int> param("param", 5, type);
  param.set_address(123);
  param.set_size(4);
  EXPECT_EQ(param.get_address(), 123);
  EXPECT_EQ(param.get_size(), 4);
  // Constructor 1
  Parameter param1 = Parameter(param);
  EXPECT_EQ(param.get_name(), param1.get_name());
  EXPECT_EQ(param.get_type(), param1.get_type());
  EXPECT_EQ(param.get_value(), param1.get_value());
  EXPECT_EQ(param.get_address(), param1.get_address());
  EXPECT_EQ(param.get_size(), param1.get_size());
  // Constructor 2
  std::shared_ptr<Parameter<int>> param_ptr = std::make_shared<Parameter<int>>(param);
  Parameter param2 = Parameter(param_ptr);
  EXPECT_EQ(param.get_name(), param2.get_name());
  EXPECT_EQ(param.get_type(), param2.get_type());
  EXPECT_EQ(param.get_value(), param2.get_value());
  EXPECT_EQ(param.get_address(), param2.get_address());
  EXPECT_EQ(param.get_size(), param2.get_size());
  // No Address No Size test
  Parameter<int> no_addr_no_size_param("no_addr_no_size_param", 7, type);
  Parameter param3 = Parameter(no_addr_no_size_param);
  EXPECT_EQ(no_addr_no_size_param.get_name(), param3.get_name());
  EXPECT_EQ(no_addr_no_size_param.get_type(), param3.get_type());
  EXPECT_EQ(no_addr_no_size_param.get_value(), param3.get_value());
}
