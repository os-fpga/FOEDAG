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

#include "compiler_tcl_infra_common.h"

TEST(ModelConfig, device_model) {
  compiler_tcl_common_run("device_name TOP");
  compiler_tcl_common_run("define_block -name SUB1");
  compiler_tcl_common_run(
      "define_attr -block SUB1 -name ATTR1 -addr 0 -width 2 -enum {ENUM1 0} "
      "{ENUM2 1} {ENUM3 2} {ENUM4 3}");
  compiler_tcl_common_run(
      "define_attr -block SUB1 -name ATTR2 -addr 2 -width 6");
  compiler_tcl_common_run(
      "define_attr -block SUB1 -name ATTR3 -addr 8 -width 2 -enum {ENUM1 3} "
      "{ENUM2 2} {ENUM3 1} {ENUM4 0} -default ENUM2");
  compiler_tcl_common_run("define_block -name SUB2");
  compiler_tcl_common_run(
      "define_attr -block SUB2 -name ATTR1 -addr 0 -width 1 -enum {ENUM1 0} "
      "{ENUM2 1} -default 1");
  compiler_tcl_common_run(
      "define_attr -block SUB2 -name ATTR2 -addr 1 -width 2 -enum {ENUM1 0} "
      "{ENUM2 3} {ENUM3 1} {ENUM4 2}");
  compiler_tcl_common_run(
      "define_attr -block SUB2 -name ATTR3 -addr 3 -width 9 -default 0x11");
  compiler_tcl_common_run("define_block -name TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB1 -name SUB1_A -logic_address 0 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB1 -name SUB1_B -logic_address 10 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB2 -name SUB2_A -logic_address 20 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB2 -name SUB2_B -logic_address 32 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB2 -name SUB2_C -logic_address 44 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB2 -name SUB2_D -logic_address 56 -parent TOP");
  compiler_tcl_common_run(
      "create_instance -block SUB2 -name SUB2_E -logic_address 68 -parent TOP");
}

TEST(ModelConfig, model_config) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  compiler_tcl_common_run("model_config set_model -feature IO TOP");
  std::string tcl_cmd = CFG_print("model_config set_api %s/model_config.json",
                                  current_dir.c_str());
  compiler_tcl_common_run(tcl_cmd);
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB1_A -name mode -value "
      "MODE1");
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB1_B -name ATTR1 -value "
      "ENUM3");
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB1_B -name ATTR2 -value "
      "17");
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB2_A -name ATTR1 -value "
      "ENUM2");
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB2_A -name ATTR2 -value "
      "ENUM2");
  compiler_tcl_common_run(
      "model_config set_attr -feature IO -instance SUB2_A -name ATTR3 -value "
      "0x155");
  std::string expected_msg = CFG_print(
      "INFO: \"instances\" object is not defined, skip the design file "
      "\"%s/model_config_design_null_instances.json\"",
      current_dir.c_str());
  tcl_cmd = CFG_print(
      "model_config set_design %s/model_config_design_null_instances.json",
      current_dir.c_str());
  compiler_tcl_common_run(tcl_cmd, 0, expected_msg);
  expected_msg = CFG_print(
      "INFO: \"instances\" object is defined but empty, skip the design file "
      "\"%s/model_config_design_empty_instances.json\"",
      current_dir.c_str());
  tcl_cmd = CFG_print(
      "model_config set_design %s/model_config_design_empty_instances.json",
      current_dir.c_str());
  compiler_tcl_common_run(tcl_cmd, 0, expected_msg);
  tcl_cmd = CFG_print("model_config set_design %s/model_config_design.json",
                      current_dir.c_str());
  compiler_tcl_common_run(tcl_cmd);
  compiler_tcl_common_run(
      "model_config write -format BIT model_config_bit.txt");
  compiler_tcl_common_run(
      "model_config write -format WORD model_config_word.txt");
  compiler_tcl_common_run(
      "model_config write -format DETAIL model_config_detail.txt");
  compiler_tcl_common_run(
      "model_config write -format TCL model_config_tcl.txt");
  compiler_tcl_common_run(
      "model_config write -format BIN model_config_bin.bin");
  compiler_tcl_common_run("model_config dump_ric TOP model_config_top_ric.txt");
}

TEST(ModelConfig, compare_result) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  ASSERT_EQ(
      CFG_compare_two_text_files(
          "model_config_bit.txt",
          CFG_print("%s/model_config_bit.golden.txt", current_dir.c_str())),
      true);
  ASSERT_EQ(
      CFG_compare_two_text_files(
          "model_config_word.txt",
          CFG_print("%s/model_config_word.golden.txt", current_dir.c_str())),
      true);
  ASSERT_EQ(
      CFG_compare_two_text_files(
          "model_config_detail.txt",
          CFG_print("%s/model_config_detail.golden.txt", current_dir.c_str())),
      true);
  ASSERT_EQ(
      CFG_compare_two_text_files(
          "model_config_tcl.txt",
          CFG_print("%s/model_config_tcl.golden.txt", current_dir.c_str())),
      true);
  ASSERT_EQ(
      CFG_compare_two_binary_files(
          "model_config_bin.bin",
          CFG_print("%s/model_config_bin.golden.bin", current_dir.c_str())),
      true);
  ASSERT_EQ(
      CFG_compare_two_text_files(
          "model_config_top_ric.txt",
          CFG_print("%s/model_config_top_ric.golden.txt", current_dir.c_str()),
          true),
      true);
  // CFG_INTERNAL_ERROR("stop");
}
