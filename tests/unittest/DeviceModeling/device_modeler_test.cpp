#include "DeviceModeling/device_modeler.h"
#include "DeviceModeling/Model.h"
#include "gtest/gtest.h"

class DeviceModelerTest : public ::testing::Test {
 protected:
  // This function runs before each test
  void SetUp() override { }

  // This function runs after each test
  void TearDown() override {}
};

// define_block
TEST_F(DeviceModelerTest, define_block_parent) {
  const int argc = 3;
  const char* argv[argc] = { "define_block", "-name", "TEST_BLOCK_PARENT" }; 
  Model::get_modler().define_block(argc, argv);
}

// define_block
TEST_F(DeviceModelerTest, define_block) {
  const int argc = 3;
  const char* argv[argc] = { "define_block", "-name", "TEST_BLOCK" }; 
  Model::get_modler().define_block(argc, argv);
}

// define_param_int
TEST_F(DeviceModelerTest, define_param_int) {
  const int argc = 12;
  const char* argv[argc] = { "define_param",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_PARAM_INT",
                              "-addr",
                              "0x00C",
                              "-width",
                              "6",
                              "-type",
                              "int" }; 
  Model::get_modler().define_param(argc, argv);
}

// define_param_integer
TEST_F(DeviceModelerTest, define_param_integer) {
  const int argc = 12;
  const char* argv[argc] = { "define_param",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_PARAM_INTEGER",
                              "-addr",
                              "0x00C",
                              "-width",
                              "6",
                              "-type",
                              "integer" }; 
  Model::get_modler().define_param(argc, argv);
}

// define_param_double
TEST_F(DeviceModelerTest, define_param_double) {
  const int argc = 12;
  const char* argv[argc] = { "define_param",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_PARAM_DOUBLE",
                              "-addr",
                              "0x00C",
                              "-width",
                              "6",
                              "-type",
                              "double" }; 
  Model::get_modler().define_param(argc, argv);
}

// define_param_string
TEST_F(DeviceModelerTest, define_param_string) {
  const int argc = 12;
  const char* argv[argc] = { "define_param",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_PARAM_STRING",
                              "-addr",
                              "0x00C",
                              "-width",
                              "6",
                              "-type",
                              "string" }; 
  Model::get_modler().define_param(argc, argv);
}

// define_attr
TEST_F(DeviceModelerTest, define_attr) {
  const int argc = 14;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR",	
                              "-addr",
                              "0x004",
                              "-width", 
                              "1",
                              "-enum",
                              "{Slave 0}", 
                              "{Master 1}",
                              "-enumname", 
                              "TEST_ATTR_ENUM" }; 
  Model::get_modler().define_attr(argc, argv);
}

// define_invalid0_attr
TEST_F(DeviceModelerTest, define_invalid0_attr) {
  const int argc = 14;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR",	
                              "-addr",
                              "invalid",
                              "-width", 
                              "1",
                              "-enum",
                              "{Slave 0}", 
                              "{Master 1}",
                              "-enumname", 
                              "TEST_ATTR_ENUM" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("Bad input: std::invalid_argument thrown when converting string 'invalid' to integer\n"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_invalid1_attr
TEST_F(DeviceModelerTest, define_invalid1_attr) {
  const int argc = 14;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR",	
                              "-addr",
                              "100000000000000000000000000000009999999",
                              "-width", 
                              "1",
                              "-enum",
                              "{Slave 0}", 
                              "{Master 1}",
                              "-enumname", 
                              "TEST_ATTR_ENUM" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("Bad input: std::out_of_range thrown when converting string '100000000000000000000000000000009999999' to integer\n"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr
TEST_F(DeviceModelerTest, create_instance) {
  const int argc = 17;
  const char* argv[argc] = { "create_instance",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_BLOCK_INST",
                              "-logic_address",
                              "0",
                              "-logic_location",
                              "0 1 2",
                              "-logic_location_x",
                              "3",
                              "-logic_location_y",
                              "4",
                              "-logic_location_y",
                              "5",
                              "-parent",
                              "TEST_BLOCK_PARENT" }; 
  Model::get_modler().create_instance(argc, argv);
}

