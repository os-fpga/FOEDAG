
//**********************************************************************************************************************************
// JSON to SDT cpp script written by ZaidTahir, for questions please email:
// zaid.butt.tahir@gmail.com or zaidt@bu.edu **************
// *********************************************************************************************************************************

#ifndef MY_CLASS_H  // include guard
#define MY_CLASS_H

#include <cstdio>
// #include <nlohmann_json/json.hpp>
#include "nlohmann_json/json.hpp"
// #include "json.hpp"
// #include "json/single_include/nlohmann/json.hpp"
// #include <json/single_include/nlohmann/json_fwd.hpp>
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

// class for storing data of sdt cpus node
class SdtRootMetaDataNode {
 public:
  string rootmetadata_size_cell;
  string rootmetadata_address_cell;
  string rootmetadata_compatible;
  string rootmetadata_model;
  string rootmetadata_node_name;

  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // class default constructor
  SdtRootMetaDataNode() {
    object_has_been_populated = 0;  // initialize with a 0

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    rootmetadata_address_cell = "";
    rootmetadata_size_cell = "";
    rootmetadata_compatible = "";
    rootmetadata_model = "";
    rootmetadata_node_name = "";
  }
};

// class for storing data of sdt interrupt controller meta data inside a cpu
// instantiations in the cpu sub-node
class SdtCpuInstSubNodeInterruptControllerData {
 public:
  string int_cont_phandle;
  string int_cont_compatible;
  string int_cont_address_cells;
  string int_cont_interrupt_cells;
  string int_cont_interrupt_controller_key;
  string cpu_inst_subnode_int_cont_node_name;

  int int_cont_id;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // default constructor
  SdtCpuInstSubNodeInterruptControllerData() {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned
    int_cont_id = -1;

    int_cont_phandle = "";
    int_cont_compatible = "";
    int_cont_address_cells = "";
    int_cont_interrupt_cells = "";
    int_cont_interrupt_controller_key = "";
    cpu_inst_subnode_int_cont_node_name = "";
  }
};

// class for storing data of sdt cpu instantiations in the cpu sub-node
class SdtCpuInstSubNode {
 public:
  string cpu_device_type;
  string cpu_sub_device_type;
  string cpu_reg;
  string cpu_reg_size;
  string cpu_reg_address;
  string cpu_status;
  string cpu_compatible;
  string cpu_riscv_isa;
  string cpu_mmu_type;
  string sdt_cpu_inst_subnode_name;

  // CPU instantiation interrupt-controller data class object
  SdtCpuInstSubNodeInterruptControllerData cpu_int_cont_data;

  int cpu_id;
  int cpu_clock_frequency;
  int cpu_i_cache_line_size;
  int cpu_d_cache_line_size;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  static int total_instances;

  // default class constructor
  SdtCpuInstSubNode() {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    cpu_id = -1;
    cpu_clock_frequency = -1;
    cpu_i_cache_line_size = -1;
    cpu_d_cache_line_size = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    cpu_device_type = "";
    cpu_sub_device_type = "";
    cpu_reg = "";
    cpu_reg_size = "";
    cpu_reg_address = "";
    cpu_status = "";
    cpu_compatible = "";
    cpu_riscv_isa = "";
    cpu_mmu_type = "";
    sdt_cpu_inst_subnode_name = "";

    total_instances = total_instances + 1;
  }
};

// class for storing data of sdt cpus node
class SdtCpusNode {
 public:
  string cpus_size_cell;
  string cpus_address_cell;
  string cpus_node_name;

  int cpus_timebase_frequency;
  int size_cpu_inst_array;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  SdtCpuInstSubNode **p_cpu_inst_array;

  // class default constructor
  SdtCpusNode() {
    object_has_been_populated = 0;  // initialize with a 0

    // -1 for ints mean that this variable wasn't assigned a value
    cpus_timebase_frequency = -1;
    size_cpu_inst_array = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    cpus_address_cell = "";
    cpus_size_cell = "";
    cpus_node_name = "";
  }
};

class SdtCpuClusterInstSubNode {
 public:
  string cpu_cluster_device_type;
  string cpu_cluster_sub_device_type;
  string cpu_cluster_reg;
  string cpu_cluster_reg_size;
  string cpu_cluster_reg_address;
  string cpu_cluster_status;
  string cpu_cluster_compatible;
  string cpu_cluster_riscv_isa;

  // CPU instantiation interrupt-controller data class object
  // SdtCpuInstSubNodeInterruptControllerData cpu_int_cont_data;
  // don't need this for cpus cluster node as of yet

  int cpu_cluster_id;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  static int total_instances;

