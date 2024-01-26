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

#include "Compiler/Constraints.h"

#include "compiler_tcl_infra_common.h"

using namespace FOEDAG;

TEST(Constraints, set_property) {
  compiler_tcl_common_run("clear_property");
  compiler_tcl_common_run(
      "set_property gbox_mode MODE_BP_DIR_A_TX OBJ1 OBJ2 \t\t \t OBJ2");
  compiler_tcl_common_run(
      "set_property -dict {\t IOSTANDARD \t "
      "LVCMOS_18_H\tPACKAGE_PIN\t\tHR_5_12_6P } OBJ1");
  compiler_tcl_common_run("set_property gbox_mode MODE_BP_DIR_B_RX OBJ1");
  compiler_tcl_common_run("write_property property.json");
}

TEST(Constraints, check_property) {
  Compiler*& compiler = compiler_tcl_common_compiler();
  ASSERT_NE(compiler, nullptr);
  Constraints* constraints = compiler->getConstraints();
  ASSERT_NE(constraints, nullptr);
  const std::vector<OBJECT_PROPERTY> properties = constraints->get_property();
  ASSERT_EQ(properties.size(), 3);
  int index = 0;
  for (auto property : properties) {
    if (index == 0) {
      ASSERT_EQ(property.objects.size(), 2);
      ASSERT_EQ(property.objects[0], "OBJ1");
      ASSERT_EQ(property.objects[1], "OBJ2");
      ASSERT_EQ(property.properties.size(), 1);
      ASSERT_EQ(property.properties[0].name, "gbox_mode");
      ASSERT_EQ(property.properties[0].value, "MODE_BP_DIR_A_TX");
    } else if (index == 1) {
      ASSERT_EQ(property.objects.size(), 1);
      ASSERT_EQ(property.objects[0], "OBJ1");
      ASSERT_EQ(property.properties.size(), 2);
      ASSERT_EQ(property.properties[0].name, "IOSTANDARD");
      ASSERT_EQ(property.properties[0].value, "LVCMOS_18_H");
      ASSERT_EQ(property.properties[1].name, "PACKAGE_PIN");
      ASSERT_EQ(property.properties[1].value, "HR_5_12_6P");
    } else if (index == 2) {
      ASSERT_EQ(property.objects.size(), 1);
      ASSERT_EQ(property.objects[0], "OBJ1");
      ASSERT_EQ(property.properties.size(), 1);
      ASSERT_EQ(property.properties[0].name, "gbox_mode");
      ASSERT_EQ(property.properties[0].value, "MODE_BP_DIR_B_RX");
    }
    index++;
  }
}

TEST(Constraints, check_json) {
  Compiler*& compiler = compiler_tcl_common_compiler();
  ASSERT_NE(compiler, nullptr);
  Constraints* constraints = compiler->getConstraints();
  ASSERT_NE(constraints, nullptr);
  nlohmann::json instances = constraints->get_property_by_json();
  ASSERT_EQ(instances.is_array(), true);
  ASSERT_EQ(instances.size(), 2);
  // OBJ1
  ASSERT_EQ(instances[0].is_object(), true);
  ASSERT_EQ(instances[0]["name"], "OBJ1");
  ASSERT_EQ(instances[0]["defined_properties"].is_array(), true);
  ASSERT_EQ(instances[0]["defined_properties"].size(), 3);
  ASSERT_EQ(instances[0]["defined_properties"][0].size(), 1);
  ASSERT_EQ(instances[0]["defined_properties"][0]["gbox_mode"],
            "MODE_BP_DIR_A_TX");
  ASSERT_EQ(instances[0]["defined_properties"][1].size(), 2);
  ASSERT_EQ(instances[0]["defined_properties"][1]["IOSTANDARD"], "LVCMOS_18_H");
  ASSERT_EQ(instances[0]["defined_properties"][1]["PACKAGE_PIN"], "HR_5_12_6P");
  ASSERT_EQ(instances[0]["defined_properties"][2].size(), 1);
  ASSERT_EQ(instances[0]["defined_properties"][2]["gbox_mode"],
            "MODE_BP_DIR_B_RX");
  ASSERT_EQ(instances[0]["properties"].is_object(), true);
  ASSERT_EQ(instances[0]["properties"].size(), 3);
  ASSERT_EQ(instances[0]["properties"]["IOSTANDARD"], "LVCMOS_18_H");
  ASSERT_EQ(instances[0]["properties"]["PACKAGE_PIN"], "HR_5_12_6P");
  ASSERT_EQ(instances[0]["properties"]["gbox_mode"], "MODE_BP_DIR_B_RX");
  // OBJ2
  ASSERT_EQ(instances[1].is_object(), true);
  ASSERT_EQ(instances[1]["name"], "OBJ2");
  ASSERT_EQ(instances[1]["defined_properties"].is_array(), true);
  ASSERT_EQ(instances[1]["defined_properties"].size(), 1);
  ASSERT_EQ(instances[1]["defined_properties"][0].size(), 1);
  ASSERT_EQ(instances[1]["defined_properties"][0]["gbox_mode"],
            "MODE_BP_DIR_A_TX");
  ASSERT_EQ(instances[1]["properties"].is_object(), true);
  ASSERT_EQ(instances[1]["properties"]["gbox_mode"], "MODE_BP_DIR_A_TX");
}

TEST(Constraints, compare_result) {
  std::string current_dir = COMPILER_TCL_COMMON_GET_CURRENT_DIR();
  ASSERT_EQ(CFG_compare_two_text_files(
                "property.json",
                CFG_print("%s/property.golden.json", current_dir.c_str())),
            true);
}
