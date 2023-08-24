/**
 * @file rs_parameter_type_test.cpp
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "DeviceModeling/rs_parameter_type.h"

#include "gtest/gtest.h"

// Test the is_int() function
TEST(ParameterTypeTest, IsInt) {
  ParameterType<int> int_param;
  EXPECT_TRUE(int_param.is_int());
}

// Test the is_double() function
TEST(ParameterTypeTest, IsDouble) {
  ParameterType<double> double_param;
  EXPECT_TRUE(double_param.is_double());
}

// Test the is_string() function
TEST(ParameterTypeTest, IsString) {
  ParameterType<std::string> string_param;
  EXPECT_TRUE(string_param.is_string());
}

// Test lower and upper bounds for int type
TEST(ParameterTypeTest, IntBounds) {
  ParameterType<int> int_param;
  int_param.set_lower_bound(1);
  int_param.set_upper_bound(10);

  EXPECT_TRUE(int_param.has_lower_bound());
  EXPECT_TRUE(int_param.has_upper_bound());
  EXPECT_EQ(int_param.get_lower_bound(), 1);
  EXPECT_EQ(int_param.get_upper_bound(), 10);
  EXPECT_TRUE(int_param.is_valid(5));
  EXPECT_FALSE(int_param.is_valid(0));
  EXPECT_FALSE(int_param.is_valid(11));
}

// Test enum values for int type
TEST(ParameterTypeTest, IntEnum) {
  ParameterType<int> int_param;
  int_param.set_size(3);  // 3 bits, maximum value is 7

  EXPECT_TRUE(int_param.has_size());
  EXPECT_EQ(int_param.get_size(), 3);

  int_param.set_enum_value(std::string("One"), 1);
  int_param.set_enum_value(std::string("Seven"), 7);

  EXPECT_TRUE(int_param.has_enum_value(std::string("One")));
  EXPECT_EQ(int_param.get_enum_value(std::string("One")), 1);
  EXPECT_TRUE(int_param.has_enum_value(std::string("Seven")));
  EXPECT_EQ(int_param.get_enum_value(std::string("Seven")), 7);
  EXPECT_THROW(int_param.set_enum_value(std::string("Eight"), 8),
               std::runtime_error);
}

// Test default value for int type
TEST(ParameterTypeTest, IntDefault) {
  ParameterType<int> int_param;
  int_param.set_lower_bound(1);
  int_param.set_upper_bound(10);

  int_param.set_default_value(5);

  EXPECT_TRUE(int_param.has_default_value());
  EXPECT_EQ(int_param.get_default_value(), 5);
  EXPECT_THROW(int_param.set_default_value(0), std::runtime_error);
  EXPECT_THROW(int_param.set_default_value(11), std::runtime_error);
}

// Test default value for int type with enum
TEST(ParameterTypeTest, IntDefaultEnum) {
  ParameterType<int> int_param;
  int_param.set_size(3);  // 3 bits, maximum value is 7

  int_param.set_enum_value(std::string("One"), 1);
  int_param.set_enum_value(std::string("Seven"), 7);

  int_param.set_default_value(1);

  EXPECT_TRUE(int_param.has_default_value());
  EXPECT_EQ(int_param.get_default_value(), 1);
  EXPECT_THROW(int_param.set_default_value(8), std::runtime_error);
}
