#include "DeviceModeling/device_instance.h"

#include "gtest/gtest.h"

class DeviceBlockInstanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor).
  }
};

// Test for the default constructor
TEST_F(DeviceBlockInstanceTest, DefaultConstructorTest) {
  device_block_instance instance;
  EXPECT_EQ(instance.get_instance_name(), "__default_instance_name__");
  EXPECT_EQ(instance.get_io_bank(), "__default_io_bank_name__");
  EXPECT_EQ(instance.get_instance_id(), -1);
  EXPECT_EQ(instance.get_logic_location_x(), -1);
  EXPECT_EQ(instance.get_logic_location_y(), -1);
  EXPECT_EQ(instance.get_logic_location_z(), -1);
  EXPECT_EQ(instance.get_logic_address(), -1);
}

// Test for the setters and getters of instance attributes
TEST_F(DeviceBlockInstanceTest, SettersAndGettersTest) {
  device_block_instance instance;

  instance.set_instance_name("TestInstance");
  EXPECT_EQ(instance.get_instance_name(), "TestInstance");

  instance.set_io_bank("TestBank");
  EXPECT_EQ(instance.get_io_bank(), "TestBank");

  instance.set_instance_id(5);
  EXPECT_EQ(instance.get_instance_id(), 5);

  instance.set_logic_location_x(10);
  EXPECT_EQ(instance.get_logic_location_x(), 10);

  instance.set_logic_location_y(20);
  EXPECT_EQ(instance.get_logic_location_y(), 20);

  instance.set_logic_location_z(30);
  EXPECT_EQ(instance.get_logic_location_z(), 30);

  instance.set_logic_address(40);
  EXPECT_EQ(instance.get_logic_address(), 40);
}

// Test for the full constructor (with all attributes)
TEST_F(DeviceBlockInstanceTest, FullConstructorTest) {
  std::shared_ptr<device_block> block_ptr = std::make_shared<device_block>();
  device_block_instance instance(block_ptr, 1, 100, 200, 300, "FullInstance",
                                 "FullBank", 400);

  EXPECT_EQ(instance.get_instance_name(), "FullInstance");
  EXPECT_EQ(instance.get_io_bank(), "FullBank");
  EXPECT_EQ(instance.get_instance_id(), 1);
  EXPECT_EQ(instance.get_logic_location_x(), 100);
  EXPECT_EQ(instance.get_logic_location_y(), 200);
  EXPECT_EQ(instance.get_logic_location_z(),
            400);  // Note: 400 because it's specified in the constructor
  EXPECT_EQ(instance.get_logic_address(), 300);
}

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