  // default class constructor
  SdtCpuClusterInstSubNode() {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    cpu_cluster_id = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    cpu_cluster_device_type = "";
    cpu_cluster_sub_device_type = "";
    cpu_cluster_reg = "";
    cpu_cluster_reg_size = "";
    cpu_cluster_reg_address = "";
    cpu_cluster_status = "";
    cpu_cluster_compatible = "";
    cpu_cluster_riscv_isa = "";

    total_instances = total_instances + 1;
  }
};

class SdtCpusClusterNode {
 public:
  string cpus_cluster_size_cell;
  string cpus_cluster_address_cell;
  string cpus_cluster_compatible;
  string cpus_cluster_phandle;
  string cpus_cluster_node_name;

  int size_cpu_cluster_inst_array;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  SdtCpuClusterInstSubNode **p_cpu_cluster_inst_array;

  // class default constructor
  SdtCpusClusterNode() {
    object_has_been_populated = 0;  // initialize with a 0

    // -1 for ints mean that this variable wasn't assigned a value
    size_cpu_cluster_inst_array = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    cpus_cluster_address_cell = "";
    cpus_cluster_size_cell = "";
    cpus_cluster_compatible = "";
    cpus_cluster_phandle = "bcpu";
    cpus_cluster_node_name = "cpus-cluster";
  }
};

class SdtMemoryInstSubNode {
  // this is not a sub node actually.. each sub not for memory is an
  // indenpendant node rather than being a subnode. So no need to have a
  // subnode_name here cx we already gave the name to the memory node

 public:
  string memory_device_type;
  string memory_sub_device_type;
  string memory_reg;
  string memory_reg_size;
  string memory_reg_address;
  string memory_compatible;

  int memory_id;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  static int total_instances;

  // default class constructor
  SdtMemoryInstSubNode() {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    memory_id = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    memory_device_type = "";
    memory_sub_device_type = "";
    memory_reg = "";
    memory_reg_size = "";
    memory_reg_address = "";
    memory_compatible = "";

    total_instances = total_instances + 1;
  }
};

class SdtMemoryNode {
 public:
  string sdt_memory_node_name;

  int size_memory_inst_array;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // declare SdtMemoryInstSubNode pointer to pointer for ptr to ptr array that
  // would store pointer that would point to
  //  SdtMemoryInstSubNode objects
  // malloc size to it depending upon "size_memory_inst_array"
  SdtMemoryInstSubNode **p_memory_inst_array;

  // class default constructor
  SdtMemoryNode() {
    sdt_memory_node_name = "";

    object_has_been_populated = 0;  // initialize with a 0

    // -1 for ints mean that this variable wasn't assigned a value
    size_memory_inst_array = -1;
  }
};

class SdtSocInstSubNodeInterruptController {
 public:
  string interrupt_controller_subnode_name;
  string interrupt_controller_phandle;
  string interrupt_controller_compatible;
  string interrupt_controller_address_cells;
  string interrupt_controller_interrupt_cells;
  string interrupt_controller_key_value;
  string interrupt_controller_reg;
  string interrupt_controller_interrupts_extended;

  string *interrupt_controller_reg_names_string_array;
  int interrupt_controller_reg_names_string_array_size;
  // this variable is vip for knowing how many elements are in the reg_names
  // string array

  string *interrupt_controller_reg_address_string_array;
  int interrupt_controller_reg_address_string_array_size;

  string *interrupt_controller_reg_size_string_array;
  int interrupt_controller_reg_size_string_array_size;

  int interrupt_controller_id;
  int interrupt_controller_riscv_max_priority;
  int interrupt_controller_riscv_ndev;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // static int total_instances;

  // default class constructor
  SdtSocInstSubNodeInterruptController(string soc_subsystem_subnode_name) {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    interrupt_controller_id = -1;
    interrupt_controller_riscv_max_priority = -1;
    interrupt_controller_riscv_ndev = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    interrupt_controller_subnode_name = soc_subsystem_subnode_name;
    interrupt_controller_phandle = "";
    interrupt_controller_compatible = "";
    interrupt_controller_address_cells = "";
    interrupt_controller_interrupt_cells = "";
    interrupt_controller_key_value =
        "not empty";  // cx we need this to be empty in json after parsing it
    interrupt_controller_reg = "";
    // interrupt_controller_reg_size = "";
    // interrupt_controller_reg_address = "";
    interrupt_controller_interrupts_extended = "";

    interrupt_controller_reg_names_string_array = NULL;
    interrupt_controller_reg_address_string_array = NULL;
    interrupt_controller_reg_size_string_array = NULL;

    interrupt_controller_reg_names_string_array_size = -1;
    interrupt_controller_reg_address_string_array_size = -1;
    interrupt_controller_reg_size_string_array_size = -1;

    // total_instances = total_instances + 1;
  }
};

class SdtSocInstSubNodeUart {
 public:
  string uart_subnode_name;
  string uart_phandle;
  string uart_compatible;
  string uart_interrupt_parent;
  string uart_reg;
  string uart_reg_size;
  string uart_reg_address;
  string uart_status;

