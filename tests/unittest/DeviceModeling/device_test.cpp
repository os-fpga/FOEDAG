#include "DeviceModeling/device.h"

#include "gtest/gtest.h"

class DeviceTest : public ::testing::Test {
 protected:
  // This function runs before each test
  void SetUp() override { test_device_ = device("TestDevice"); }

  // This function runs after each test
  void TearDown() override {}

  device test_device_;
};

// Test default constructor
TEST_F(DeviceTest, DefaultConstructorTest) {
  device default_device;
  EXPECT_EQ(default_device.device_name(), "__default_device_name__");
  EXPECT_EQ(default_device.schema_version(), "");
  EXPECT_EQ(default_device.device_version(), "");
}

// Test parameterized constructor
TEST_F(DeviceTest, ParameterizedConstructorTest) {
  EXPECT_EQ(test_device_.device_name(), "TestDevice");
}

// Test setters and getters for schema version, device name, and device version
TEST_F(DeviceTest, SettersAndGettersTest) {
  test_device_.set_schema_version("v1.0");
  EXPECT_EQ(test_device_.schema_version(), "v1.0");

  test_device_.set_device_name("NewDevice");
  EXPECT_EQ(test_device_.device_name(), "NewDevice");

  test_device_.set_device_version("v2.0");
  EXPECT_EQ(test_device_.device_version(), "v2.0");
}

// Test the == operator
TEST_F(DeviceTest, EqualityOperatorTest) {
  device another_device("TestDevice");
  EXPECT_TRUE(test_device_ == another_device);

  another_device.set_device_version("v2.0");
  EXPECT_FALSE(test_device_ == another_device);
}

// Test add_instantiated_block, is_block_instantiated, and
// remove_instantiated_block
TEST_F(DeviceTest, InstantiatedBlocksTest) {
  test_device_.add_instantiated_block("Block1");
  EXPECT_TRUE(test_device_.is_block_instantiated("Block1"));

  test_device_.remove_instantiated_block("Block1");
  EXPECT_FALSE(test_device_.is_block_instantiated("Block1"));
}

// Test setUserToRtlMapping and getRtlNameFromUser
TEST_F(DeviceTest, UserToRtlMappingTest) {
  test_device_.setUserToRtlMapping("UserSignal", "RTLSignal");
  EXPECT_EQ(test_device_.getRtlNameFromUser("UserSignal"), "RTLSignal");
  EXPECT_EQ(test_device_.getRtlNameFromUser("NonexistentSignal"), "");
}
