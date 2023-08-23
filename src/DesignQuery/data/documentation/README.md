## Details regarding TCL commands for SDT (System Device Tree) generation

The SDT generating TCL commands used in the "FOEDAG/gbox_top.tcl" script, used to generate the system device tree from the JSON file present in the folder "FOEDAG/src/DesignQuery/data/JSON_FILES" are mentioned below along with some detailed description. 

In the following SDT generating TCL commands you will see that there are two versions of the same command, one with an additional argument "verbose" and one without. Adding "verbose" argument to the SDT related TCL function will print debugging information from the SDT generating C++ library "FOEDAG/src/DesignQuery/sdtgen_cpp_nlohman_lib_v6.cpp" that is parsing the JSON file and generation the SDT file, so if there is any confusion or bug, it would get printed there. The user can go into the SDT generation C++ lib and paste additional printf commands under the "verbose" conditional blocks if additional debudgging information is required.

The SDT generating  TCL commands along with their brief descriptions are mentioned below.

- The following is the TCL command for generating the "cpus" SDT node only. This command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the "cpus" SDT node only.
  
  **puts [ sdt_gen_cpus_node verbose ]**

  **puts [ sdt_gen_cpus_node ]**

- The following is the TCL command for generating the "cpus-cluster" SDT node only. This command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the "cpus-cluster" SDT node only.

  **puts [ sdt_gen_cpus_cluster_node verbose ]**

  **puts [ sdt_gen_cpus_cluster_node ]**

- The following is the TCL command for generating the "memory" SDT nodes only. This command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the "memory" SDT nodes only.
  
  **puts [ sdt_gen_memory_node verbose ]**

  **puts [ sdt_gen_memory_node ]**

- The following is the TCL command for generating the "soc" SDT node only. This command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the "soc" SDT node only.

  **puts [ sdt_gen_soc_node verbose ]**

  **puts [ sdt_gen_soc_node ]**

- The following is the TCL command for generating the "root meta data" SDT node only. This command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the "root meta data" SDT node only.


  **puts [ sdt_gen_root_metadata_node verbose ]**

  **puts [ sdt_gen_root_metadata_node ]**

- The following is the TCL command for generating the whole SDT. his command is independent of all other commands and can be used on its own. This will print a SDT file in the "FOEDAG" folder that will contain the whole SDT.

  **puts [ sdt_gen_system_device_tree verbose ]**
  
  **puts [ sdt_gen_system_device_tree ]**