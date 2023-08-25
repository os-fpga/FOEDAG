#include "DeviceModeling/device_net.h"

#include <gtest/gtest.h>

#include <memory>

// This fixture will help in setting up some common setup and teardown for each
// test
class DeviceNetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Common setup if required
  }

  void TearDown() override {
    // Common cleanup if required
  }
};

// Testing the constructor
TEST_F(DeviceNetTest, ConstructWithNameOnly) {
  device_net net("test_net");
  EXPECT_EQ(net.get_net_name(), "test_net");
  EXPECT_EQ(net.get_signal(), nullptr);
}

// Testing the copy constructor
TEST_F(DeviceNetTest, CopyConstructor) {
  device_net net_original("test_net");
  device_net net_copy(net_original);
  EXPECT_EQ(net_copy.get_net_name(), "test_net");
  EXPECT_EQ(net_copy.get_signal(), nullptr);
}

// Testing setter and getter of net name
TEST_F(DeviceNetTest, SetAndGetNetName) {
  device_net net("initial_name");
  net.set_net_name("new_name");
  EXPECT_EQ(net.get_net_name(), "new_name");
}

// Testing source and sink relationships
TEST_F(DeviceNetTest, SetAndGetSource) {
  device_net net("test_net");
  std::shared_ptr<device_net> source_net =
      std::make_shared<device_net>("source_net");
  net.set_source(source_net);
  EXPECT_EQ(net.get_source(), source_net);
}

TEST_F(DeviceNetTest, AddAndGetSinks) {
  device_net net("test_net");
  std::shared_ptr<device_net> sink1 = std::make_shared<device_net>("sink1");
  std::shared_ptr<device_net> sink2 = std::make_shared<device_net>("sink2");

  net.add_sink(sink1);
  net.add_sink(sink2);

  const auto& sinks = net.get_sink_set();
  EXPECT_EQ(sinks.size(), 2);
  EXPECT_NE(sinks.find(sink1), sinks.end());
  EXPECT_NE(sinks.find(sink2), sinks.end());
}

// Testing equality functions
TEST_F(DeviceNetTest, EqualityOperators) {
  device_net net1("test_net");
  device_net net2("test_net");
  device_net net3("another_net");

  EXPECT_FALSE(net1 == net2);  // Since your == checks for same object reference
  EXPECT_TRUE(net1 != net2);   // Inverse
  EXPECT_FALSE(net1 == net3);
  EXPECT_TRUE(net1 != net3);
}

TEST_F(DeviceNetTest, CheckEqualityFunction) {
  device_net net1("test_net");
  device_net net2("test_net");
  device_net net3("another_net");

  EXPECT_TRUE(net1.equal(net2));  // Since they are separate instances with
                                   // potentially separate sinks, sources etc.
  EXPECT_FALSE(net1.equal(net3));
}

// Testing string representation functions
TEST_F(DeviceNetTest, ToStringFunction) {
  device_net net("test_net");
  std::string expected_start = "Net Name: test_net";
  EXPECT_TRUE(net.to_string().find(expected_start) != std::string::npos);
}

TEST_F(DeviceNetTest, FormattedStringFunction) {
  device_net net("test_net");
  std::string expected_start = "device_net: Net Name: test_net";
  EXPECT_TRUE(net.to_formatted_string().find(expected_start) !=
              std::string::npos);
}
