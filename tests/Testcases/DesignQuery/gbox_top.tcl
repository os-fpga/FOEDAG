# puts  [ test_device_modeling_tcl ]
# puts [ exec date ]

# use "verbose" with tcl command to display debugging information for the sdt generating cpp library "sdtgen_cpp_nlohman_lib_v6"

# puts [ sdt_gen_cpus_node verbose ]
puts [ sdt_gen_cpus_node ]
# # puts [ sdt_gen_cpus_cluster_node verbose ]
puts [ sdt_gen_cpus_cluster_node ]
# # puts [ sdt_gen_memory_node verbose ]
puts [ sdt_gen_memory_node ]
# # puts [ sdt_gen_soc_node verbose ]
puts [ sdt_gen_soc_node ]
# # puts [ sdt_gen_root_metadata_node verbose ]
puts [ sdt_gen_root_metadata_node ]
# puts [ sdt_gen_system_device_tree verbose ]
puts [ sdt_gen_system_device_tree ]

# run this tcl script by running this command in FOEDAG folder: ./build/bin/foedag --batch --script ./tests/Testcases/DesignQuery/gbox_top.tcl
