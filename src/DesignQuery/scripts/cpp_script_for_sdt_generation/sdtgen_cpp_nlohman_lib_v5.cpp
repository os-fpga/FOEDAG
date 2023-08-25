
//**********************************************************************************************************************************
// JSON to SDT cpp script written by ZaidTahir, for questions please email:
// zaid.butt.tahir@gmail.com or zaidt@bu.edu **************
// *********************************************************************************************************************************

#include <cstdio>
// #include <nlohmann_json/json.hpp>
#include "nlohmann_json/json.hpp"
// #include "json.hpp"
// #include "json/single_include/nlohmann/json.hpp"
// #include <json/single_include/nlohmann/json_fwd.hpp>

#include "sdtgen_cpp_nlohman_lib_v5.h"

using json = nlohmann::json;

using namespace std;

#include <fstream>
// #include <ofstream>
#include <cstring>
#include <string>
// #include <print>
#include <iostream>
#include <limits>
#include <sstream>  // header file for stringstream
#include <stdexcept>

// Global variables
string node_tab = "\t";
string subnode_tab = "\t\t";
string subsubnode_tab = "\t\t\t";

int get_soc_node(json data, SdtSocNode &sdt_soc_node_obj, int verbose) {
  if ((!data["root"].empty()) && (!data["root"]["soc"].empty())) {
    if (!data["root"]["soc"]["#size-cells"].empty()) {
      sdt_soc_node_obj.soc_size_cell = data["root"]["soc"]["#size-cells"];
      if (verbose)
        cout << "\n\nsdt_soc_node_obj.soc_size_cell = "
             << sdt_soc_node_obj.soc_size_cell << endl;
    } else {
      if (verbose) printf("soc #size-cells key-value pair not found \n");
    }

    if (!data["root"]["soc"]["#address-cells"].empty()) {
      sdt_soc_node_obj.soc_address_cell = data["root"]["soc"]["#address-cells"];
      if (verbose)
        cout << "sdt_soc_node_obj.soc_address_cell = "
             << sdt_soc_node_obj.soc_address_cell << endl;
    } else {
      if (verbose) printf("soc #address-cells key-value pair not found \n");
    }

    if (!data["root"]["soc"]["compatible"].empty()) {
      sdt_soc_node_obj.soc_compatible = data["root"]["soc"]["compatible"];
      if (verbose)
        cout << "sdt_soc_node_obj.soc_compatible = "
             << sdt_soc_node_obj.soc_compatible << endl;
    } else {
      if (verbose) printf("soc compatible key-value pair not found \n");
    }

    if (!data["root"]["soc"]["ranges"].empty()) {
      sdt_soc_node_obj.soc_ranges_key_value = data["root"]["soc"]["ranges"];
      if (verbose)
        cout << "sdt_soc_node_obj.soc_ranges_key_value = "
             << sdt_soc_node_obj.soc_ranges_key_value << endl;
    } else {
      if (verbose) printf("soc compatible key-value pair not found \n");
    }

    // this tells that the object has been populated
    sdt_soc_node_obj.object_has_been_populated = 1;
    if (verbose)
      cout << "sdt_soc_node_obj.object_has_been_populated = "
           << sdt_soc_node_obj.object_has_been_populated << endl;

    // get soc_subsystems array
    if ((!data["root"]["soc"]["soc_subsystems"].empty()) &&
        (data["root"]["soc"]["soc_subsystems"].is_array())) {
      sdt_soc_node_obj.size_soc_inst_array =
          data["root"]["soc"]["soc_subsystems"].size();
      if (verbose)
        printf("size of soc_inst_array = %d \n",
               sdt_soc_node_obj.size_soc_inst_array);
      // size of cpu insts array = 1

      // instantiate SdtSocInstSubNode ptr to ptr array
      sdt_soc_node_obj.p_soc_inst_array = (SdtSocInstSubNode **)malloc(
          sdt_soc_node_obj.size_soc_inst_array * sizeof(SdtSocInstSubNode *));

      for (int i = 0; i < sdt_soc_node_obj.size_soc_inst_array; i++) {
        string soc_subsystem_type =
            data["root"]["soc"]["soc_subsystems"][i]["subsystem"];

        sdt_soc_node_obj.p_soc_inst_array[i] =
            new SdtSocInstSubNode(soc_subsystem_type);

        sdt_soc_node_obj.p_soc_inst_array[i]->object_has_been_populated = 1;

        if (soc_subsystem_type == "interrupt-controller") {

          sdt_soc_node_obj.p_soc_inst_array[i]
              ->soc_interrupt_controller_object->object_has_been_populated = 1;

          // reading subsystem int cont id
          if ((!data["root"]["soc"]["soc_subsystems"][i]["id"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["id"].is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object->interrupt_controller_id =
                data["root"]["soc"]["soc_subsystems"][i]["id"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_id = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_id
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_id \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont phandle
          if ((!data["root"]["soc"]["soc_subsystems"][i]["phandle"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["phandle"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_phandle =
                data["root"]["soc"]["soc_subsystems"][i]["phandle"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_phandle \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont compatible
          if ((!data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_compatible =
                data["root"]["soc"]["soc_subsystems"][i]["compatible"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_compatible = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_compatible
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_compatible \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont address_cells
          if ((!data["root"]["soc"]["soc_subsystems"][i]["#address-cells"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["#address-cells"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_address_cells =
                data["root"]["soc"]["soc_subsystems"][i]["#address-cells"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_address_cells = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_address_cells
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_address_cells \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont interrupt_cells
          if ((!data["root"]["soc"]["soc_subsystems"][i]["#interrupt-cells"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["#interrupt-cells"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_interrupt_cells =
                data["root"]["soc"]["soc_subsystems"][i]["#interrupt-cells"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_interrupt_cells = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_interrupt_cells
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_interrupt_cells \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont key_value
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupt-controller"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupt-controller"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_key_value =
                data["root"]["soc"]["soc_subsystems"][i]
                    ["interrupt-controller"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_key_value = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_key_value
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_key_value \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont reg
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg"].is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object->interrupt_controller_reg =
                data["root"]["soc"]["soc_subsystems"][i]["reg"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_reg = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_reg
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_reg \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont interrupts_extended
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_interrupts_extended =
                data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_interrupts_extended = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_interrupts_extended
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_interrupts_extended \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont riscv_max_priority
          if ((!data["root"]["soc"]["soc_subsystems"][i]["riscv,max-priority"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["riscv,max-priority"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_riscv_max_priority =
                data["root"]["soc"]["soc_subsystems"][i]["riscv,max-priority"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_riscv_max_priority = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_riscv_max_priority
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_riscv_max_priority \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont riscv_ndev
          if ((!data["root"]["soc"]["soc_subsystems"][i]["riscv,ndev"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["riscv,ndev"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_riscv_ndev =
                data["root"]["soc"]["soc_subsystems"][i]["riscv,ndev"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_riscv_ndev = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_riscv_ndev
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_riscv_ndev \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont reg_address
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                   .is_array())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_address_string_array_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_address"].size();

            int array_size =
                sdt_soc_node_obj.p_soc_inst_array[i]
                    ->soc_interrupt_controller_object
                    ->interrupt_controller_reg_address_string_array_size;

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_reg_address_string_array_size = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_reg_address_string_array_size
                   << endl;

            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_address_string_array =
                new string[array_size];
            // https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value

            for (int j = 0; j < array_size; j++) {
              sdt_soc_node_obj.p_soc_inst_array[i]
                  ->soc_interrupt_controller_object
                  ->interrupt_controller_reg_address_string_array[j] =
                  data["root"]["soc"]["soc_subsystems"][i]["reg_address"][j];
            }

            for (int j = 0; j < array_size; j++) {
              if (verbose)
                cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                     << "]->soc_interrupt_controller_object->interrupt_"
                        "controller_reg_address_string_array["
                     << j << "] = "
                     << sdt_soc_node_obj.p_soc_inst_array[i]
                            ->soc_interrupt_controller_object
                            ->interrupt_controller_reg_address_string_array[j]
                     << endl;
            }

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_reg_address_string_array \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont reg_size
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_size"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_size"]
                   .is_array())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_size_string_array_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_size"].size();

            int array_size =
                sdt_soc_node_obj.p_soc_inst_array[i]
                    ->soc_interrupt_controller_object
                    ->interrupt_controller_reg_size_string_array_size;

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_reg_size_string_array_size = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_reg_size_string_array_size
                   << endl;

            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_size_string_array =
                new string[array_size];
            // https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value

            for (int j = 0; j < array_size; j++) {
              sdt_soc_node_obj.p_soc_inst_array[i]
                  ->soc_interrupt_controller_object
                  ->interrupt_controller_reg_size_string_array[j] =
                  data["root"]["soc"]["soc_subsystems"][i]["reg_size"][j];
            }

            for (int j = 0; j < array_size; j++) {
              if (verbose)
                cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                     << "]->soc_interrupt_controller_object->interrupt_"
                        "controller_reg_size_string_array["
                     << j << "] = "
                     << sdt_soc_node_obj.p_soc_inst_array[i]
                            ->soc_interrupt_controller_object
                            ->interrupt_controller_reg_size_string_array[j]
                     << endl;
            }

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_reg_size_string_array \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont reg_names
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg-names"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg-names"]
                   .is_array())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_names_string_array_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg-names"].size();

            int array_size =
                sdt_soc_node_obj.p_soc_inst_array[i]
                    ->soc_interrupt_controller_object
                    ->interrupt_controller_reg_names_string_array_size;

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_interrupt_controller_object->interrupt_"
                      "controller_reg_names_string_array_size = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_interrupt_controller_object
                          ->interrupt_controller_reg_names_string_array_size
                   << endl;

            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object
                ->interrupt_controller_reg_names_string_array =
                new string[array_size];
            // https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value

            for (int j = 0; j < array_size; j++) {
              sdt_soc_node_obj.p_soc_inst_array[i]
                  ->soc_interrupt_controller_object
                  ->interrupt_controller_reg_names_string_array[j] =
                  data["root"]["soc"]["soc_subsystems"][i]["reg-names"][j];
            }

            for (int j = 0; j < array_size; j++) {
              if (verbose)
                cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                     << "]->soc_interrupt_controller_object->interrupt_"
                        "controller_reg_names_string_array["
                     << j << "] = "
                     << sdt_soc_node_obj.p_soc_inst_array[i]
                            ->soc_interrupt_controller_object
                            ->interrupt_controller_reg_names_string_array[j]
                     << endl;
            }

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_interrupt_controller_object->interrupt_controller_reg_names_string_array \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

        } else if (soc_subsystem_type == "uart") {
          sdt_soc_node_obj.p_soc_inst_array[i]
              ->soc_uart_object->object_has_been_populated = 1;

          // reading subsystem uart id
          if ((!data["root"]["soc"]["soc_subsystems"][i]["id"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["id"].is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_uart_object->uart_id =
                data["root"]["soc"]["soc_subsystems"][i]["id"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_id \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart phandle
          if ((!data["root"]["soc"]["soc_subsystems"][i]["phandle"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["phandle"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_phandle =
                data["root"]["soc"]["soc_subsystems"][i]["phandle"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_phandle \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart compatible
          if ((!data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_compatible =
                data["root"]["soc"]["soc_subsystems"][i]["compatible"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_compatible = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_compatible
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_compatible \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart reg
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg"].is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_uart_object->uart_reg =
                data["root"]["soc"]["soc_subsystems"][i]["reg"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_reg = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_reg
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_reg \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart reg_address
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_reg_address =
                data["root"]["soc"]["soc_subsystems"][i]["reg_address"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_reg_address = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_reg_address
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_reg_address \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart reg_size
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_size"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_size"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_reg_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_size"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_reg_size = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_reg_size
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_reg_size \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart reg_shift
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg-shift"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg-shift"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_reg_shift =
                data["root"]["soc"]["soc_subsystems"][i]["reg-shift"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_reg_shift = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_reg_shift
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_reg_shift \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart status
          if ((!data["root"]["soc"]["soc_subsystems"][i]["status"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["status"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_uart_object->uart_status =
                data["root"]["soc"]["soc_subsystems"][i]["status"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_status = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_status
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_status \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart clock_frequency
          if ((!data["root"]["soc"]["soc_subsystems"][i]["clock-frequency"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["clock-frequency"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_clock_frequency =
                data["root"]["soc"]["soc_subsystems"][i]["clock-frequency"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_clock_frequency = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_clock_frequency
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_clock_frequency \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem uart interrupt_parent
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_interrupt_parent =
                data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_interrupt_parent = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_interrupt_parent
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_interrupt_parent \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem int cont interrupts
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupts"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupts"]
                   .is_array())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_interrupts_string_array_size =
                data["root"]["soc"]["soc_subsystems"][i]["interrupts"].size();

            int array_size =
                sdt_soc_node_obj.p_soc_inst_array[i]
                    ->soc_uart_object->uart_interrupts_string_array_size;

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_uart_object->uart_interrupts_string_array_size = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_uart_object->uart_interrupts_string_array_size
                   << endl;

            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_uart_object->uart_interrupts_string_array =
                new string[array_size];
            // https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value

            for (int j = 0; j < array_size; j++) {
              sdt_soc_node_obj.p_soc_inst_array[i]
                  ->soc_uart_object->uart_interrupts_string_array[j] =
                  data["root"]["soc"]["soc_subsystems"][i]["interrupts"][j];
            }

            for (int j = 0; j < array_size; j++) {
              if (verbose)
                cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                     << "]->soc_uart_object->uart_interrupts_string_array[" << j
                     << "] = "
                     << sdt_soc_node_obj.p_soc_inst_array[i]
                            ->soc_uart_object->uart_interrupts_string_array[j]
                     << endl;
            }

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_uart_object->uart_interrupts_string_array \
                         not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

        } else if (soc_subsystem_type == "gpio") {
          sdt_soc_node_obj.p_soc_inst_array[i]
              ->soc_gpio_object->object_has_been_populated = 1;


          // reading subsystem gpio id
          if ((!data["root"]["soc"]["soc_subsystems"][i]["id"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["id"].is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object->gpio_id =
                data["root"]["soc"]["soc_subsystems"][i]["id"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_gpio_object->gpio_id = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_gpio_object->gpio_id
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_id \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio phandle
          if ((!data["root"]["soc"]["soc_subsystems"][i]["phandle"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["phandle"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_phandle =
                data["root"]["soc"]["soc_subsystems"][i]["phandle"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_gpio_object->gpio_phandle = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_gpio_object->gpio_phandle
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_phandle \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio compatible
          if ((!data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_compatible =
                data["root"]["soc"]["soc_subsystems"][i]["compatible"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_compatible \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio interrupt_parent
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_interrupt_parent =
                data["root"]["soc"]["soc_subsystems"][i]["interrupt-parent"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_interrupt_parent \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio controller_key_value
          if ((!data["root"]["soc"]["soc_subsystems"][i]["gpio-controller"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["gpio-controller"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_controller_key_value =
                data["root"]["soc"]["soc_subsystems"][i]["gpio-controller"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_controller_key_value \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio reg
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg"].is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object->gpio_reg =
                data["root"]["soc"]["soc_subsystems"][i]["reg"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_reg \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio reg_size
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_size"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_size"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_reg_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_size"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_reg_size \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio reg_address
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_reg_address =
                data["root"]["soc"]["soc_subsystems"][i]["reg_address"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_gpio_object->gpio_reg_address = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_gpio_object->gpio_reg_address
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_reg_address \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio status
          if ((!data["root"]["soc"]["soc_subsystems"][i]["status"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["status"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object->gpio_status =
                data["root"]["soc"]["soc_subsystems"][i]["status"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_status \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio cells
          if ((!data["root"]["soc"]["soc_subsystems"][i]["#gpio-cells"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["#gpio-cells"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object->gpio_cells =
                data["root"]["soc"]["soc_subsystems"][i]["#gpio-cells"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_gpio_object->gpio_cells = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_gpio_object->gpio_cells
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_cells \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio ngpios
          if ((!data["root"]["soc"]["soc_subsystems"][i]["ngpios"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["ngpios"]
                   .is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object->gpio_ngpios =
                data["root"]["soc"]["soc_subsystems"][i]["ngpios"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_ngpios \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem gpio interrupts
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupts"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupts"]
                   .is_array())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_interrupts_string_array_size =
                data["root"]["soc"]["soc_subsystems"][i]["interrupts"].size();

            int array_size =
                sdt_soc_node_obj.p_soc_inst_array[i]
                    ->soc_gpio_object->gpio_interrupts_string_array_size;

            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_gpio_object->gpio_interrupts_string_array =
                new string[array_size];
            // https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value

            for (int j = 0; j < array_size; j++) {
              sdt_soc_node_obj.p_soc_inst_array[i]
                  ->soc_gpio_object->gpio_interrupts_string_array[j] =
                  data["root"]["soc"]["soc_subsystems"][i]["interrupts"][j];
            }

            for (int j = 0; j < array_size; j++) {
              if (verbose)
                cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                     << "]->soc_gpio_object->gpio_interrupts_string_array[" << j
                     << "] = "
                     << sdt_soc_node_obj.p_soc_inst_array[i]
                            ->soc_gpio_object->gpio_interrupts_string_array[j]
                     << endl;
            }

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_gpio_object->gpio_interrupts_string_array \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

        } else if (soc_subsystem_type == "syscon") {
          sdt_soc_node_obj.p_soc_inst_array[i]
              ->soc_syscon_object->object_has_been_populated = 1;

          if (verbose)
            cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                 << "]->soc_syscon_object->object_has_been_populated = " \ 
                    
                 
                 
                 
                 << sdt_soc_node_obj.p_soc_inst_array[i]
                        ->soc_syscon_object->object_has_been_populated
                 << endl;

          if (verbose)
            cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                 << "]->soc_syscon_object->syscon_subnode_name = " \ 
                    
                 
                 
                 
                 << sdt_soc_node_obj.p_soc_inst_array[i]
                        ->soc_syscon_object->syscon_subnode_name
                 << endl;

          // reading subsystem syscon id
          if ((!data["root"]["soc"]["soc_subsystems"][i]["id"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["id"].is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_syscon_object->syscon_id =
                data["root"]["soc"]["soc_subsystems"][i]["id"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_syscon_object->syscon_id = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_syscon_object->syscon_id
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_id \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon phandle
          if ((!data["root"]["soc"]["soc_subsystems"][i]["phandle"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["phandle"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_phandle =
                data["root"]["soc"]["soc_subsystems"][i]["phandle"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_syscon_object->syscon_phandle = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_syscon_object->syscon_phandle
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_phandle \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon compatible
          if ((!data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_compatible =
                data["root"]["soc"]["soc_subsystems"][i]["compatible"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_compatible \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon reg
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg"].is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_reg =
                data["root"]["soc"]["soc_subsystems"][i]["reg"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_reg \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon reg_size
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_size"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_size"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_reg_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_size"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_reg_size \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon reg_address
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_reg_address =
                data["root"]["soc"]["soc_subsystems"][i]["reg_address"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_reg_address \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem syscon status
          if ((!data["root"]["soc"]["soc_subsystems"][i]["status"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["status"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_syscon_object->syscon_status =
                data["root"]["soc"]["soc_subsystems"][i]["status"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_syscon_object->syscon_status \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

        } else if (soc_subsystem_type == "timer") {
          sdt_soc_node_obj.p_soc_inst_array[i]
              ->soc_timer_object->object_has_been_populated = 1;

          // reading subsystem timer id
          if ((!data["root"]["soc"]["soc_subsystems"][i]["id"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["id"].is_number())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_timer_object->timer_id =
                data["root"]["soc"]["soc_subsystems"][i]["id"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_id \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer phandle
          if ((!data["root"]["soc"]["soc_subsystems"][i]["phandle"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["phandle"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_phandle =
                data["root"]["soc"]["soc_subsystems"][i]["phandle"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_phandle \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer compatible
          if ((!data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["compatible"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_compatible =
                data["root"]["soc"]["soc_subsystems"][i]["compatible"];

            if (verbose)
              cout << "sdt_soc_node_obj.p_soc_inst_array[" << i
                   << "]->soc_timer_object->timer_compatible = "
                   << sdt_soc_node_obj.p_soc_inst_array[i]
                          ->soc_timer_object->timer_compatible
                   << endl;

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_compatible \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer interrupts_extended
          if ((!data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_interrupts_extended =
                data["root"]["soc"]["soc_subsystems"][i]["interrupts-extended"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_interrupts_extended \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer reg
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg"].is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]->soc_timer_object->timer_reg =
                data["root"]["soc"]["soc_subsystems"][i]["reg"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_reg \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer reg_size
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_size"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_size"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_reg_size =
                data["root"]["soc"]["soc_subsystems"][i]["reg_size"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_reg_size \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer reg_address
          if ((!data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                    .empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["reg_address"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_reg_address =
                data["root"]["soc"]["soc_subsystems"][i]["reg_address"];


          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_reg_address \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

          // reading subsystem timer status
          if ((!data["root"]["soc"]["soc_subsystems"][i]["status"].empty()) &&
              (data["root"]["soc"]["soc_subsystems"][i]["status"]
                   .is_string())) {
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_timer_object->timer_status =
                data["root"]["soc"]["soc_subsystems"][i]["status"];

          } else {
            if (verbose)
              printf(
                  "sdt_soc_node_obj.p_soc_inst_array[%d]->soc_timer_object->timer_status \
                        not found or isn't of the correct data TYPE\n",
                  i);
            // throw 50;
            // exit(0);
          }

        } else {
          if (verbose)
            cout << "****** ERROR *************** Incorrect soc_subsystem_type "
                    "read from JSON SOC node for "
                    "sdt_soc_node_obj.p_soc_inst_array["
                 << i << "]->soc_subsystem = "
                 << sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem << endl
                 << endl;
        }
      }

      if (verbose)
        printf("\n\ntotal intances of class SdtSocInstSubNode = %d\n\n",
               SdtSocInstSubNode::total_instances);
      // total intances of class SdtSocInstSubNode = 1

    } else {
      if (verbose)
        printf(
            "soc_insts in soc node is either EMPTY or is not the type ARRAY "
            "\n");
    }

    return 1;

  } else {
    if (verbose) printf("There is no soc node present in the JSON file \n");

    return 0;
  }
}

// void get_memory_node(json data, SdtMemoryNode &sdt_memory_node_obj) {
int get_memory_node(json data, SdtMemoryNode &sdt_memory_node_obj,
                    int verbose) {
  if ((!data["root"].empty()) && (!data["root"]["memory"].empty())) {
    // this tells that the object has been populated
    sdt_memory_node_obj.object_has_been_populated = 1;
    if (verbose)
      cout << " sdt_memory_node_obj.object_has_been_populated = "
           << sdt_memory_node_obj.object_has_been_populated << endl;

    sdt_memory_node_obj.sdt_memory_node_name = "memory";
    if (verbose)
      cout << " sdt_memory_node_obj.sdt_memory_node_name = "
           << sdt_memory_node_obj.sdt_memory_node_name << endl;

    // get memory instances array
    if ((!data["root"]["memory"]["memory_insts"].empty()) &&
        (data["root"]["memory"]["memory_insts"].is_array())) {
      sdt_memory_node_obj.size_memory_inst_array =
          data["root"]["memory"]["memory_insts"].size();
      if (verbose)
        printf("size of memory insts array = %d \n",
               sdt_memory_node_obj.size_memory_inst_array);

      // instantiate SdtMemoryInstSubNode ptr to ptr array
      sdt_memory_node_obj.p_memory_inst_array = (SdtMemoryInstSubNode **)malloc(
          sdt_memory_node_obj.size_memory_inst_array *
          sizeof(SdtMemoryInstSubNode *));
      // sizeof(SdtMemoryInstSubNode*) is size of ptr for this class which is
      // equal to every other ptr = 8 bytes

      // instantiate SdtMemoryInstSubNode objects and allocated memory and
      // return their pointers using new
      for (int i = 0; i < sdt_memory_node_obj.size_memory_inst_array; i++) {
        // each bin in ptr to ptr array contains a pointer that points to an
        // object of class SdtMemoryInstSubNode when we use square braces, those
        // pointers will be dereferenced
        sdt_memory_node_obj.p_memory_inst_array[i] = new SdtMemoryInstSubNode();

        // allocate cpu insts meta data to memory_inst class objects
        // p_memory_inst_array[i] contains pointers to memory_inst class objects

        sdt_memory_node_obj.p_memory_inst_array[i]->object_has_been_populated =
            1;
        if (verbose)
          cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
               << "]->object_has_been_populated = "
               << sdt_memory_node_obj.p_memory_inst_array[i]
                      ->object_has_been_populated
               << endl;
        // throw 50;
        // exit(0);

        // will use cout for variables that cause issue in printf
        if ((!data["root"]["memory"]["memory_insts"][i]["id"].empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["id"].is_number())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_id =
              data["root"]["memory"]["memory_insts"][i]["id"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_id = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]->memory_id
                 << endl;
          // displays 8 .. so I will not use int() function

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_id not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["compatible"]
                  .empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["compatible"]
                 .is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_compatible =
              data["root"]["memory"]["memory_insts"][i]["compatible"];
          if (verbose)
            cout
                << "sdt_memory_node_obj.p_memory_inst_array[" << i
                << "]->memory_compatible = "
                << sdt_memory_node_obj.p_memory_inst_array[i]->memory_compatible
                << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_"
                "compatible not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["device_type"]
                  .empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["device_type"]
                 .is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_device_type =
              data["root"]["memory"]["memory_insts"][i]["device_type"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_device_type = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]
                        ->memory_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_device_"
                "type not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["sub_device_type"]
                  .empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["sub_device_type"]
                 .is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_sub_device_type =
              data["root"]["memory"]["memory_insts"][i]["sub_device_type"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_sub_device_type = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]
                        ->memory_sub_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_sub_"
                "device_type not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["reg"].empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["reg"].is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg =
              data["root"]["memory"]["memory_insts"][i]["reg"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_reg = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_reg not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["reg_size"].empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["reg_size"]
                 .is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg_size =
              data["root"]["memory"]["memory_insts"][i]["reg_size"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_reg_size = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg_size
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_reg_size "
                "not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["memory"]["memory_insts"][i]["reg_address"]
                  .empty()) &&
            (data["root"]["memory"]["memory_insts"][i]["reg_address"]
                 .is_string())) {
          sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg_address =
              data["root"]["memory"]["memory_insts"][i]["reg_address"];
          if (verbose)
            cout << "sdt_memory_node_obj.p_memory_inst_array[" << i
                 << "]->memory_reg_address = "
                 << sdt_memory_node_obj.p_memory_inst_array[i]
                        ->memory_reg_address
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_memory_node_obj.p_memory_inst_array[%d]->memory_reg_"
                "address not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }
      }
      if (verbose)
        printf("total intances of class SdtMemoryInstSubNode = %d\n",
               SdtMemoryInstSubNode::total_instances);
      // total intances of class SdtMemoryInstSubNode = 1

    } else {
      if (verbose)
        printf(
            "memory instances array in memory node is either EMPTY or is not "
            "the type ARRAY \n");
    }

    return 1;

  } else {
    if (verbose) printf("There is no memory node present in the JSON file \n");

    return 0;
  }
}

// void get_cpus_cluster(json data, SdtCpusClusterNode
// &sdt_cpus_cluster_node_obj) {
int get_cpus_cluster_node(json data,
                          SdtCpusClusterNode &sdt_cpus_cluster_node_obj,
                          int verbose) {
  if ((!data["root"].empty()) && (!data["root"]["cpus-cluster"].empty())) {
    if (!data["root"]["cpus-cluster"]["#size-cells"].empty()) {
      sdt_cpus_cluster_node_obj.cpus_cluster_size_cell =
          data["root"]["cpus-cluster"]["#size-cells"];
      if (verbose)
        cout << " sdt_cpus_cluster_node_obj.cpus_cluster_size_cell = "
             << sdt_cpus_cluster_node_obj.cpus_cluster_size_cell << endl;
    } else {
      if (verbose)
        printf("cpus-cluster #size-cells key-value pair not found \n");
    }

    if (!data["root"]["cpus-cluster"]["#address-cells"].empty()) {
      sdt_cpus_cluster_node_obj.cpus_cluster_address_cell =
          data["root"]["cpus-cluster"]["#address-cells"];
      if (verbose)
        cout << " sdt_cpus_cluster_node_obj.cpus_cluster_address_cell = "
             << sdt_cpus_cluster_node_obj.cpus_cluster_address_cell << endl;
    } else {
      if (verbose)
        printf("cpus-cluster #address-cells key-value pair not found \n");
    }

    if (!data["root"]["cpus-cluster"]["compatible"].empty()) {
      sdt_cpus_cluster_node_obj.cpus_cluster_compatible =
          data["root"]["cpus-cluster"]["compatible"];
      if (verbose)
        cout << " sdt_cpus_cluster_node_obj.cpus_cluster_compatible = "
             << sdt_cpus_cluster_node_obj.cpus_cluster_compatible << endl;
    } else {
      if (verbose)
        printf("cpus-cluster compatible key-value pair not found \n");
    }

    // this tells that the object has been populated
    sdt_cpus_cluster_node_obj.object_has_been_populated = 1;
    if (verbose)
      cout << " sdt_cpus_cluster_node_obj.object_has_been_populated = "
           << sdt_cpus_cluster_node_obj.object_has_been_populated << endl;

    // get cpu_cluster_inst array
    if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"].empty()) &&
        (data["root"]["cpus-cluster"]["cpu_cluster_insts"].is_array())) {
      sdt_cpus_cluster_node_obj.size_cpu_cluster_inst_array =
          data["root"]["cpus-cluster"]["cpu_cluster_insts"].size();
      if (verbose)
        printf("size of cpu_cluster insts array = %d \n",
               sdt_cpus_cluster_node_obj.size_cpu_cluster_inst_array);
      // size of cpu insts array = 1

      // instantiate SdtCpuClusterInstSubNode ptr to ptr array
      sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array =
          (SdtCpuClusterInstSubNode **)malloc(
              sdt_cpus_cluster_node_obj.size_cpu_cluster_inst_array *
              sizeof(SdtCpuClusterInstSubNode *));
      // sizeof(SdtCpuClusterInstSubNode*) is size of ptr for this class which
      // is equal to every other ptr = 8 bytes

      // instantiate SdtCpuClusterInstSubNode objects and allocated memory and
      // return their pointers using new
      for (int i = 0; i < sdt_cpus_cluster_node_obj.size_cpu_cluster_inst_array;
           i++) {
        // each bin in ptr to ptr array contains a pointer that points to an
        // object of class SdtCpuClusterInstSubNode when we use square braces,
        // those pointers will be dereferenced
        sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i] =
            new SdtCpuClusterInstSubNode();

        // allocate cpu insts meta data to cpu_cluster_inst class objects
        // p_cpu_cluster_inst_array[i] contains pointers to cpu_cluster_inst
        // class objects

        sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
            ->object_has_been_populated = 1;
        if (verbose)
          cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
               << "]->object_has_been_populated = "
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->object_has_been_populated
               << endl;
        // throw 50;
        // exit(0);

        // will use cout for variables that cause issue in printf
        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["id"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["id"]
                 .is_number())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_id =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["id"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_id = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_id
                 << endl;
          // displays 8 .. so I will not use int() function

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "id not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["compatible"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["compatible"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_compatible =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["compatible"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_compatible = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_compatible
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_compatible not found or isn't of the correct data "
                "TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["device_type"]
                      .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["device_type"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_device_type =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["device_type"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_device_type = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_device_type not found or isn't of the correct data "
                "TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["sub_device_type"]
                      .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                 ["sub_device_type"]
                     .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_sub_device_type =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["sub_device_type"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_sub_device_type = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_sub_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_sub_device_type not found or isn't of the correct "
                "data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_reg =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_reg = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_reg
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_reg not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg_size"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg_size"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_reg_size =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg_size"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_reg_size = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_reg_size
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_reg_size not found or isn't of the correct data "
                "TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["reg_address"]
                      .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["reg_address"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_reg_address =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]
                  ["reg_address"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_reg_address = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_reg_address
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_reg_address not found or isn't of the correct data "
                "TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["status"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["status"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_status =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["status"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_status = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_status
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "cluster_status not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["riscv,isa"]
                  .empty()) &&
            (data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["riscv,isa"]
                 .is_string())) {
          sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->cpu_cluster_riscv_isa =
              data["root"]["cpus-cluster"]["cpu_cluster_insts"][i]["riscv,isa"];
          if (verbose)
            cout << "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[" << i
                 << "]->cpu_cluster_riscv_isa = "
                 << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                        ->cpu_cluster_riscv_isa
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[%d]->cpu_"
                "riscv_isa not found or isn't of the correct data TYPE \n",
                i);
          // throw 50;
          // exit(0);
        }
      }
      if (verbose)
        printf("total intances of class SdtCpuClusterInstSubNode = %d\n",
               SdtCpuClusterInstSubNode::total_instances);
      // total intances of class SdtCpuClusterInstSubNode = 1

    } else {
      if (verbose)
        printf(
            "cpus_cluster_insts in cpus-cluster node is either EMPTY or is not "
            "the type ARRAY \n");
    }

    return 1;

  } else {
    if (verbose)
      printf("There is no cpus_cluster node present in the JSON file \n");

    return 0;
  }
}

// void get_rootmetadata_node (json data, SdtRootMetaDataNode
// &sdt_rootmetadata_node_obj) {
int get_rootmetadata_node(json data,
                          SdtRootMetaDataNode &sdt_rootmetadata_node_obj,
                          int verbose) {
  if ((!data["root"].empty()) && (!data["root"]["sdt_root_metadata"].empty())) {
    if (!data["root"]["sdt_root_metadata"]["#size-cells"].empty()) {
      sdt_rootmetadata_node_obj.rootmetadata_size_cell =
          data["root"]["sdt_root_metadata"]["#size-cells"];
      if (verbose)
        cout << " sdt_rootmetadata_node_obj.rootmetadata_size_cell = "
             << sdt_rootmetadata_node_obj.rootmetadata_size_cell << endl;
    } else {
      if (verbose)
        printf("rootmetadata #size-cells key-value pair not found \n");
    }

    if (!data["root"]["sdt_root_metadata"]["#address-cells"].empty()) {
      sdt_rootmetadata_node_obj.rootmetadata_address_cell =
          data["root"]["sdt_root_metadata"]["#address-cells"];
      if (verbose)
        cout << " sdt_rootmetadata_node_obj.rootmetadata_address_cell = "
             << sdt_rootmetadata_node_obj.rootmetadata_address_cell << endl;
    } else {
      if (verbose)
        printf("rootmetadata #address-cells key-value pair not found \n");
    }

    if (!data["root"]["sdt_root_metadata"]["compatible"].empty()) {
      sdt_rootmetadata_node_obj.rootmetadata_compatible =
          data["root"]["sdt_root_metadata"]["compatible"];
      if (verbose)
        cout << " sdt_rootmetadata_node_obj.rootmetadata_compatible = "
             << sdt_rootmetadata_node_obj.rootmetadata_compatible << endl;
    } else {
      if (verbose)
        printf("rootmetadata compatible key-value pair not found \n");
    }

    if (!data["root"]["sdt_root_metadata"]["model"].empty()) {
      sdt_rootmetadata_node_obj.rootmetadata_model =
          data["root"]["sdt_root_metadata"]["model"];
      if (verbose)
        cout << " sdt_rootmetadata_node_obj.rootmetadata_model = "
             << sdt_rootmetadata_node_obj.rootmetadata_model << endl;
    } else {
      if (verbose) printf("rootmetadata model key-value pair not found \n");
    }

    // this tells that the object has been populated
    sdt_rootmetadata_node_obj.object_has_been_populated = 1;
    if (verbose)
      cout << " sdt_rootmetadata_node_obj.object_has_been_populated = "
           << sdt_rootmetadata_node_obj.object_has_been_populated << endl;

    sdt_rootmetadata_node_obj.rootmetadata_node_name = "sdt_root_metadata";
    if (verbose)
      cout << " sdt_rootmetadata_node_obj.rootmetadata_node_name = "
           << sdt_rootmetadata_node_obj.rootmetadata_node_name << endl;
    if (verbose) cout << endl;

    return 1;

  } else {
    if (verbose)
      printf("There is no rootmetadata node present in the JSON file \n");

    return 0;
  }
}

// void get_cpus (json data, SdtCpusNode &sdt_cpus_node_obj) {
int get_cpus_node(json data, SdtCpusNode &sdt_cpus_node_obj, int verbose) {
  if ((!data["root"].empty()) && (!data["root"]["cpus"].empty())) {
    if (!data["root"]["cpus"]["#address-cells"].empty()) {
      sdt_cpus_node_obj.cpus_size_cell = data["root"]["cpus"]["#size-cells"];
      if (verbose)
        cout << " sdt_cpus_node_obj.cpus_size_cell = "
             << sdt_cpus_node_obj.cpus_size_cell << endl;
    } else {
      if (verbose) printf("cpus #size-cells key-value pair not found \n");
    }

    if (!data["root"]["cpus"]["#address-cells"].empty()) {
      sdt_cpus_node_obj.cpus_address_cell =
          data["root"]["cpus"]["#address-cells"];
      if (verbose)
        cout << " sdt_cpus_node_obj.cpus_address_cell = "
             << sdt_cpus_node_obj.cpus_address_cell << endl;
    } else {
      if (verbose) printf("cpus #address-cells key-value pair not found \n");
    }

    if (!data["root"]["cpus"]["timebase-frequency"].empty()) {
      sdt_cpus_node_obj.cpus_timebase_frequency =
          data["root"]["cpus"]["timebase-frequency"];
      if (verbose)
        cout << " sdt_cpus_node_obj.cpus_timebase_frequency = "
             << sdt_cpus_node_obj.cpus_timebase_frequency << endl;
    } else {
      if (verbose)
        printf("cpus timebase-frequency key-value pair not found \n");
    }

    // this tells that the object has been populated
    sdt_cpus_node_obj.object_has_been_populated = 1;
    if (verbose)
      cout << " sdt_cpus_node_obj.object_has_been_populated = "
           << sdt_cpus_node_obj.object_has_been_populated << endl;

    sdt_cpus_node_obj.cpus_node_name = "cpus";
    if (verbose)
      cout << " sdt_cpus_node_obj.cpus_node_name = "
           << sdt_cpus_node_obj.cpus_node_name << endl;

    // get cpu_insts array
    if ((!data["root"]["cpus"]["cpu_insts"].empty()) &&
        (data["root"]["cpus"]["cpu_insts"].is_array())) {
      sdt_cpus_node_obj.size_cpu_inst_array =
          data["root"]["cpus"]["cpu_insts"].size();

      if (verbose)
        printf("size of cpu insts array = %d \n",
               sdt_cpus_node_obj.size_cpu_inst_array);
      // size of cpu insts array = 1

      // instantiate SdtCpuInsts ptr to ptr array
      sdt_cpus_node_obj.p_cpu_inst_array = (SdtCpuInstSubNode **)malloc(
          sdt_cpus_node_obj.size_cpu_inst_array * sizeof(SdtCpuInstSubNode *));

      // instantiate SdtCpuInstSubNode objects and allocated memory and return
      // their pointers using new
      for (int i = 0; i < sdt_cpus_node_obj.size_cpu_inst_array; i++) {
        sdt_cpus_node_obj.p_cpu_inst_array[i] = new SdtCpuInstSubNode();

        sdt_cpus_node_obj.p_cpu_inst_array[i]->object_has_been_populated = 1;
        if (verbose)
          cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
               << "]->object_has_been_populated = "
               << sdt_cpus_node_obj.p_cpu_inst_array[i]
                      ->object_has_been_populated
               << endl;
        // throw 50;
        // exit(0);

        sdt_cpus_node_obj.p_cpu_inst_array[i]->sdt_cpu_inst_subnode_name =
            "cpu";
        if (verbose)
          cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
               << "]->sdt_cpu_inst_subnode_name = "
               << sdt_cpus_node_obj.p_cpu_inst_array[i]
                      ->sdt_cpu_inst_subnode_name
               << endl;

        // will use cout for variables that cause issue in printf
        // removing int(), c_str() and printf statements below. Also no need to
        // throw errors or exist if a key value pair isnt found
        if ((!data["root"]["cpus"]["cpu_insts"][i]["id"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["id"].is_number())) {
          // sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_id =
          // int(data["root"]["cpus"]["cpu_insts"][i]["id"]);
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_id =
              data["root"]["cpus"]["cpu_insts"][i]["id"];

          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i << "]->cpu_id = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_id << endl;
          // displays 8 .. so I will not use int() function

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_id not found or "
                "isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["clock-frequency"]
                  .empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["clock-frequency"]
                 .is_number())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_clock_frequency =
              data["root"]["cpus"]["cpu_insts"][i]["clock-frequency"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_clock_frequency = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_clock_frequency
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_clock_frequency "
                "not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["i-cache-line-size"]
                  .empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["i-cache-line-size"]
                 .is_number())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_i_cache_line_size =
              int(data["root"]["cpus"]["cpu_insts"][i]["i-cache-line-size"]);
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_i_cache_line_size = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_i_cache_line_size
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_i_cache_line_size "
                "not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["d-cache-line-size"]
                  .empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["d-cache-line-size"]
                 .is_number())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_d_cache_line_size =
              data["root"]["cpus"]["cpu_insts"][i]["d-cache-line-size"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_d_cache_line_size = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_d_cache_line_size
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_d_cache_line_size "
                "not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["device_type"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["device_type"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_device_type =
              data["root"]["cpus"]["cpu_insts"][i]["device_type"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_device_type = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_device_type not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["sub_device_type"]
                  .empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["sub_device_type"]
                 .is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_sub_device_type =
              data["root"]["cpus"]["cpu_insts"][i]["sub_device_type"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_sub_device_type = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_sub_device_type
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_sub_device_type "
                "not found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["reg"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["reg"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg =
              data["root"]["cpus"]["cpu_insts"][i]["reg"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_reg = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_reg not found or "
                "isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["reg_size"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["reg_size"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg_size =
              data["root"]["cpus"]["cpu_insts"][i]["reg_size"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_reg_size = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg_size << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_reg_size not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["reg_address"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["reg_address"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg_address =
              data["root"]["cpus"]["cpu_insts"][i]["reg_address"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_reg_address = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg_address
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_reg_address not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["status"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["status"].is_string())) {
          // sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_status =
          // data["root"]["cpus"]["cpu_insts"][i]["status"].get<std::string>().c_str();
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_status =
              data["root"]["cpus"]["cpu_insts"][i]["status"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_status = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_status << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_status not found "
                "or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["compatible"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["compatible"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_compatible =
              data["root"]["cpus"]["cpu_insts"][i]["compatible"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_compatible = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_compatible
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_compatible not "
                "found or isn't of the correct data TYPE \n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["riscv,isa"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["riscv,isa"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_riscv_isa =
              data["root"]["cpus"]["cpu_insts"][i]["riscv,isa"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_riscv_isa = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_riscv_isa
                 << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_riscv_isa not "
                "found or isn't of the correct data TYPE \n",
                i);
          // throw 50;
          // exit(0);
        }

        if ((!data["root"]["cpus"]["cpu_insts"][i]["mmu-type"].empty()) &&
            (data["root"]["cpus"]["cpu_insts"][i]["mmu-type"].is_string())) {
          sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_mmu_type =
              data["root"]["cpus"]["cpu_insts"][i]["mmu-type"];
          if (verbose)
            cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                 << "]->cpu_mmu_type = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_mmu_type << endl;

        } else {
          if (verbose)
            printf(
                "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_mmu_type not "
                "found or isn't of the correct data TYPE\n",
                i);
          // throw 50;
          // exit(0);
        }

        // testing for outputs from cpus ints interrupt controller
        if (verbose)
          cout << "Interrupt controller value for data[root][cpus][cpu_insts]["
               << i << "][interrupt-controller].empty() = "
               << data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                      .empty()
               << endl;

        // populating cpu insts interrupt controller object if it exists
        if (!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                 .empty()) {
          // cpu_int_cont_data is object of class
          // "SdtCpuInstSubNodeInterruptControllerData" initialized with each
          // cpu insts object

          sdt_cpus_node_obj.p_cpu_inst_array[i]
              ->cpu_int_cont_data.cpu_inst_subnode_int_cont_node_name =
              "interrupt-controller";
          if (verbose)
            cout
                << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                << "]->cpu_int_cont_data.cpu_inst_subnode_int_cont_node_name = "
                << sdt_cpus_node_obj.p_cpu_inst_array[i]
                       ->cpu_int_cont_data.cpu_inst_subnode_int_cont_node_name
                << endl;

          sdt_cpus_node_obj.p_cpu_inst_array[i]
              ->cpu_int_cont_data.object_has_been_populated = 1;
          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["id"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["id"]
                       .is_number())) {
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_id =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["id"];
            if (verbose)
              cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                   << "]->cpu_int_cont_data.int_cont_id = "
                   << sdt_cpus_node_obj.p_cpu_inst_array[i]
                          ->cpu_int_cont_data.int_cont_id
                   << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_id not found or isn't of the correct data TYPE\n",
                  i);
          }

          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["phandle"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["phandle"]
                       .is_string())) {
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_phandle =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["phandle"];
            if (verbose)
              cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                   << "]->cpu_int_cont_data.int_cont_phandle = "
                   << sdt_cpus_node_obj.p_cpu_inst_array[i]
                          ->cpu_int_cont_data.int_cont_phandle
                   << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_phandle not found or isn't of the correct data "
                  "TYPE\n",
                  i);
          }

          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["compatible"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["compatible"]
                       .is_string())) {
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_compatible =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["compatible"];
            if (verbose)
              cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                   << "]->cpu_int_cont_data.int_cont_compatible = "
                   << sdt_cpus_node_obj.p_cpu_inst_array[i]
                          ->cpu_int_cont_data.int_cont_compatible
                   << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_compatible not found or isn't of the correct data "
                  "TYPE\n",
                  i);
          }

          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["#address-cells"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["#address-cells"]
                       .is_string())) {
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_address_cells =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["#address-cells"];
            if (verbose)
              cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                   << "]->cpu_int_cont_data.int_cont_address_cells = "
                   << sdt_cpus_node_obj.p_cpu_inst_array[i]
                          ->cpu_int_cont_data.int_cont_address_cells
                   << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_address_cells not found or isn't of the correct "
                  "data TYPE\n",
                  i);
          }

          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["#interrupt-cells"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["#interrupt-cells"]
                       .is_string())) {
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_interrupt_cells =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["#interrupt-cells"];
            if (verbose)
              cout << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                   << "]->cpu_int_cont_data.int_cont_interrupt_cells = "
                   << sdt_cpus_node_obj.p_cpu_inst_array[i]
                          ->cpu_int_cont_data.int_cont_interrupt_cells
                   << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_interrupt_cells not found or isn't of the correct "
                  "data TYPE\n",
                  i);
          }

          if ((!data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["interrupt-controller"]
                        .empty()) &&
              (data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                   ["interrupt-controller"]
                       .is_string())) {
            // even though this key "interrupt-controller" has an empty string
            // value "", but it is not empty. If this key didn't exist at all,
            // then the empty() func wouldve returned 1.
            sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.int_cont_interrupt_controller_key =
                data["root"]["cpus"]["cpu_insts"][i]["interrupt-controller"]
                    ["interrupt-controller"];
            if (verbose)
              cout
                  << "sdt_cpus_node_obj.p_cpu_inst_array[" << i
                  << "]->cpu_int_cont_data.int_cont_interrupt_controller_key = "
                  << sdt_cpus_node_obj.p_cpu_inst_array[i]
                         ->cpu_int_cont_data.int_cont_interrupt_controller_key
                  << endl;
          } else {
            if (verbose)
              printf(
                  "sdt_cpus_node_obj.p_cpu_inst_array[%d]->cpu_int_cont_data."
                  "int_cont_interrupt_controller_key not found or isn't of the "
                  "correct data TYPE\n",
                  i);
          }

        } else {
          if (verbose)
            printf(
                " data[\"root\"][\"cpus\"][\"cpu_insts\"][%d][\"interrupt-"
                "controller\"] is EMPTY \n",
                i);
        }
      }
      if (verbose)
        printf("total intances of class SdtCpuInstSubNode = %d\n",
               SdtCpuInstSubNode::total_instances);
      // total intances of class SdtCpuInstSubNode = 1

    } else {
      if (verbose)
        printf(
            "cpu_insts in cpus node is either EMPTY or is not the type ARRAY "
            "\n");
    }

    return 1;

  } else {
    if (verbose) printf("There is no cpus node present in the JSON file \n");

    return 0;
  }
}

string return_string_from_ofstream_file(ofstream &outfile, string file_path) {
  // first we flush the outfile
  outfile.flush();

  int size;

  ifstream inputFile;

  inputFile.open(file_path);

  // calculating size of inputFile
  inputFile.seekg(0, inputFile.end);
  size = inputFile.tellg();
  inputFile.seekg(0, inputFile.beg);

  // allocate memory:
  char *data_file = new char[size];

  // read data as a block:
  inputFile.read(data_file, size);

  inputFile.close();

  // printf("Printing the read JSON file file \n");

  // print content:
  // cout.write (data_file, size);

  string data_file_string = data_file;
  // string data_file_string = string(data_file);

  return data_file_string;
}

// passing &output by ref cx we wana change it
// string gen_rootmetadata_node(ofstream &outfile, SdtRootMetaDataNode
// sdt_rootmetadata_node_obj, int verbose) {
int gen_rootmetadata_node(ofstream &outfile,
                          SdtRootMetaDataNode sdt_rootmetadata_node_obj,
                          int verbose) {
  stringstream buffer;
  // https://stackoverflow.com/questions/5193173/getting-cout-output-to-a-stdstring

  if (sdt_rootmetadata_node_obj.object_has_been_populated) {
    // outfile << node_tab << "compatible = " << "\"" <<
    // sdt_rootmetadata_node_obj.rootmetadata_compatible << "\";" << endl;
    buffer << node_tab << "compatible = "
           << "\"" << sdt_rootmetadata_node_obj.rootmetadata_compatible << "\";"
           << endl;

    // outfile << node_tab << "#address-cells = " <<
    // sdt_rootmetadata_node_obj.rootmetadata_address_cell << ";" << endl;
    buffer << node_tab << "#address-cells = "
           << sdt_rootmetadata_node_obj.rootmetadata_address_cell << ";"
           << endl;

    // outfile << node_tab << "#size-cells = " <<
    // sdt_rootmetadata_node_obj.rootmetadata_size_cell << ";" << endl;
    buffer << node_tab << "#size-cells = "
           << sdt_rootmetadata_node_obj.rootmetadata_size_cell << ";" << endl;

    // outfile << node_tab << "model = " << "\"" <<
    // sdt_rootmetadata_node_obj.rootmetadata_model << "\";" << endl;
    buffer << node_tab << "model = "
           << "\"" << sdt_rootmetadata_node_obj.rootmetadata_model << "\";"
           << endl;

    outfile << buffer.str();
    // works!

    if (verbose == 1) {
      cout << "gen_rootmetadata_node output = \n\n" << buffer.str() << endl;
    }

    return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!!\nRootmetadata node hasn't been populated "
              "yet!!!\n\n"
           << endl;

    return 0;
  }

  // return buffer.str();
}

// string gen_cpus_cluster_node(ofstream &outfile, SdtCpusClusterNode
// sdt_cpus_cluster_node_obj, int verbose) {
int gen_cpus_cluster_node(ofstream &outfile,
                          SdtCpusClusterNode sdt_cpus_cluster_node_obj,
                          int verbose) {
  stringstream buffer;

  if (sdt_cpus_cluster_node_obj.object_has_been_populated) {
    buffer << endl << node_tab << "/* Boot CPU configuration */\n" << endl;

    buffer << node_tab << sdt_cpus_cluster_node_obj.cpus_cluster_phandle << ": "
           << sdt_cpus_cluster_node_obj.cpus_cluster_node_name \ 
        << " {"
           << endl;

    buffer << node_tab << node_tab << "#address-cells = "
           << sdt_cpus_cluster_node_obj.cpus_cluster_address_cell << ";"
           << endl;
    buffer << node_tab << node_tab << "#size-cells = "
           << sdt_cpus_cluster_node_obj.cpus_cluster_size_cell << ";" << endl;
    buffer << node_tab << node_tab << "compatible = "
           << sdt_cpus_cluster_node_obj.cpus_cluster_compatible << ";" << endl;

    for (int i = 0; i < sdt_cpus_cluster_node_obj.size_cpu_cluster_inst_array;
         i++) {
      if (sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
              ->object_has_been_populated) {
        buffer << node_tab << node_tab
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_sub_device_type
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_id
               << ": "
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_device_type
               << "@"
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_reg_address
               << " {" << endl;

        buffer << node_tab << node_tab << node_tab << "compatible = "
               << "\""
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_compatible
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "reg = "
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_reg
               << ";" << endl;

        buffer << node_tab << node_tab << node_tab << "status = "
               << "\""
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_status
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "riscv,isa = "
               << "\""
               << sdt_cpus_cluster_node_obj.p_cpu_cluster_inst_array[i]
                      ->cpu_cluster_riscv_isa
               << "\";" << endl;

        buffer << node_tab << node_tab << "};" << endl;

      } else {
        if (verbose)
          cout << "\nWarning!!!!\nsdt_cpus_cluster_node_obj->p_cpu_cluster_"
                  "inst_array["
               << i << "] object has not been populated!!! \n\n";
      }
    }

    buffer << node_tab << "};" << endl;

    outfile << buffer.str();

    if (verbose == 1) {
      cout << "gen_cpus_cluster_node output = \n\n" << buffer.str() << endl;
    }

    return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!!\nCPUS-Cluster node hasn't been populated "
              "yet!!!\n\n"
           << endl;

    return 0;
  }

  // return buffer.str();
}

string gen_soc_subsystem_timer(stringstream &buffer_out,
                               SdtSocInstSubNodeTimer *soc_timer_object,
                               int verbose) {
  stringstream buffer;

  if (soc_timer_object->object_has_been_populated) {
    buffer << endl
           << node_tab << node_tab << soc_timer_object->timer_phandle << ": "
           << soc_timer_object->timer_subnode_name << "@"
           << soc_timer_object->timer_reg_address << " {" << endl;

    buffer << node_tab << node_tab << node_tab << "compatible = \""
           << soc_timer_object->timer_compatible << "\";\n";
    buffer << node_tab << node_tab << node_tab
           << "reg = " << soc_timer_object->timer_reg << ";\n";
    buffer << node_tab << node_tab << node_tab << "interrupts-extended = "
           << soc_timer_object->timer_interrupts_extended << ";\n";
    buffer << node_tab << node_tab << node_tab << "status = \""
           << soc_timer_object->timer_status << "\";\n";

    buffer << node_tab << node_tab << "};" << endl;

    buffer_out << buffer.str();  // works

    if (verbose == 1) {
      // cout << "gen_soc_subsystem_timer output = \n\n" << buffer.str() <<
      // endl;
    }

  } else {
    if (verbose)
      cout << "\n\nWarning!!! soc_timer_object hasn't been populated in "
              "gen_soc_subsystem_timer in gen_soc_node \n\n";
  }

  return buffer.str();
}

string gen_soc_subsystem_syscon(stringstream &buffer_out,
                                SdtSocInstSubNodeSyscon *soc_syscon_object,
                                int verbose) {
  stringstream buffer;

  if (soc_syscon_object->object_has_been_populated) {
    buffer << endl
           << node_tab << node_tab << soc_syscon_object->syscon_phandle << ": "
           << soc_syscon_object->syscon_subnode_name << "@"
           << soc_syscon_object->syscon_reg_address << " {" << endl;

    buffer << node_tab << node_tab << node_tab << "compatible = \""
           << soc_syscon_object->syscon_compatible << "\";\n";
    buffer << node_tab << node_tab << node_tab
           << "reg = " << soc_syscon_object->syscon_reg << ";\n";
    buffer << node_tab << node_tab << node_tab << "status = \""
           << soc_syscon_object->syscon_status << "\";\n";

    buffer << node_tab << node_tab << "};" << endl;

    buffer_out << buffer.str();  // works

    if (verbose == 1) {
      // cout << "gen_soc_subsystem_syscon output = \n\n" << buffer.str() <<
      // endl;
    }

  } else {
    if (verbose)
      cout << "\n\nWarning!!! soc_syscon_object hasn't been populated in "
              "gen_soc_subsystem_syscon in gen_soc_node \n\n";
  }

  return buffer.str();
}

string gen_soc_subsystem_gpio(stringstream &buffer_out,
                              SdtSocInstSubNodeGpio *soc_gpio_object,
                              int verbose) {
  stringstream buffer;

  if (soc_gpio_object->object_has_been_populated) {
    buffer << endl
           << node_tab << node_tab << soc_gpio_object->gpio_phandle << ": "
           << soc_gpio_object->gpio_subnode_name << "@"
           << soc_gpio_object->gpio_reg_address << " {" << endl;

    buffer << node_tab << node_tab << node_tab << "compatible = \""
           << soc_gpio_object->gpio_compatible << "\";\n";
    buffer << node_tab << node_tab << node_tab
           << "reg = " << soc_gpio_object->gpio_reg << ";\n";

    buffer << node_tab << node_tab << node_tab << "interrupts = <";
    for (int i = 0; i < soc_gpio_object->gpio_interrupts_string_array_size;
         i++) {
      if (i < (soc_gpio_object->gpio_interrupts_string_array_size - 1)) {
        buffer << soc_gpio_object->gpio_interrupts_string_array[i] << " ";
      } else {
        buffer << soc_gpio_object->gpio_interrupts_string_array[i] << ">;\n";
      }
    }

    buffer << node_tab << node_tab << node_tab << "interrupt-parent = <"
           << soc_gpio_object->gpio_interrupt_parent << ">;\n";

    if (soc_gpio_object->gpio_controller_key_value == "") {
      buffer << node_tab << node_tab << node_tab << "gpio-controller;"
             << "\n";
    }

    buffer << node_tab << node_tab << node_tab << "ngpios = <"
           << soc_gpio_object->gpio_ngpios << ">;\n";
    buffer << node_tab << node_tab << node_tab << "#gpio-cells = <"
           << soc_gpio_object->gpio_cells << ">;\n";
    buffer << node_tab << node_tab << node_tab << "status = \""
           << soc_gpio_object->gpio_status << "\";\n";

    buffer << node_tab << node_tab << "};" << endl;

    buffer_out << buffer.str();  // works

    if (verbose == 1) {
      // cout << "gen_soc_subsystem_gpio output = \n\n" << buffer.str() << endl;
    }

  } else {
    if (verbose)
      cout << "\n\nWarning!!! soc_gpio_object hasn't been populated in "
              "gen_soc_subsystem_gpio in gen_soc_node \n\n";
  }

  return buffer.str();
}

string gen_soc_subsystem_uart(stringstream &buffer_out,
                              SdtSocInstSubNodeUart *soc_uart_object,
                              int verbose) {
  stringstream buffer;

  if (soc_uart_object->object_has_been_populated) {
    buffer << endl
           << node_tab << node_tab << soc_uart_object->uart_phandle << ": "
           << soc_uart_object->uart_subnode_name << "@"
           << soc_uart_object->uart_reg_address << " {" << endl;

    buffer << node_tab << node_tab << node_tab << "compatible = \""
           << soc_uart_object->uart_compatible << "\";\n";
    buffer << node_tab << node_tab << node_tab
           << "reg = " << soc_uart_object->uart_reg << ";\n";

    buffer << node_tab << node_tab << node_tab << "interrupts = <";
    for (int i = 0; i < soc_uart_object->uart_interrupts_string_array_size;
         i++) {
      if (i < (soc_uart_object->uart_interrupts_string_array_size - 1)) {
        buffer << soc_uart_object->uart_interrupts_string_array[i] << " ";
      } else {
        buffer << soc_uart_object->uart_interrupts_string_array[i] << ">;\n";
      }
    }

    buffer << node_tab << node_tab << node_tab << "interrupt-parent = <"
           << soc_uart_object->uart_interrupt_parent << ">;\n";
    buffer << node_tab << node_tab << node_tab << "clock-frequency = <"
           << soc_uart_object->uart_clock_frequency << ">;\n";
    buffer << node_tab << node_tab << node_tab << "reg-shift = <"
           << soc_uart_object->uart_reg_shift << ">;\n";
    buffer << node_tab << node_tab << node_tab << "status = \""
           << soc_uart_object->uart_status << "\";\n";

    buffer << node_tab << node_tab << "};" << endl;

    // buffer_out = buffer;  // error
    buffer_out << buffer.str();  // works

    if (verbose == 1) {
      // cout << "gen_soc_subsystem_uart output = \n\n" << buffer.str() << endl;
    }

  } else {
    if (verbose)
      cout << "\n\nWarning!!! soc_uart_object hasn't been populated in "
              "gen_soc_subsystem_uart in gen_soc_node \n\n";
  }

  return buffer.str();
}

string gen_soc_subsystem_int_controller(
    stringstream &buffer_out,
    SdtSocInstSubNodeInterruptController *soc_interrupt_controller_object,
    int verbose) {
  // int gen_soc_subsystem_int_controller(stringstream &buffer_out,
  // SdtSocInstSubNodeInterruptController * soc_interrupt_controller_object, int
  // verbose) {

  stringstream buffer;

  if (soc_interrupt_controller_object->object_has_been_populated) {
    buffer << node_tab << node_tab
           << soc_interrupt_controller_object->interrupt_controller_phandle
           << ": "
           << soc_interrupt_controller_object->interrupt_controller_subnode_name
           << "@"
           << soc_interrupt_controller_object
                  ->interrupt_controller_reg_address_string_array[0]
           << " {" << endl;
    // using interrupt_controller_reg_address_string_array[0] idx 0 so that
    // first reg address is used as the node address

    buffer << node_tab << node_tab << node_tab << "compatible = \""
           << soc_interrupt_controller_object->interrupt_controller_compatible
           << "\";\n";
    buffer
        << node_tab << node_tab << node_tab << "#address-cells = "
        << soc_interrupt_controller_object->interrupt_controller_address_cells
        << ";\n";
    buffer
        << node_tab << node_tab << node_tab << "#interrupt-cells = "
        << soc_interrupt_controller_object->interrupt_controller_interrupt_cells
        << ";\n";
    if (soc_interrupt_controller_object->interrupt_controller_key_value == "") {
      buffer << node_tab << node_tab << node_tab << "interrupt-controller;\n";
    }
    buffer << node_tab << node_tab << node_tab << "reg = "
           << soc_interrupt_controller_object->interrupt_controller_reg
           << ";\n";

    buffer << node_tab << node_tab << node_tab << "reg-names = ";
    for (int i = 0; i < soc_interrupt_controller_object
                            ->interrupt_controller_reg_names_string_array_size;
         i++) {
      if (i < (soc_interrupt_controller_object
                   ->interrupt_controller_reg_names_string_array_size -
               1)) {
        buffer << "\""
               << soc_interrupt_controller_object
                      ->interrupt_controller_reg_names_string_array[i]
               << "\",";
      } else {
        buffer << "\""
               << soc_interrupt_controller_object
                      ->interrupt_controller_reg_names_string_array[i]
               << "\";\n";
      }
    }

    buffer << node_tab << node_tab << node_tab << "riscv,max-priority = <"
           << soc_interrupt_controller_object
                  ->interrupt_controller_riscv_max_priority
           << ">;\n";
    buffer << node_tab << node_tab << node_tab << "riscv,ndev = <"
           << soc_interrupt_controller_object->interrupt_controller_riscv_ndev
           << ">;\n";
    buffer << node_tab << node_tab << node_tab << "interrupts-extended = <"
           << soc_interrupt_controller_object
                  ->interrupt_controller_interrupts_extended
           << ">;\n";

    buffer << node_tab << node_tab << "};" << endl;

    // buffer_out = buffer;  // error
    buffer_out << buffer.str();  // works

    if (verbose == 1) {
      // cout << "gen_soc_subsystem_int_controller output = \n\n" <<
      // buffer.str() << endl;
    }

    // return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!! soc_interrupt_controller_object hasn't been "
              "populated in gen_soc_subsystem_int_controller in gen_soc_node "
              "\n\n";

    // return 0;
  }

  return buffer.str();
}

// string gen_soc_node(ofstream &outfile, SdtSocNode sdt_soc_node_obj, int
// verbose) {
int gen_soc_node(ofstream &outfile, SdtSocNode sdt_soc_node_obj, int verbose) {
  stringstream buffer;
  string generated_soc_subsystem_string;

  if ((sdt_soc_node_obj.object_has_been_populated) &&
      (sdt_soc_node_obj.size_soc_inst_array > 0)) {
    buffer << endl << node_tab << "/* SOC node */\n" << endl;
    buffer << node_tab << sdt_soc_node_obj.soc_node_name << " {" << endl;
    buffer << node_tab << node_tab
           << "compatible = " << sdt_soc_node_obj.soc_compatible << ";" << endl;
    buffer << node_tab << node_tab
           << "#address-cells = " << sdt_soc_node_obj.soc_address_cell << ";"
           << endl;
    buffer << node_tab << node_tab
           << "#size-cells = " << sdt_soc_node_obj.soc_size_cell << ";" << endl;
    if (sdt_soc_node_obj.soc_ranges_key_value == "") {
      buffer << node_tab << node_tab << "ranges;" << endl << endl << endl;
    }

    for (int i = 0; i < sdt_soc_node_obj.size_soc_inst_array; i++) {
      if (sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem ==
          "interrupt-controller") {
        generated_soc_subsystem_string = gen_soc_subsystem_int_controller(
            buffer,
            sdt_soc_node_obj.p_soc_inst_array[i]
                ->soc_interrupt_controller_object,
            verbose);

        if (verbose == 1) {
          cout << "\n\ngen_soc_subsystem_int_controller = \n\n"
               << generated_soc_subsystem_string << endl;
        }

      } else if (sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem ==
                 "uart") {
        generated_soc_subsystem_string = gen_soc_subsystem_uart(
            buffer, sdt_soc_node_obj.p_soc_inst_array[i]->soc_uart_object,
            verbose);

        if (verbose == 1) {
          cout << "\n\ngen_soc_subsystem_uart = \n\n"
               << generated_soc_subsystem_string << endl;
        }

      } else if (sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem ==
                 "gpio") {
        generated_soc_subsystem_string = gen_soc_subsystem_gpio(
            buffer, sdt_soc_node_obj.p_soc_inst_array[i]->soc_gpio_object,
            verbose);

        if (verbose == 1) {
          cout << "\n\ngen_soc_subsystem_gpio = \n\n"
               << generated_soc_subsystem_string << endl;
        }

      } else if (sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem ==
                 "syscon") {
        generated_soc_subsystem_string = gen_soc_subsystem_syscon(
            buffer, sdt_soc_node_obj.p_soc_inst_array[i]->soc_syscon_object,
            verbose);

        if (verbose == 1) {
          cout << "\n\ngen_soc_subsystem_syscon = \n\n"
               << generated_soc_subsystem_string << endl;
        }

      } else if (sdt_soc_node_obj.p_soc_inst_array[i]->soc_subsystem ==
                 "timer") {
        generated_soc_subsystem_string = gen_soc_subsystem_timer(
            buffer, sdt_soc_node_obj.p_soc_inst_array[i]->soc_timer_object,
            verbose);

        if (verbose == 1) {
          cout << "\n\ngen_soc_subsystem_timer = \n\n"
               << generated_soc_subsystem_string << endl;
        }

      } else {
        if (verbose)
          cout << "Warning!!!\nInvalid soc_subsystem detected in "
                  "gen_soc_node() function\n\n";
      }
    }

    buffer << node_tab << "};" << endl;

    outfile << buffer.str();

    if (verbose == 1) {
      cout << "gen_soc_node output = \n\n" << buffer.str() << endl;
    }

    return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!!\nSOC node hasn't been populated yet!!!\n\n"
           << endl;

    return 0;
  }

  // return buffer.str();
}

// passing &output by ref cx we wana change it
// string gen_memory_node(ofstream &outfile, SdtMemoryNode sdt_memory_node_obj,
// int verbose) {
int gen_memory_node(ofstream &outfile, SdtMemoryNode sdt_memory_node_obj,
                    int verbose) {
  stringstream buffer;

  if (sdt_memory_node_obj.object_has_been_populated) {
    if (sdt_memory_node_obj.size_memory_inst_array > 0) {
      for (int i = 0; i < sdt_memory_node_obj.size_memory_inst_array; i++) {
        buffer << endl << node_tab << "/* Memory SDT Node */\n" << endl;

        buffer << node_tab
               << sdt_memory_node_obj.p_memory_inst_array[i]
                      ->memory_sub_device_type
               << sdt_memory_node_obj.p_memory_inst_array[i]->memory_id << ": "
               << sdt_memory_node_obj.sdt_memory_node_name << "@"
               << sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg_address
               << " {" << endl;

        buffer << node_tab << node_tab << "compatible = "
               << sdt_memory_node_obj.p_memory_inst_array[i]->memory_compatible
               << ";" << endl;

        buffer << node_tab << node_tab << "device type = "
               << sdt_memory_node_obj.p_memory_inst_array[i]->memory_device_type
               << ";" << endl;

        buffer << node_tab << node_tab << "reg = "
               << sdt_memory_node_obj.p_memory_inst_array[i]->memory_reg << ";"
               << endl;

        buffer << node_tab << "};" << endl;
      }

      outfile << buffer.str();

      if (verbose == 1) {
        cout << "gen_memory_node output = \n\n" << buffer.str() << endl;
      }

    } else {
      if (verbose)
        cout << "\n\nWarning!!!\nMemory node array instances haven't been "
                "populated yet!!!\n\n"
             << endl;
    }

    return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!!\nMemory node hasn't been populated yet!!!\n\n"
           << endl;

    return 0;
  }

  // return buffer.str();
}

// passing &output by ref cx we wana change it
// string gen_cpus_node(ofstream &outfile, SdtCpusNode sdt_cpus_node_obj, int
// verbose) {
int gen_cpus_node(ofstream &outfile, SdtCpusNode sdt_cpus_node_obj,
                  int verbose) {
  stringstream buffer;

  if (sdt_cpus_node_obj.object_has_been_populated) {
    buffer << endl
           << node_tab << "/* Application CPU configuration */\n"
           << endl;
    buffer << node_tab << sdt_cpus_node_obj.cpus_node_name << " {" << endl;
    buffer << node_tab << node_tab
           << "#address-cells = " << sdt_cpus_node_obj.cpus_address_cell << ";"
           << endl;
    buffer << node_tab << node_tab
           << "#size-cells = " << sdt_cpus_node_obj.cpus_size_cell << ";"
           << endl;
    buffer << node_tab << node_tab << "timebase-frequency = "
           << sdt_cpus_node_obj.cpus_timebase_frequency << ";" << endl;

    for (int i = 0; i < sdt_cpus_node_obj.size_cpu_inst_array; i++) {
      if (sdt_cpus_node_obj.p_cpu_inst_array[i]->object_has_been_populated) {
        buffer << node_tab << node_tab
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_sub_device_type
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_id << ": "
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_device_type << "@"
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg_address << " {"
               << endl;

        buffer << node_tab << node_tab << node_tab << "compatible = "
               << "\"" << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_compatible
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "device_type = "
               << "\"" << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_device_type
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab
               << "reg = " << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_reg
               << ";" << endl;

        buffer << node_tab << node_tab << node_tab << "status = "
               << "\"" << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_status
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "riscv,isa = "
               << "\"" << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_riscv_isa
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "mmu-type = "
               << "\"" << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_mmu_type
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "clock-frequency = "
               << "\""
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_clock_frequency
               << "\";" << endl;

        buffer << node_tab << node_tab << node_tab << "i-cache-line-size = "
               << "<"
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_i_cache_line_size
               << ">;" << endl;

        buffer << node_tab << node_tab << node_tab << "d-cache-line-size = "
               << "<"
               << sdt_cpus_node_obj.p_cpu_inst_array[i]->cpu_d_cache_line_size
               << ">;" << endl;

        if (sdt_cpus_node_obj.p_cpu_inst_array[i]
                ->cpu_int_cont_data.object_has_been_populated) {
          buffer << node_tab << node_tab << node_tab
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]
                        ->cpu_int_cont_data.int_cont_phandle
                 << ": "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]
                        ->cpu_int_cont_data.cpu_inst_subnode_int_cont_node_name
                 << " {" << endl;

          buffer << node_tab << node_tab << node_tab << node_tab
                 << "compatible = "
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]
                        ->cpu_int_cont_data.int_cont_compatible
                 << endl;

          buffer << node_tab << node_tab << node_tab << node_tab
                 << "#address-cells = "
                 << "<"
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]
                        ->cpu_int_cont_data.int_cont_address_cells
                 << ">;" << endl;

          buffer << node_tab << node_tab << node_tab << node_tab
                 << "#interrupt-cells = "
                 << "<"
                 << sdt_cpus_node_obj.p_cpu_inst_array[i]
                        ->cpu_int_cont_data.int_cont_interrupt_cells
                 << ">;" << endl;

          if (sdt_cpus_node_obj.p_cpu_inst_array[i]
                  ->cpu_int_cont_data.int_cont_interrupt_controller_key == "") {
            buffer << node_tab << node_tab << node_tab << node_tab
                   << "interrupt-controller;" << endl;
          }

          buffer << node_tab << node_tab << node_tab << "};" << endl;

        } else {
          if (verbose)
            cout << "\nWarning!!!!\nsdt_cpus_node_obj->p_cpu_inst_array[" << i
                 << "].cpu_int_cont_data object has not been populated!!! \n\n";
        }

        buffer << node_tab << node_tab << "};" << endl;

      } else {
        if (verbose)
          cout << "\nWarning!!!!\nsdt_cpus_node_obj->p_cpu_inst_array[" << i
               << "] object has not been populated!!! \n\n";
      }
    }

    buffer << node_tab << "};" << endl;

    outfile << buffer.str();

    if (verbose == 1) {
      cout << "gen_cpus_node output = \n\n" << buffer.str() << endl;
    }

    return 1;

  } else {
    if (verbose)
      cout << "\n\nWarning!!!\nCPUs node hasn't been populated yet!!!\n\n"
           << endl;

    return 0;
  }

  // return buffer.str();
}
