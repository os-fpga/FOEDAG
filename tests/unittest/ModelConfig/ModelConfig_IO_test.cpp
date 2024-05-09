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

#include "Configuration/ModelConfig/ModelConfig_IO.h"

#include "Utils/FileUtils.h"
#include "compiler_tcl_infra_common.h"

class ModelConfig_IO : public ::testing::Test {
 protected:
  void SetUp() override {
    compiler_tcl_common_setup();
    create_unittest_directory("ModelConfig");
  }
  void source_model() {
    std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
    std::vector<std::filesystem::path> files =
        FOEDAG::FileUtils::FindFilesByExtension(
            CFG_print("%s/ric", current_dir.c_str()), ".json");
    std::string cmd =
        CFG_print("source %s/ric/virgotc_bank.tcl", current_dir.c_str());
    compiler_tcl_common_run("undefine_device VIRGOTC_BANK");
    compiler_tcl_common_run(cmd);
    compiler_tcl_common_run("model_config set_model -feature IO VIRGOTC_BANK");
    cmd = CFG_print("source %s/ric/virgotc_bank.tcl", current_dir.c_str());
    for (auto file : files) {
      if (file.string().size() > 9 &&
          file.string().rfind(".api.json") == file.string().size() - 9) {
        std::string filepath =
            CFG_change_directory_to_linux_format(file.string());
        cmd = CFG_print("model_config set_api -feature IO {%s}",
                        filepath.c_str());
        compiler_tcl_common_run(cmd);
      }
    }
  }
  void TearDown() override {}
};

TEST_F(ModelConfig_IO, allocate_resources) {
  std::vector<std::vector<uint32_t>> TEST_CASES = {
      {0, 2, 3}, {1, 3}, {1, 2}, {1, 2}, {0}};
  std::vector<MODEL_RESOURCE_INSTANCE*> instances;
  uint32_t i = 0;
  for (auto test : TEST_CASES) {
    printf("Testing Case %d\n", i);
    uint32_t possible = 0;
    for (auto p : test) {
      possible |= (1 << p);
    }
    MODEL_RESOURCE_INSTANCE* new_resource =
        new MODEL_RESOURCE_INSTANCE("a", possible, 5, i);
    bool status = FOEDAG::ModelConfig_IO::allocate_resource(instances,
                                                            new_resource, true);
    if (i < 4) {
      EXPECT_EQ(status, true);
      EXPECT_EQ((uint32_t)(instances.size()), i + 1);
      EXPECT_NE(new_resource, nullptr);
      if (i == 0) {
        EXPECT_EQ(instances[0]->decision, 0);
      } else if (i == 1) {
        EXPECT_EQ(instances[0]->decision, 0);
        EXPECT_EQ(instances[1]->decision, 1);
      } else if (i == 2) {
        EXPECT_EQ(instances[0]->decision, 0);
        EXPECT_EQ(instances[1]->decision, 1);
        EXPECT_EQ(instances[2]->decision, 2);
      } else {
        EXPECT_EQ(instances[0]->decision, 0);
        EXPECT_EQ(instances[1]->decision, 3);
        EXPECT_EQ(instances[2]->decision, 2);
        EXPECT_EQ(instances[3]->decision, 1);
      }
    } else {
      EXPECT_EQ(status, false);
      EXPECT_EQ((uint32_t)(instances.size()), 4);
      EXPECT_EQ(new_resource, nullptr);
      EXPECT_EQ(instances[0]->decision, 0);
      EXPECT_EQ(instances[1]->decision, 3);
      EXPECT_EQ(instances[2]->decision, 2);
      EXPECT_EQ(instances[3]->decision, 1);
    }
    i++;
  }
  while (instances.size()) {
    delete instances.back();
    instances.pop_back();
  }
}

TEST_F(ModelConfig_IO, set_property) {
  compiler_tcl_common_run("clear_property");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HR PACKAGE_PIN HR_1_4_2P} "
      "{din}");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HR PACKAGE_PIN HR_1_6_3P} "
      "{dout}");
  compiler_tcl_common_run(
      "set_property -dict {IOSTANDARD LVCMOS_18_HP PACKAGE_PIN HR_1_CC_38_19P} "
      "clk0");
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

TEST_F(ModelConfig_IO, gen_ppdb_negative) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  std::string cmd = CFG_print(
      "model_config gen_ppdb -netlist_ppdb "
      "%s/model_config_netlist.negative.ppdb.json "
      "-config_mapping %s/apis/config_attributes.mapping.json "
      "-is_unittest "
      "utst/ModelConfig/model_config.negative.ppdb.json",
      current_dir.c_str(), current_dir.c_str());
  compiler_tcl_common_run(cmd);
}

TEST_F(ModelConfig_IO, gen_bitstream_source) { source_model(); }

TEST_F(ModelConfig_IO, gen_bitstream_set_design) {
  compiler_tcl_common_run(
      "model_config set_design -feature IO "
      "utst/ModelConfig/model_config.ppdb.json");
}

TEST_F(ModelConfig_IO, gen_bitstream_write) {
  compiler_tcl_common_run(
      "model_config write -feature IO -format DETAIL "
      "utst/ModelConfig/model_config_io_bitstream.detail.bit");
}

TEST_F(ModelConfig_IO, gen_bitstream_set_design_negative) {
  // Re-source
  source_model();
  compiler_tcl_common_run(
      "model_config set_design -feature IO "
      "utst/ModelConfig/model_config.negative.ppdb.json");
}

TEST_F(ModelConfig_IO, gen_bitstream_write_negative) {
  compiler_tcl_common_run(
      "model_config write -feature IO -format DETAIL "
      "utst/ModelConfig/model_config_io_bitstream.negative.detail.bit");
}

TEST_F(ModelConfig_IO, compare_result) {
  std::string golden_dir = COMPILER_TCL_COMMON_GET_CURRENT_GOLDEN_DIR();
  compare_unittest_file(false, "model_config.ppdb.json", "ModelConfig",
                        golden_dir);
  compare_unittest_file(false, "model_config_io_bitstream.detail.bit",
                        "ModelConfig", golden_dir);
  compare_unittest_file(false, "model_config.negative.ppdb.json", "ModelConfig",
                        golden_dir);
  compare_unittest_file(false, "model_config_io_bitstream.negative.detail.bit",
                        "ModelConfig", golden_dir);
}
