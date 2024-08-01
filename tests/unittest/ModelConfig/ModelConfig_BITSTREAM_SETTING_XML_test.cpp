/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Utils/FileUtils.h"
#include "compiler_tcl_infra_common.h"

class ModelConfig_BITSTREAM_SETTING_XML : public ::testing::Test {
 protected:
  void SetUp() override {
    compiler_tcl_common_setup();
    create_unittest_directory("ModelConfig");
    std::filesystem::current_path("utst/ModelConfig");
  }
  void TearDown() override { std::filesystem::current_path("../.."); }
};

TEST_F(ModelConfig_BITSTREAM_SETTING_XML, gen_bitstream_setting_xml) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  std::string cmd = CFG_print(
      "model_config gen_bitstream_setting_xml -is_unittest -device_size 22x4 "
      "-design %s/design_edit.sdc -pin %s/Pin_Table.csv "
      "%s/empty_bitstream_setting.xml bitstream_setting.xml",
      current_dir.c_str(), current_dir.c_str(), current_dir.c_str());
  compiler_tcl_common_run(cmd);
}

TEST_F(ModelConfig_BITSTREAM_SETTING_XML, compare_result) {
  std::string golden_dir = COMPILER_TCL_COMMON_GET_CURRENT_GOLDEN_DIR();
  compare_unittest_file(false, "bitstream_setting.xml", "ModelConfig",
                        golden_dir);
}