  string *uart_interrupts_string_array = NULL;
  // intializing this pointer here instead of constructor

  int uart_interrupts_string_array_size;
  // this variable is vip for knowing how many elements are in the reg_names
  // string array

  int uart_id;
  int uart_clock_frequency;
  int uart_reg_shift;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // static int total_instances;

  // default class constructor
  SdtSocInstSubNodeUart(string soc_subsystem_subnode_name) {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    uart_id = -1;
    uart_interrupts_string_array_size = -1;
    uart_clock_frequency = -1;
    uart_reg_shift = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    uart_subnode_name = soc_subsystem_subnode_name;
    uart_phandle = "";
    uart_compatible = "";
    uart_interrupt_parent = "";
    uart_reg = "";
    uart_reg_size = "";
    uart_reg_address = "";
    uart_status = "";
    uart_compatible = "";

    // total_instances = total_instances + 1;
  }
};

class SdtSocInstSubNodeGpio {
 public:
  string gpio_subnode_name;
  string gpio_phandle;
  string gpio_compatible;
  string gpio_interrupt_parent;
  string gpio_reg;
  string gpio_reg_size;
  string gpio_reg_address;
  string gpio_status;
  string gpio_controller_key_value;

  // will use dynamic string array allocation for this, ref
  // "https://stackoverflow.com/questions/20207400/dynamically-allocated-string-array-then-change-its-value"
  string *gpio_interrupts_string_array = NULL;
  // intializing this pointer here instead of constructor
  int gpio_interrupts_string_array_size;
  // this variable is vip for knowing how many elements are in the reg_names
  // string array

  int gpio_id;
  int gpio_cells;
  int gpio_ngpios;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // static int total_instances;

  // default class constructor
  SdtSocInstSubNodeGpio(string soc_subsystem_subnode_name) {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    gpio_id = -1;
    gpio_interrupts_string_array_size = -1;
    gpio_ngpios = -1;
    gpio_cells = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    gpio_subnode_name = soc_subsystem_subnode_name;
    gpio_phandle = "";
    gpio_compatible = "";
    gpio_interrupt_parent = "";
    gpio_reg = "";
    gpio_reg_size = "";
    gpio_reg_address = "";
    gpio_status = "";
    gpio_controller_key_value = "not empty";  // cx this needs to be empty

    // total_instances = total_instances + 1;
  }
};

class SdtSocInstSubNodeSyscon {
 public:
  string syscon_subnode_name;
  string syscon_phandle;
  string syscon_compatible;
  string syscon_reg;
  string syscon_reg_size;
  string syscon_reg_address;
  string syscon_status;

  int syscon_id;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // static int total_instances;

  // default class constructor
  SdtSocInstSubNodeSyscon(string soc_subsystem_subnode_name) {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    syscon_id = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    syscon_subnode_name = soc_subsystem_subnode_name;
    syscon_phandle = "";
    syscon_compatible = "";
    syscon_reg = "";
    syscon_reg_size = "";
    syscon_reg_address = "";
    syscon_status = "";

    // total_instances = total_instances + 1;
  }
};

class SdtSocInstSubNodeTimer {
 public:
  string timer_subnode_name;
  string timer_phandle;
  string timer_compatible;
  string timer_interrupts_extended;
  string timer_reg;
  string timer_reg_size;
  string timer_reg_address;
  string timer_status;

  int timer_id;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  // static int total_instances;

  // default class constructor
  SdtSocInstSubNodeTimer(string soc_subsystem_subnode_name) {
    // initialize with a 0
    object_has_been_populated = 0;

    // -1 for ints mean that this variable wasn't assigned a value
    timer_id = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    timer_subnode_name = soc_subsystem_subnode_name;
    timer_phandle = "";
    timer_compatible = "";
    timer_interrupts_extended = "";
    timer_reg = "";
    timer_reg_size = "";
    timer_reg_address = "";
    timer_status = "";

    // total_instances = total_instances + 1;
  }
};

class SdtSocInstSubNode {
 public:
  // string carrying the subsystem type
  string soc_subsystem;

  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  static int total_instances;

  SdtSocInstSubNodeInterruptController *soc_interrupt_controller_object;
  SdtSocInstSubNodeUart *soc_uart_object;
  SdtSocInstSubNodeGpio *soc_gpio_object;
  SdtSocInstSubNodeSyscon *soc_syscon_object;
  SdtSocInstSubNodeTimer *soc_timer_object;

