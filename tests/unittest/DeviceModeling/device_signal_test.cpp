#include "DeviceModeling/device_signal.h"

#include <gtest/gtest.h>

class DeviceSignalTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialization code here
  }

  void TearDown() override {
    // Code here will be called immediately after each test
  }
};

TEST_F(DeviceSignalTest, ConstructorDefaultName) {
  device_signal signal;
  EXPECT_EQ(signal.get_name(), "__default_signal_name__");
  EXPECT_EQ(signal.get_size(), 0u);
}

TEST_F(DeviceSignalTest, ConstructorWithName) {
  auto signal = device_signal::create("signalA", 3);
  EXPECT_EQ(signal->get_name(), "signalA");
  EXPECT_EQ(signal->get_size(), 3u);
}

TEST_F(DeviceSignalTest, InitializationVec) {
  auto signal = device_signal::create("signalB");
  signal->initialize_vec(3);
  EXPECT_EQ(signal->get_size(), 3u);
  EXPECT_EQ(signal->get_net(0)->get_net_name(), "signalB__0__");
  EXPECT_EQ(signal->get_net(1)->get_net_name(), "signalB__1__");
  EXPECT_EQ(signal->get_net(2)->get_net_name(), "signalB__2__");
}

TEST_F(DeviceSignalTest, ReplaceNet) {
  auto signal = device_signal::create("signalC");
  signal->initialize_vec(2);
  auto newNet = std::make_shared<device_net>("newNet");
  signal->replace_net(1, newNet);
  EXPECT_EQ(signal->get_net(1)->get_net_name(), "newNet");
}

TEST_F(DeviceSignalTest, AddNet) {
  auto signal = device_signal::create("signalD");
  auto newNet1 = std::make_shared<device_net>("newNet1");
  auto newNet2 = std::make_shared<device_net>("newNet2");
  signal->add_net(newNet1);
  signal->add_net(newNet2);
  EXPECT_EQ(signal->get_size(), 2u);
  EXPECT_EQ(signal->get_net(0)->get_net_name(), "newNet1");
  EXPECT_EQ(signal->get_net(1)->get_net_name(), "newNet2");
}

TEST_F(DeviceSignalTest, CopyConstructor) {
  auto original = device_signal::create("signalE");
  original->initialize_vec(2);
  device_signal copied(*original);
  EXPECT_EQ(copied.get_name(), "signalE");
  EXPECT_EQ(copied.get_size(), 2u);
  EXPECT_TRUE(copied.get_net(0)->equal(*original->get_net(0)));
  EXPECT_TRUE(copied.get_net(1)->equal(*original->get_net(1)));
}

TEST_F(DeviceSignalTest, StreamInsertionOperator) {
  auto signal = device_signal::create("signalI");
  signal->initialize_vec(2);
  std::ostringstream oss;
  oss << *signal;
  std::string expectedOutput =
      "Signal Name: signalI, Size: 2, Nets: signalI__0__ signalI__1__ ";
  EXPECT_EQ(oss.str(), expectedOutput);
}