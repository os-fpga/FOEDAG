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

class ModelConfig_IO : public ::testing::Test {
 protected:
  void SetUp() override {
    compiler_tcl_common_setup();
    create_unittest_directory("ModelConfig");
  }
  void TearDown() override {}
};

TEST_F(ModelConfig_IO, set_property) {
  compiler_tcl_common_run("clear_property");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HR PACKAGE_PIN HR_2_6_3P} "
      "{din[0]}");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HR PACKAGE_PIN HR_5_12_6P} "
      "{dout[0]}");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HP PACKAGE_PIN HP_2_CC_10_5P} "
      "clk0");
  compiler_tcl_common_run(
      "set_property -dict {ROUTE_TO_FABRIC_CLK 1 PACKAGE_PIN HR_5_CC_28_14P} "
      "clk1");
  compiler_tcl_common_run(
      "write_property utst/ModelConfig/model_config.property.json");
}

TEST_F(ModelConfig_IO, gen_ppdb) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  std::string cmd = CFG_print(
      "model_config gen_ppdb -netlist_ppdb %s/model_config_netlist.ppdb.json "
      "-config_mapping %s/apis/config_attributes.mapping.json "
      "-property_json utst/ModelConfig/model_config.property.json "
      "-is_unittest "
      "utst/ModelConfig/model_config.ppdb.json",
      current_dir.c_str(), current_dir.c_str());
  compiler_tcl_common_run(cmd);
}

TEST_F(ModelConfig_IO, gen_bitstream) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  std::vector<std::filesystem::path> files =
      FOEDAG::FileUtils::FindFilesByExtension(
          CFG_print("%s/ric", current_dir.c_str()), ".json");
  std::string cmd =
      CFG_print("source %s/ric/virgotc_bank.tcl", current_dir.c_str());
  compiler_tcl_common_run(cmd);
  compiler_tcl_common_run("model_config set_model -feature IO VIRGOTC_BANK");
  cmd = CFG_print("source %s/ric/virgotc_bank.tcl", current_dir.c_str());
  for (auto file : files) {
    if (file.string().size() > 9 &&
        file.string().rfind(".api.json") == file.string().size() - 9) {
      std::string filepath =
          CFG_change_directory_to_linux_format(file.string());
      cmd =
          CFG_print("model_config set_api -feature IO {%s}", filepath.c_str());
      compiler_tcl_common_run(cmd);
    }
  }
  compiler_tcl_common_run(
      "model_config set_design -feature IO "
      "utst/ModelConfig/model_config.ppdb.json");
  compiler_tcl_common_run(
      "model_config write -feature IO -format DETAIL "
      "utst/ModelConfig/model_config_io_bitstream.detail.bit");
}

TEST_F(ModelConfig_IO, compare_result) {
  std::string golden_dir = COMPILER_TCL_COMMON_GET_CURRENT_GOLDEN_DIR();
  compare_unittest_file(false, "model_config.ppdb.json", "ModelConfig",
                        golden_dir);
  compare_unittest_file(false, "model_config_io_bitstream.detail.bit",
                        "ModelConfig", golden_dir);
}