  // default class constructor
  SdtSocInstSubNode(string soc_subsystem_type) {
    // initialize with a 0
    object_has_been_populated = 0;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    soc_subsystem = soc_subsystem_type;

    if (soc_subsystem_type == "interrupt-controller") {
      soc_interrupt_controller_object =
          new SdtSocInstSubNodeInterruptController(soc_subsystem_type);

    } else if (soc_subsystem_type == "uart") {
      // SdtSocInstSubNodeUart soc_uart_object(soc_subsystem_type);

      soc_uart_object = new SdtSocInstSubNodeUart(soc_subsystem_type);

    } else if (soc_subsystem_type == "gpio") {
      // SdtSocInstSubNodeGpio soc_gpio_object(soc_subsystem_type);

      soc_gpio_object = new SdtSocInstSubNodeGpio(soc_subsystem_type);

    } else if (soc_subsystem_type == "syscon") {
      // SdtSocInstSubNodeSyscon soc_syscon_object(soc_subsystem_type);

      soc_syscon_object = new SdtSocInstSubNodeSyscon(soc_subsystem_type);

    } else if (soc_subsystem_type == "timer") {
      // SdtSocInstSubNodeTimer soc_timer_object(soc_subsystem_type);

      soc_timer_object = new SdtSocInstSubNodeTimer(soc_subsystem_type);

    } else {
      // printf("Invalid subsystem type %s, exited with error \n",
      // soc_subsystem_type); throw 50; exit(0);
    }

    total_instances = total_instances + 1;
  }
};

class SdtSocNode {
 public:
  string soc_size_cell;
  string soc_address_cell;
  string soc_compatible;
  string soc_ranges_key_value;
  string soc_node_name;

  int size_soc_inst_array;
  int object_has_been_populated;  // must become 1 when class variables have
                                  // been populated

  SdtSocInstSubNode **p_soc_inst_array;

  // class default constructor
  SdtSocNode() {
    object_has_been_populated = 0;  // initialize with a 0

    // -1 for ints mean that this variable wasn't assigned a value
    size_soc_inst_array = -1;

    // if a string variable of our object has a empty string value, it means it
    // wasn't present in the json (wasn't assigned a value)
    soc_address_cell = "";
    soc_size_cell = "";
    soc_compatible = "";
    soc_ranges_key_value =
        "not empty";  // since this needs to be empty after json is
    soc_node_name = "soc";
  }
};

// void get_soc_node(json data, SdtSocNode &sdt_soc_node_obj) {
int get_soc_node(json data, SdtSocNode &sdt_soc_node_obj, int verbose = 0);

// void get_memory_node(json data, SdtMemoryNode &sdt_memory_node_obj) {
int get_memory_node(json data, SdtMemoryNode &sdt_memory_node_obj,
                    int verbose = 0);

int get_cpus_cluster_node(json data,
                          SdtCpusClusterNode &sdt_cpus_cluster_node_obj,
                          int verbose = 0);

int get_rootmetadata_node(json data,
                          SdtRootMetaDataNode &sdt_rootmetadata_node_obj,
                          int verbose = 0);

int get_cpus_node(json data, SdtCpusNode &sdt_cpus_node_obj, int verbose = 0);

string return_string_from_ofstream_file(ofstream &outfile, string file_path);

int gen_rootmetadata_node(ofstream &outfile,
                          SdtRootMetaDataNode sdt_rootmetadata_node_obj,
                          int verbose = 0);

// string gen_cpus_cluster_node(ofstream &outfile, SdtCpusClusterNode
// sdt_cpus_cluster_node_obj, int verbose=0) {
int gen_cpus_cluster_node(ofstream &outfile,
                          SdtCpusClusterNode sdt_cpus_cluster_node_obj,
                          int verbose = 0);

string gen_soc_subsystem_timer(stringstream &buffer_out,
                               SdtSocInstSubNodeTimer *soc_timer_object,
                               int verbose = 0);

string gen_soc_subsystem_syscon(stringstream &buffer_out,
                                SdtSocInstSubNodeSyscon *soc_syscon_object,
                                int verbose = 0);

string gen_soc_subsystem_gpio(stringstream &buffer_out,
                              SdtSocInstSubNodeGpio *soc_gpio_object,
                              int verbose = 0);

string gen_soc_subsystem_uart(stringstream &buffer_out,
                              SdtSocInstSubNodeUart *soc_uart_object,
                              int verbose = 0);

string gen_soc_subsystem_int_controller(
    stringstream &buffer_out,
    SdtSocInstSubNodeInterruptController *soc_interrupt_controller_object,
    int verbose = 0);

int gen_soc_node(ofstream &outfile, SdtSocNode sdt_soc_node_obj,
                 int verbose = 0);

int gen_memory_node(ofstream &outfile, SdtMemoryNode sdt_memory_node_obj,
                    int verbose = 0);

int gen_cpus_node(ofstream &outfile, SdtCpusNode sdt_cpus_node_obj,
                  int verbose = 0);

#endif /* MY_CLASS_H */
