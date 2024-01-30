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

// get_null_device_model
TEST_F(DeviceModelerTest, get_null_device_model) {
  device* model = Model::get_modler().get_device_model("__default_device_name__");
  EXPECT_EQ(model, nullptr);
}

// undefine_device
TEST_F(DeviceModelerTest, undefine_non_exist_device) {
  const int argc = 2;
  const char* argv[argc] = { "undefine_device", "TEST_DEVICE" }; 
  Model::get_modler().undefine_device(argc, argv);
}

// define_device
TEST_F(DeviceModelerTest, define_device) {  
  const int argc = 2;
  const char* argv[argc] = { "device_name", "TEST_DEVICE" }; 
  Model::get_modler().device_name(argc, argv);
}

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
  const int argc = 17;
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
                              "Slave 0,Master 1",
                              "-enumname", 
                              "TEST_ATTR_ENUM",
                              "-default",
                              "1",
                              "-upper_bound",
                              "1" }; 
  Model::get_modler().define_attr(argc, argv);
}

// define_attr_no_enumnum
TEST_F(DeviceModelerTest, define_attr_no_enumnum) {
  const int argc = 9;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR0",	
                              "-addr",
                              "0",
                              "-width", 
                              "1" }; 
  Model::get_modler().define_attr(argc, argv);
}

// define_invalid0_attr
TEST_F(DeviceModelerTest, define_invalid0_attr) {
  const int argc = 11;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR1",	
                              "-addr",
                              "invalid",
                              "-width", 
                              "1",
                              "-enum",
                              "Slave 0,Master 1" }; 
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
  const int argc = 13;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR1",	
                              "-addr",
                              "100000000000000000000000000000009999999",
                              "-width", 
                              "1",
                              "-enum",
                              "Slave 0,Master 1",
                              "-enumname", 
                              "TEST_ATTR1_INVALID_ENUM" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("Bad input: std::out_of_range thrown when converting string '100000000000000000000000000000009999999' to integer\n"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_duplicated_attr
TEST_F(DeviceModelerTest, define_duplicated_attr) {
  const int argc = 13;
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
                              "Slave 0,Master 1",
                              "-enumname", 
                              "TEST_ATTR0_ENUM" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("In the definition of Attribute TEST_ATTR, found duplication attribute in block TEST_BLOCK"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_duplicated_enumname
TEST_F(DeviceModelerTest, define_duplicated_enumname) {
  const int argc = 13;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR2",	
                              "-addr",
                              "invalid",
                              "-width", 
                              "1",
                              "-enum",
                              "Slave 0,Master 1",
                              "-enumname", 
                              "TEST_ATTR0_ENUM" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("In the definition of Attribute TEST_ATTR2, found duplication enumname TEST_ATTR0_ENUM in block TEST_BLOCK"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr_invalid_block
TEST_F(DeviceModelerTest, define_attr_invalid_block) {
  const int argc = 13;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK0",
                              "-name",
                              "TEST_ATTR2",	
                              "-addr",
                              "0",
                              "-width", 
                              "1",
                              "-enum",
                              "Slave 0,Master 1",
                              "-default",
                              "Master" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("In the definition of Attribute TEST_ATTR2, could not find block TEST_BLOCK0"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr_without_width
TEST_F(DeviceModelerTest, define_attr_without_width) {
  const int argc = 9;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR3",	
                              "-enum",
                              "Slave 0,Master 1",
                              "-default",
                              "Master" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::invalid_argument const& err) {
    EXPECT_EQ(err.what(), std::string("Missing necessary argument: -width"));
  } catch (...) {
    FAIL() << "Expected std::invalid_argument";
  }
}

// define_attr_invalid_width
TEST_F(DeviceModelerTest, define_attr_invalid_width) {
  const int argc = 11;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR3",	
                              "-addr",
                              "0",
                              "-width", 
                              "0",
                              "-enum",
                              "Slave 0,Master 1" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("Illegal size (0) when defining atttibute TEST_ATTR3"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr_empty_width
TEST_F(DeviceModelerTest, define_attr_empty_width) {
  const int argc = 11;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR3",	
                              "-addr",
                              "0",
                              "-width", 
                              "",
                              "-enum",
                              "Slave 0,Master 1" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("In the definition of Attribute TEST_ATTR3, width input is empty"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr_out_of_range_default
TEST_F(DeviceModelerTest, define_attr_out_of_range_default) {
  const int argc = 13;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR3",	
                              "-addr",
                              "0",
                              "-width", 
                              "3",
                              "-enum",
                              "Slave 0,Master 1",
                              "-default",
                              "100" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("The value 100 can not fit within 3 bits"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// define_attr_invalid_enums
TEST_F(DeviceModelerTest, define_attr_invalid_enums) {
  const int argc = 13;
  const char* argv[argc] = { "define_attr",
                              "-block",
                              "TEST_BLOCK",
                              "-name",
                              "TEST_ATTR3",	
                              "-addr",
                              "0",
                              "-width", 
                              "3",
                              "-enum",
                              "{Slave 0} {Master 1}",
                              "-default",
                              "7" }; 
  try {
    Model::get_modler().define_attr(argc, argv);
  } catch (std::runtime_error const& err) {
    EXPECT_EQ(err.what(), std::string("Fail to parse enum input ({Slave 0} {Master 1}) when defining atttibute TEST_ATTR3"));
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }
}

// create_instance
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

// repeat_get_null_device_model
TEST_F(DeviceModelerTest, repeat_get_null_device_model) {
  device* model = Model::get_modler().get_device_model("__default_device_name__");
  EXPECT_EQ(model, nullptr);
}

// get_device_model
TEST_F(DeviceModelerTest, get_device_model) {
  device* model = Model::get_modler().get_device_model("TEST_DEVICE");
  ASSERT_NE(model, nullptr);
}

// undefine_device
TEST_F(DeviceModelerTest, undefine_device) {
  const int argc = 2;
  const char* argv[argc] = { "undefine_device", "TEST_DEVICE" }; 
  Model::get_modler().undefine_device(argc, argv);
}

// get_device_model
TEST_F(DeviceModelerTest, get_deleted_device_model) {
  device* model = Model::get_modler().get_device_model("TEST_DEVICE");
  ASSERT_EQ(model, nullptr);
}

