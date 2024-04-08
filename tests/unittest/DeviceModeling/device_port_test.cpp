#include "DeviceModeling/device_port.h"

#include <gtest/gtest.h>

class DevicePortTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // This is run before every test.
    // You can set up resources or initial conditions here if needed.
  }

  void TearDown() override {
    // This is run after every test.
    // Clean up resources if any were allocated in SetUp.
  }
};

TEST_F(DevicePortTest, DefaultConstructor) {
  device_port port;
  EXPECT_EQ(port.get_name(), "__default_port_name__");
  EXPECT_EQ(port.is_input(), false);
  EXPECT_EQ(port.get_signal(), nullptr);
  EXPECT_EQ(port.get_block(), nullptr);
}

TEST_F(DevicePortTest, ParameterizedConstructor) {
  device_signal signal("test_signal");
  // device_block block;  // Assuming device_block has a default constructor.
  device_port port("test_port", true, &signal, nullptr);

  EXPECT_EQ(port.get_name(), "test_port");
  EXPECT_EQ(port.is_input(), true);
  EXPECT_EQ(port.get_signal(), &signal);
  EXPECT_EQ(port.get_block(), nullptr);
}

TEST_F(DevicePortTest, SetGetName) {
  device_port port;
  port.set_name("new_name");
  EXPECT_EQ(port.get_name(), "new_name");
}

TEST_F(DevicePortTest, SetDirection) {
  device_port port;
  port.set_direction(true);
  EXPECT_EQ(port.is_input(), true);
  EXPECT_EQ(port.is_output(), false);
}

TEST_F(DevicePortTest, SetGetSignal) {
  device_signal signal("signal_name");
  device_port port;
  port.set_signal(&signal);
  EXPECT_EQ(port.get_signal(), &signal);
}

TEST_F(DevicePortTest, SetGetBlock) {
  device_block* block =
      (device_block*)10101;  // Assuming device_block has a default constructor.
  device_port port;
  port.set_block(block);
  EXPECT_EQ(port.get_block(), block);
}

TEST_F(DevicePortTest, EqualFunction) {
  device_signal signal1("signal1");
  device_signal signal2("signal2");
  // device_block block1, block2;
  device_port port1("port1", true, &signal1, nullptr);
  device_port port2("port1", true, &signal1, nullptr);
  device_port port3("port2", false, &signal2, nullptr);

  EXPECT_FALSE(port1.equal(port2));
  EXPECT_FALSE(port1.equal(port3));
}

TEST_F(DevicePortTest, StreamInsertionOperator) {
  device_signal signal("test_signal");
  // device_block block;
  device_port port("test_port", true, &signal, nullptr);

  std::stringstream ss;
  ss << port;

  // This test assumes a specific string format for your stream insertion.
  // Adjust the expected string accordingly.
  std::string expected =
      "Port Name: test_port, Direction: Input, Signal: test_signal";
  EXPECT_EQ(ss.str(), expected);
}

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
