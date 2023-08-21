#!/usr/bin/tclsh
package require json
package require Tcl 8.5

package require json::write
# package require json::json2prettydict

if {$::tcl_version < 8.5} {
    package require dict
}

##########################################################################################################################################
########## JSON to SDT TCL script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu ############
##########################################################################################################################################

# Use this command to run this script: 
	# "tclsh sdtgen_v6.tcl -o output_v6.sdt -f"
# All functionality moved to procs in tcl script version5


set node_spacing "\t\t"
set sub_node_spacing "\t\t\t\t\t"
set sub_sub_node_spacing "\t\t\t\t\t\t\t"

proc is-dict {value} {
  # puts $value
  return [expr {[string is list $value] && ([llength $value]&1) == 0}]
}

proc extract_sub_module_names {modules_dict_list {module_name_dict_key "module"}}  {
	set modules_names_list {}

	foreach module_dict $modules_dict_list {
		set dict_key_for_module_name $module_name_dict_key
		set module_name [dict get $module_dict $dict_key_for_module_name]
		lappend modules_names_list $module_name
	}

	return $modules_names_list
}

proc extract_sub_module_inst_names {modules_dict_list {module_inst_name_dict_key "instName"}}  {
	set modules_inst_names_list {}

	foreach module_dict $modules_dict_list {
		set dict_key_for_module_inst_name $module_inst_name_dict_key
		set module_inst_name [dict get $module_dict $dict_key_for_module_inst_name]
		lappend modules_inst_names_list $module_inst_name
	}

	return $modules_inst_names_list
}


proc extract_sub_module_addresses_sizes {modules_dict_list {module_inst_address_dict_key "reg_address"} {module_inst_size_dict_key "reg_size"}}  {
	set modules_inst_addresses_list {}
	set modules_inst_sizes_list {}

	foreach module_dict $modules_dict_list {
		set dict_key_for_module_address $module_inst_address_dict_key
		set dict_key_for_module_size $module_inst_size_dict_key
		set module_address [dict get $module_dict $dict_key_for_module_address]
		set module_size [dict get $module_dict $dict_key_for_module_size]

		lappend modules_inst_addresses_list $module_address
		lappend modules_inst_sizes_list $module_size

	}

	# puts $modules_inst_addresses_list

	set combined_list {}

	lappend combined_list $modules_inst_addresses_list
	lappend combined_list $modules_inst_sizes_list

	return $combined_list
}

# https://wiki.tcl-lang.org/page/args
	# default args 
proc read_json_file {{path_to_json_file "./JSON_Files/GPIO_Yosis_Ver_Example/gold_hier.json"}} {
		# needed dot ./ cx / was pointing to home
	set fp [open $path_to_json_file r+]  
	set json_file [read $fp]
	close $fp
	return $json_file
		# return json_file 
			# was just returning the text json_file
}

proc get_sdt_nodes_dict_from_json {json_file_path} {

	set gold_hier_json_file [read_json_file $json_file_path] 

		# custom path can be given as well as an arg

	# puts $gold_hier_json_file
		# prints the gold_hier_v2.json

	# exit the tcl script
	# exit 2

	# convert json file to multi element list (Dicts within it)
	set gold_hier_json_file_convereted_to_dict_list [json::many-json2dict $gold_hier_json_file]
	# puts $gold_hier_json_file_convereted_to_dict_list

	puts "llength gold_hier_json_file_convereted_to_dict_list = [llength $gold_hier_json_file_convereted_to_dict_list]"
		# llength gold_hier_json_file_convereted_to_dict_list = 1

	puts "llength \[lindex gold_hier_json_file_convereted_to_dict_list 0\] = [llength [lindex $gold_hier_json_file_convereted_to_dict_list 0]]"
		# llength [lindex gold_hier_json_file_convereted_to_dict_list 0] = 6
			# removes the outer curly braces

	# exit 2

	# puts [lindex $gold_hier_json_file_convereted_to_dict_list 0]
		# gets printed without the first curely braces {} (list initializer braces)

	# puts "dict size gold_hier_json_file_convereted_to_dict_list = [dict size $gold_hier_json_file_convereted_to_dict_list]"
		# Error: missing value to go with key

	puts "dict size \[lindex gold_hier_json_file_convereted_to_dict_list 0\] = [dict size [lindex $gold_hier_json_file_convereted_to_dict_list 0]]"
		# dict size [lindex gold_hier_json_file_convereted_to_dict_list 0] = 7

	# exit 2

	set sdt_nodes_dict [lindex $gold_hier_json_file_convereted_to_dict_list 0]

	return $sdt_nodes_dict
}

# Copyright (C) 2023 Rapid Silicon
# This code is distributed under the terms
# of the Rapid Silicon End User License Agreement
# SPDX-License-Identifier: rs-eula

#This proc generates the entire structure of the S-DTS
#It calls the other procs provided here to get the needed information

proc get_cpus {sdt_nodes_dict &cpus_inst_list &cpus_size_cell &cpus_address_cell &cpus_timebase_frequency} {
	#This proc should get a Tcl dictionary of all of the CPU clusters in the design
	#The dictionary should be a collection of node handles and then all information is
	#collected via teh get_cpu_info proc

	# the & is for denoting that we are passing these args by reference
	upvar ${&cpus_inst_list} cpus_inst_list
	upvar ${&cpus_size_cell} cpus_size_cell
	upvar ${&cpus_address_cell} cpus_address_cell
	upvar ${&cpus_timebase_frequency} cpus_timebase_frequency

	set iter 0 

	foreach key [dict keys $sdt_nodes_dict] {

		set keys_jsonFile_main_dicts $key

		if { $keys_jsonFile_main_dicts == "cpus"} {

			puts "key number $iter for gold_hier_json_file_convereted_to_dict_list idx 0 dict = $keys_jsonFile_main_dicts"

			set cpus_dict_key $keys_jsonFile_main_dicts

			set cpus_dict [dict get $sdt_nodes_dict $cpus_dict_key]

			# puts "\n\n cpus_dict = $cpus_dict \n\n"
			# exit 2
			# extracting cpus object out of array
				# already extracted
			# set cpus_dict [lindex $cpus_dict 0]
			# puts "\n\n after lindex 0 cpus_dict = $cpus_dict \n\n"
				#  after lindex 0cpus_dict = #address-cells 
			# exit 2

			set cpus_address_cells_key "#address-cells"
			set cpus_timebase_frequency_key "timebase-frequency"
			set cpus_size_cells_key "#size-cells"
			set cpus_array_list_key "cpu_insts"

			set cpus_address_cell [dict get $cpus_dict $cpus_address_cells_key]
			set cpus_size_cell [dict get $cpus_dict $cpus_address_cells_key]
			set cpus_timebase_frequency [dict get $cpus_dict $cpus_timebase_frequency_key]
			set cpus_inst_list [dict get $cpus_dict $cpus_array_list_key]


			# puts $cpus_address_cell
			# puts $cpus_size_cell
			# puts $cpus_inst_list
			# puts [lindex $cpus_inst_list 0]
				# this is the correct one.. removes the list curly braces

			# exit 2

		}

		incr iter
	}

}

# Right now cpu clusters means the BCPU ... we can combine cpus and cpu-cluster nodes later 
proc get_cpus_clusters {sdt_nodes_dict &cpus_cluster_inst_list &cpus_cluster_size_cell &cpus_cluster_address_cell &cpus_cluster_compatible} {
	#This proc should get a Tcl dictionary of all of the CPU clusters in the design
	#The dictionary should be a collection of node handles and then all information is
	#collected via teh get_cpu_info proc

	# the & is for denoting that we are passing these args by reference
	upvar ${&cpus_cluster_inst_list} cpus_cluster_inst_list
	upvar ${&cpus_cluster_size_cell} cpus_cluster_size_cell
	upvar ${&cpus_cluster_address_cell} cpus_cluster_address_cell
	upvar ${&cpus_cluster_compatible} cpus_cluster_compatible

	set iter 0 

	foreach key [dict keys $sdt_nodes_dict] {

		set keys_jsonFile_main_dicts $key

		if { $keys_jsonFile_main_dicts == "cpus-cluster"} {

				set cpus_cluster_dict_key $keys_jsonFile_main_dicts

				set cpus_cluster_dict [dict get $sdt_nodes_dict $cpus_cluster_dict_key]

				# puts "\n\ncpus_cluster_dict = $cpus_cluster_dict \n\n"
				# exit 2

				set cpus_cluster_address_cells_key "#address-cells"
				set cpus_cluster_size_cells_key "#size-cells"
				set cpus_compatible_key "compatible"
				set cpus_cluster_array_list_key "cpu_cluster_insts"

				set cpus_cluster_address_cell [dict get $cpus_cluster_dict $cpus_cluster_address_cells_key]
				set cpus_cluster_size_cell [dict get $cpus_cluster_dict $cpus_cluster_size_cells_key]
				set cpus_cluster_compatible [dict get $cpus_cluster_dict $cpus_compatible_key]
				set cpus_cluster_inst_list [dict get $cpus_cluster_dict $cpus_cluster_array_list_key]

				# puts "\n"
				# puts "\n cpus_cluster_address_cell = $cpus_cluster_address_cell \n"
				# puts "\n cpus_cluster_size_cell = $cpus_cluster_size_cell \n"
				# puts "\n cpus_cluster_compatible = $cpus_cluster_compatible \n"
				# puts "\n cpus_cluster_inst_list = $cpus_cluster_inst_list \n"
				# puts "\n cpus_cluster_inst_list idx 0 = [lindex $cpus_cluster_inst_list 0] \n"
					# this is the correct one.. removes the list curly braces

				# exit 2


			}

		incr iter
	}

}

proc get_memory_nodes {sdt_nodes_dict &memory_modules_inst_list} {
#This proc should get a Tcl dictionary of all of the memory nodes in the design
	
	# the & is for denoting that we are passing these args by reference
	upvar ${&memory_modules_inst_list} memory_modules_inst_list
	
	set iter 0 

	foreach key [dict keys $sdt_nodes_dict] {

		set keys_jsonFile_main_dicts $key

		if { $keys_jsonFile_main_dicts == "memory"} {

			set memory_modules_dict_key $keys_jsonFile_main_dicts
			
			set memory_modules_inst_list_dict_key "memory_insts"

			set memory_modules_dict [dict get $sdt_nodes_dict $memory_modules_dict_key]

			# puts "\n\ncpus_cluster_dict = $cpus_cluster_dict \n\n"
			# exit 2

			set memory_modules_inst_list [dict get $memory_modules_dict $memory_modules_inst_list_dict_key]

			# puts "\n memory_modules_inst_list = $memory_modules_inst_list \n"
			# puts "\n memory_modules_inst_list idx 0 = [lindex $memory_modules_inst_list 0] \n"
			# 	# this is the correct one.. removes the list curly braces

			# exit 2

		}

	}

}



proc get_cpu_info {} {
	#This proc should get a Tcl dictionary of metadata for a specific CPU cluster
	#This should include information such as the number of CPUs in the cluster, 
	#specific memory regionas associated with the cluster, and other metadata
	#required for a properly formed DTS node
	
}

proc get_mem_info {} {
	#This proc should get a Tcl dictionary of all memory regions in the design
	#This is DIFFERENT than the base and offset values for an IP.  See the get_ip_info
}

proc get_interrupts {} {
	#This proc should get a Tcl dictionary of all interrupt signals in the design
	#It should also be able to determine the source and sink of each interrupt
}

proc get_clks {} {
	#This proc should get a Tcl dictionary of all clock signals in the design
	#It should also be able to determine the source and sink of each clock
	
}

proc get_amba_interfaces {} {
	#This proc should get a Tcl dictionary of all AXI/AMBA interfaces in the design
}

proc get_soc {sdt_nodes_dict &soc_address_cells &soc_size_cells &soc_compatible &soc_ranges &soc_subsystems_list} {
	#This proc should get a Tcl dictionary of all IPs (hard and soft) in the design

	# the & is for denoting that we are passing these args by reference
	upvar ${&soc_address_cells} soc_address_cells
	upvar ${&soc_size_cells} soc_size_cells
	upvar ${&soc_compatible} soc_compatible
	upvar ${&soc_ranges} soc_ranges
	upvar ${&soc_subsystems_list} soc_subsystems_list

	set iter 0 

	foreach key [dict keys $sdt_nodes_dict] {

		set keys_jsonFile_main_dicts $key

		if { $keys_jsonFile_main_dicts == "soc"} {

			set soc_node_dict_key $keys_jsonFile_main_dicts
			
			set soc_subsystems_list_dict_key "soc_subsystems"

			set soc_node_dict [dict get $sdt_nodes_dict $soc_node_dict_key]

			# puts "\n\n soc_node_dict = $soc_node_dict \n\n"
			# exit 2

			set soc_subsystems_list [dict get $soc_node_dict $soc_subsystems_list_dict_key]

			# # puts "\n soc_subsystems_list = $soc_subsystems_list \n"
			# puts "\n soc_subsystems_list idx 0 = [lindex $soc_subsystems_list 0] \n"
			#  	# this is the correct one.. removes the list curly braces
			#  	# prints the first subsystem from the list without the curly braces

			# exit 2

			set soc_address_cells_key "#address-cells"
			set soc_size_cells_key "#size-cells"
			set soc_compatible_key "compatible"
			set soc_ranges_key "ranges"

			set soc_address_cells [dict get $soc_node_dict $soc_address_cells_key]
			set soc_size_cells [dict get $soc_node_dict $soc_size_cells_key]
			set soc_compatible [dict get $soc_node_dict $soc_compatible_key]
			set soc_ranges [dict get $soc_node_dict $soc_ranges_key]

		}

		incr iter
	}
}

proc get_ip_location {} {
	#This proc should determine if a specific IP instance is hardened or in FPGA region
}

proc get_ip_info {} {
	#This proc should get a Tcl dictionary of all metadata for a given IP instance
	#This will have AT A MINIMUM, the base address, high address, clock signal net name
	#interrupt net name (if used), any additional IP information specific to that IP
	
}

proc get_device_target {} {
	#This proc should get a value from the design that corresponds to the specific
	#Rapid Silicon device (eg, GE100)
	
	#hard-coded for now
	#vendor string
	dict set deviceTargetInfo vendor "Rapid Silicon"
	#model name string
	dict set deviceTargetInfo model "GE100"
	
	return $deviceTargetInfo
	
}

proc get_board_info {} {
	#This proc should get a value from the design that corresponds to the specific
	#board name value from the Raptor DS metadata
	
	#hard-coded for now
	#vendor string
	dict set boardInfoDict vendor "Rapid Silicon"
	#compatible string
	dict set boardInfoDict compat "rapid-si, glv-1.0"
	#full name
	dict set boardInfoDict name "Rapid Silicon Gemini Launch Vehicle Board"
	#version
	dict set boardInfoDict ver "1.0"
	
	return $boardInfoDict
}

proc gen_copyright_header {} {
	set copyrightString "/*\n \
	*Copyright (c) 2023 Rapid Silicon\n \
	*SPDX-License-Identifier: rs-eula\n \
	*JSON to SDT TCL script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n \
	*/\n\n"
						  
	return $copyrightString
}

proc gen_dts_preamble outputFile {
	upvar 1 $outputFile localOutputFile
	
	#create the DTS header/pre-amble -- everything before the cpus{} node
	set boardInfo [get_board_info]

	# print the board compatible string
	puts $localOutputFile "\t\t compatible = \"[dict get $boardInfo compat]\"";

	# print the address and cell size
	# puts $localOutputFile "\t\t #address-cells = <0x2>;"
	puts $localOutputFile "\t\t #address-cells = <0x1>;"
	# puts $localOutputFile "\t\t #size-cells = <0x2>;"
	puts $localOutputFile "\t\t #size-cells = <0x1>;"

	# print the board model
	puts $localOutputFile "\t\t model = \"[dict get $boardInfo name] revision [dict get $boardInfo ver]\""	

}

proc gen_bootargs {} {
	#This proc generates the BOOTARGS node in the DTS
	
}

proc gen_cpu_node {&dtsFile cpus_inst_list cpus_address_cell cpus_size_cell cpus_timebase_frequency} {
	#This proc generates the CPUS node in the DTS

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"
	set sub_sub_sub_node_spacing "\t\t\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	puts $dtsFile "\n$node_spacing /* Application CPU configuration */"
	puts $dtsFile "\n$node_spacing cpus \{"
	puts $dtsFile "$sub_node_spacing #address-cells = $cpus_address_cell;"
	puts $dtsFile "$sub_node_spacing #size-cells = $cpus_size_cell;"
	puts $dtsFile "$sub_node_spacing timebase-frequency = $cpus_timebase_frequency;"

	set i 0

	# writing SDT cpus node.. In the template gemini.sdt file the BCPU is mentioned under cpus-cluster node and not in cpus node,
	# so we will generate BCPU under cpus-cluster node.
	foreach cpu $cpus_inst_list {

		# puts "cpu = $cpu"
		# exit 2
			# cpu = id 0 device_type cpu sub_device_type acpu reg <0x00000000> status okay compatible riscv riscv,isa rv32e
		
		# extract each cpu address and size
		set cpu_sub_node_name "cpu"
		set sub_module_name $cpu_sub_node_name
		
		set cpu_phandle_key "sub_device_type"
		set cpu_phandle [dict get $cpu $cpu_phandle_key]
		set sub_module_inst_name $cpu_phandle
		
		set cpu_address_key "reg_address"
		set cpu_address [dict get $cpu $cpu_address_key]

		set cpu_id_key "id"
		set cpu_id [dict get $cpu $cpu_id_key]

		set cpu_size_key "reg_size"

		set cpu_reg_key "reg"
		set cpu_reg [dict get $cpu $cpu_reg_key]

		set cpu_device_type_key "device_type"
		set cpu_device_type [dict get $cpu $cpu_device_type_key]
		
		set cpu_status_key "status"
		set cpu_status [dict get $cpu $cpu_status_key]
		
		set cpu_compatible_key "compatible"
		set cpu_compatible [dict get $cpu $cpu_compatible_key]
		
		set cpu_riscv_isa_key "riscv,isa"
		set cpu_riscv_isa [dict get $cpu $cpu_riscv_isa_key]

		set cpu_mmu_type_key "mmu-type"
		set cpu_mmu_type [dict get $cpu $cpu_mmu_type_key]

		set cpu_clock_frequency_key "clock-frequency"
		set cpu_clock_frequency [dict get $cpu $cpu_clock_frequency_key]

		set cpu_i_cache_line_size_key "i-cache-line-size"
		set cpu_i_cache_line_size [dict get $cpu $cpu_i_cache_line_size_key]

		set cpu_d_cache_line_size_key "d-cache-line-size"
		set cpu_d_cache_line_size [dict get $cpu $cpu_d_cache_line_size_key]

		set cpu_interrupt_controller_key "interrupt-controller"
		set cpu_interrupt_controller [dict get $cpu $cpu_interrupt_controller_key]

		if {[llength $cpu_interrupt_controller] >= 1} {

			set cpu_interrupt_controller_id_key "id"
			set cpu_interrupt_controller_phandle_key "phandle"
			set cpu_interrupt_controller_compatible_key "compatible"
			set cpu_interrupt_controller_address_cells_key "#address-cells"
			set cpu_interrupt_controller_interrupt_cells_key "#interrupt-cells"
			set cpu_interrupt_controller_interrupt_controller_key "interrupt-controller"


			set cpu_interrupt_controller_id [dict get $cpu_interrupt_controller $cpu_interrupt_controller_id_key]
			set cpu_interrupt_controller_phandle [dict get $cpu_interrupt_controller $cpu_interrupt_controller_phandle_key]
			set cpu_interrupt_controller_compatible [dict get $cpu_interrupt_controller $cpu_interrupt_controller_compatible_key]
			set cpu_interrupt_controller_address_cells [dict get $cpu_interrupt_controller $cpu_interrupt_controller_address_cells_key]
			set cpu_interrupt_controller_interrupt_cells [dict get $cpu_interrupt_controller $cpu_interrupt_controller_interrupt_cells_key]
			set cpu_interrupt_controller_interrupt_controller [dict get $cpu_interrupt_controller $cpu_interrupt_controller_interrupt_controller_key]


		}



		puts $dtsFile "$sub_node_spacing $sub_module_inst_name$cpu_id: $sub_module_name@$cpu_address \{"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_compatible_key = \"$cpu_compatible\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_device_type_key = \"$cpu_device_type\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_reg_key = $cpu_reg;"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_status_key = \"$cpu_status\";"
		
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_riscv_isa_key = \"$cpu_riscv_isa\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_mmu_type_key = \"$cpu_mmu_type\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_clock_frequency_key = <$cpu_clock_frequency>;"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_i_cache_line_size_key = <$cpu_i_cache_line_size>;"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_d_cache_line_size_key = <$cpu_d_cache_line_size>;"

		# generating cpu interrupt controller property
		if {[llength $cpu_interrupt_controller] >= 1} {
			puts $dtsFile "$sub_sub_node_spacing\t  $cpu_interrupt_controller_phandle: $cpu_interrupt_controller_interrupt_controller_key \{"
			puts $dtsFile "$sub_sub_sub_node_spacing\t  $cpu_interrupt_controller_compatible_key = \"$cpu_interrupt_controller_compatible\";"
			puts $dtsFile "$sub_sub_sub_node_spacing\t  $cpu_interrupt_controller_address_cells_key = <$cpu_interrupt_controller_address_cells>;"
			puts $dtsFile "$sub_sub_sub_node_spacing\t  $cpu_interrupt_controller_interrupt_cells_key = <$cpu_interrupt_controller_interrupt_cells>;"
			if { $cpu_interrupt_controller_interrupt_controller == "" } {
				puts $dtsFile "$sub_sub_sub_node_spacing\t  $cpu_interrupt_controller_interrupt_controller_key;"
			}

			puts $dtsFile "$sub_sub_node_spacing\t  \};"
		}

		puts $dtsFile "\n$sub_node_spacing \};"
		incr i


	}

	# end cpu node
	puts $dtsFile "\n$node_spacing \};"

}

proc gen_cpu_cluster_node {&dtsFile cpus_cluster_address_cell cpus_cluster_size_cell cpus_cluster_compatible cpus_cluster_inst_list} {
	#This proc generates the CPU_cluster node in the DTS

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	puts $dtsFile "\n$node_spacing /* Boot CPU configuration */ "
	puts $dtsFile "\n$node_spacing bcpu: cpus-cluster@0 \{"

	puts $dtsFile "$sub_node_spacing #address-cells = $cpus_cluster_address_cell;"
	puts $dtsFile "$sub_node_spacing #size-cells = $cpus_cluster_size_cell;"
	puts $dtsFile "$sub_node_spacing compatible = $cpus_cluster_compatible;"

	set i 0

	# writing SDT BCPU cpus-cluster node
	foreach cpu $cpus_cluster_inst_list {

		# extract each cpu address and size
		set cpu_sub_node_name "cpu"
		set sub_module_name $cpu_sub_node_name
		
		set cpu_address_key "reg_address"
		set cpu_address [dict get $cpu $cpu_address_key]

		set cpu_id_key "id"
		set cpu_id [dict get $cpu $cpu_id_key]

		set cpu_size_key "reg_size"

		set cpu_reg_key "reg"
		set cpu_reg [dict get $cpu $cpu_reg_key]

		set cpu_device_type_key "device_type"
		
		set cpu_status_key "status"
		set cpu_status [dict get $cpu $cpu_status_key]
		
		set cpu_compatible_key "compatible"
		set cpu_compatible [dict get $cpu $cpu_compatible_key]
		
		set cpu_riscv_isa_key "riscv,isa"
		set cpu_riscv_isa [dict get $cpu $cpu_riscv_isa_key]

		puts $dtsFile "\n\n$sub_sub_node_spacing $sub_module_name@$cpu_address \{"
		puts $dtsFile "\n$sub_sub_node_spacing\t  $cpu_compatible_key = \"$cpu_compatible\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_reg_key = $cpu_reg;"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_status_key = \"$cpu_status\";"
		puts $dtsFile "$sub_sub_node_spacing\t  $cpu_riscv_isa_key = \"$cpu_riscv_isa\";"
		puts $dtsFile "\n$sub_sub_node_spacing \};"
		incr i


	}

	# end cpu_cluster node
	puts $dtsFile "\n$node_spacing \};"


}

#https://github.com/RapidSilicon/zephyr-rapidsi-dev2/blob/c9c26d648db1a698d48083f45ab856e9a4726be7/dts/riscv/rapidsi/rapidsi_gemini.dtsi
	# memory nodes examples
proc gen_memory_nodes {&dtsFile memory_modules_inst_list} {
	#This proc generates the memory nodes in the DTS

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	set i 0

	foreach memory_modules_dict $memory_modules_inst_list {

		# extract each memory node address and size
		set memory_node_name "memory"
		
		set memory_node_phandle_key "sub_device_type"
		set memory_node_phandle [dict get $memory_modules_dict $memory_node_phandle_key]

		set memory_node_device_type_key "device_type"
		set memory_node_device_type [dict get $memory_modules_dict $memory_node_device_type_key]

		set memory_node_id_key "id"
		set memory_node_id [dict get $memory_modules_dict $memory_node_id_key]

		set memory_node_reg_key "reg"
		set memory_node_reg [dict get $memory_modules_dict $memory_node_reg_key]

		set memory_node_compatible_key "compatible"
		set memory_node_compatible [dict get $memory_modules_dict $memory_node_compatible_key]

		set memory_node_address_key "reg_address"
		set memory_node_address [dict get $memory_modules_dict $memory_node_address_key]



		puts $dtsFile "\n$node_spacing /* Memory SDT Node */"
		puts $dtsFile "\n$node_spacing $memory_node_phandle$memory_node_id: $memory_node_name@$memory_node_address \{"

		puts $dtsFile "$sub_node_spacing compatible = \"$memory_node_compatible\";"

		puts $dtsFile "$sub_node_spacing device type = \"$memory_node_device_type\";"
		puts $dtsFile "$sub_node_spacing reg = $memory_node_reg;"
		
		# end memory node
		puts $dtsFile "\n$node_spacing \};"

	}
}

proc gen_soc_subsystem_int_controller {&dtsFile soc_subsystem soc_subsystem_name} {

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	# generic node properties keys
	set soc_subsystem_id_key "id"
	set soc_subsystem_phandle_key "phandle"
	set soc_subsystem_compatible_key "compatible"
	set soc_subsystem_address_cells_key "#address-cells"
	set soc_subsystem_reg_key "reg"
	set soc_subsystem_reg_names_key "reg-names"
	set soc_subsystem_reg_address_key "reg_address"
	set soc_subsystem_reg_size_key "reg_size"

	# int controller specific node properties keys
	set soc_subsystem_int_controller_interrupt_cells_key "#interrupt-cells"
	set soc_subsystem_int_controller_interrupt_controller_key "interrupt-controller"
	set soc_subsystem_int_controller_riscv_priority_key "riscv,max-priority"
	set soc_subsystem_int_controller_riscv_ndev_key "riscv,ndev"
	set soc_subsystem_int_controller_interrupts_extended_key "interrupts-extended"

	# getting interrupt controller node properties values
	
	# generic node properties
	set soc_subsystem_id [dict get $soc_subsystem $soc_subsystem_id_key]
	set soc_subsystem_phandle [dict get $soc_subsystem $soc_subsystem_phandle_key]
	set soc_subsystem_compatible [dict get $soc_subsystem $soc_subsystem_compatible_key]
	set soc_subsystem_address_cells [dict get $soc_subsystem $soc_subsystem_address_cells_key]
	set soc_subsystem_reg [dict get $soc_subsystem $soc_subsystem_reg_key]
	set soc_subsystem_reg_names [dict get $soc_subsystem $soc_subsystem_reg_names_key]
	set soc_subsystem_reg_address [dict get $soc_subsystem $soc_subsystem_reg_address_key]
	set soc_subsystem_reg_size [dict get $soc_subsystem $soc_subsystem_reg_size_key]
	
	# int controller specific node properties 
	set soc_subsystem_int_controller_interrupt_cells [dict get $soc_subsystem $soc_subsystem_int_controller_interrupt_cells_key]
	set soc_subsystem_int_controller_interrupt_controller [dict get $soc_subsystem $soc_subsystem_int_controller_interrupt_controller_key]
	set soc_subsystem_int_controller_riscv_priority [dict get $soc_subsystem $soc_subsystem_int_controller_riscv_priority_key]
	set soc_subsystem_int_controller_riscv_ndev [dict get $soc_subsystem $soc_subsystem_int_controller_riscv_ndev_key]
	set soc_subsystem_int_controller_interrupts_extended [dict get $soc_subsystem $soc_subsystem_int_controller_interrupts_extended_key]


	# puts "\n\n Int contr subnode stuff below \n\n"
	# puts "\n\n soc_subsystem_id = $soc_subsystem_id \n\n"
	# puts "\n\n soc_subsystem_id list size = [llength $soc_subsystem_id] \n\n"
	# 	# soc_subsystem_id list size = 1
	# puts "\n\n soc_subsystem_reg_names = $soc_subsystem_reg_names \n\n"
	# puts "\n\n soc_subsystem_reg_address = $soc_subsystem_reg_address \n\n"
	# puts "\n\n soc_subsystem_reg_address list size = [llength $soc_subsystem_reg_address] \n\n"
	# 	# soc_subsystem_reg_address list size = 3
	# puts "\n\n soc_subsystem_reg_size = $soc_subsystem_reg_size \n\n"
	# puts "\n\n soc_subsystem_reg = $soc_subsystem_reg \n\n"
	# puts "\n\n soc_subsystem_int_controller_interrupts_extended = $soc_subsystem_int_controller_interrupts_extended \n\n"
	# puts "\n\n soc_subsystem_int_controller_interrupt_cells = $soc_subsystem_int_controller_interrupt_cells \n\n"


	# exit 2
	set soc_subsystem_starting_address "undefined"

	# extracting starting addresss from multiple addresses in the reg address 
	if {[llength $soc_subsystem_reg_address] > 1} {
		set soc_subsystem_starting_address [lindex $soc_subsystem_reg_address 0] 
	} elseif {[llength $soc_subsystem_reg_address] == 1} {
		set soc_subsystem_starting_address $soc_subsystem_reg_address
	}

	set list_of_reg_names {}

	foreach reg_names $soc_subsystem_reg_names {

		set reg_name_holder "\"$reg_names\""
		lappend list_of_reg_names $reg_name_holder

	}

	set reg_names_string_holder [join $list_of_reg_names ", "]

	puts "\n\n reg_names_string_holder = $reg_names_string_holder \n\n"
		# reg_names_string_holder = "prio, ", "irq_en, ", "reg, " 
	# puts "\n\n list_of_reg_names = $list_of_reg_names \n\n"
	# puts "\n\n list_of_reg_names = [lindex $list_of_reg_names 0] \n\n"

	# exit 2

	puts $dtsFile "\n\n$sub_node_spacing $soc_subsystem_phandle: $soc_subsystem_name@$soc_subsystem_starting_address \{"

	puts $dtsFile "\n$sub_node_spacing\t $soc_subsystem_compatible_key = \"$soc_subsystem_compatible\";"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_address_cells_key = $soc_subsystem_address_cells;"
	# puts $dtsFile "$sub_node_spacing\t #size-cells = <1>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_int_controller_interrupt_cells_key = $soc_subsystem_int_controller_interrupt_cells;"
	
	if {$soc_subsystem_int_controller_interrupt_controller == ""} {
		puts $dtsFile "$sub_node_spacing\t $soc_subsystem_int_controller_interrupt_controller_key;"
	}
	
	# generating generic sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_key = $soc_subsystem_reg;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_names_key = $reg_names_string_holder;"
	
	# generating interrupt controller specific sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_int_controller_riscv_priority_key = <$soc_subsystem_int_controller_riscv_priority>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_int_controller_riscv_ndev_key = <$soc_subsystem_int_controller_riscv_ndev>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_int_controller_interrupts_extended_key = <$soc_subsystem_int_controller_interrupts_extended>;"
	puts $dtsFile "\n$sub_node_spacing \};"

	# exit 2
}


proc gen_soc_subsystem_uart {&dtsFile soc_subsystem soc_subsystem_name} {

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	# generic node properties keys
	set soc_subsystem_id_key "id"
	set soc_subsystem_phandle_key "phandle"
	set soc_subsystem_compatible_key "compatible"
	# set soc_subsystem_address_cells_key "#address-cells"
	set soc_subsystem_reg_key "reg"
	# set soc_subsystem_reg_names_key "reg-names"
	set soc_subsystem_reg_address_key "reg_address"
	set soc_subsystem_reg_size_key "reg_size"

	# uart specific node properties keys
	set soc_subsystem_uart_interrupts_key "interrupts"
	set soc_subsystem_uart_interrupt_parent_key "interrupt-parent"
	set soc_subsystem_uart_clock_frequency_key "clock-frequency"
	set soc_subsystem_uart_reg_shift_key "reg-shift"
	set soc_subsystem_uart_status_key "status"

	# getting uart node properties values
	
	# generic node properties
	set soc_subsystem_id [dict get $soc_subsystem $soc_subsystem_id_key]
	set soc_subsystem_phandle [dict get $soc_subsystem $soc_subsystem_phandle_key]
	set soc_subsystem_compatible [dict get $soc_subsystem $soc_subsystem_compatible_key]
	# set soc_subsystem_address_cells [dict get $soc_subsystem $soc_subsystem_address_cells_key]
	set soc_subsystem_reg [dict get $soc_subsystem $soc_subsystem_reg_key]
	# set soc_subsystem_reg_names [dict get $soc_subsystem $soc_subsystem_reg_names_key]
	set soc_subsystem_reg_address [dict get $soc_subsystem $soc_subsystem_reg_address_key]
	set soc_subsystem_reg_size [dict get $soc_subsystem $soc_subsystem_reg_size_key]
	
	# uart specific node properties 
	set soc_subsystem_uart_interrupts [dict get $soc_subsystem $soc_subsystem_uart_interrupts_key]
	set soc_subsystem_uart_interrupt_parent [dict get $soc_subsystem $soc_subsystem_uart_interrupt_parent_key]
	set soc_subsystem_uart_clock_frequency [dict get $soc_subsystem $soc_subsystem_uart_clock_frequency_key]
	set soc_subsystem_uart_reg_shift [dict get $soc_subsystem $soc_subsystem_uart_reg_shift_key]
	set soc_subsystem_uart_status [dict get $soc_subsystem $soc_subsystem_uart_status_key]

	set soc_subsystem_starting_address "undefined"

	# extracting starting addresss from multiple addresses in the reg address 
	if {[llength $soc_subsystem_reg_address] > 1} {
		set soc_subsystem_starting_address [lindex $soc_subsystem_reg_address 0] 
	} elseif {[llength $soc_subsystem_reg_address] == 1} {
		set soc_subsystem_starting_address $soc_subsystem_reg_address
	}

	# generating generic sub-node properties
	puts $dtsFile "\n\n$sub_node_spacing $soc_subsystem_phandle: $soc_subsystem_name@$soc_subsystem_starting_address \{"
	puts $dtsFile "\n$sub_node_spacing\t $soc_subsystem_compatible_key = \"$soc_subsystem_compatible\";"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_key = $soc_subsystem_reg;"

	# generating uart specific sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_uart_interrupts_key = <$soc_subsystem_uart_interrupts>;"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_uart_interrupt_parent_key = <$soc_subsystem_uart_interrupt_parent>;"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_uart_clock_frequency_key = <$soc_subsystem_uart_clock_frequency>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_uart_reg_shift_key = <$soc_subsystem_uart_reg_shift>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_uart_status_key = \"$soc_subsystem_uart_status\";"
	puts $dtsFile "\n$sub_node_spacing \};"

	# exit 2

}

proc gen_soc_subsystem_gpio {&dtsFile soc_subsystem soc_subsystem_name} {

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	# generic node properties keys
	set soc_subsystem_id_key "id"
	set soc_subsystem_phandle_key "phandle"
	set soc_subsystem_compatible_key "compatible"
	# set soc_subsystem_address_cells_key "#address-cells"
	set soc_subsystem_reg_key "reg"
	# set soc_subsystem_reg_names_key "reg-names"
	set soc_subsystem_reg_address_key "reg_address"
	set soc_subsystem_reg_size_key "reg_size"

	# gpio specific node properties keys
	set soc_subsystem_gpio_gpio_controller_key "gpio-controller"
	set soc_subsystem_gpio_ngpios_key "ngpios"
	set soc_subsystem_gpio_gpio_cells_key "#gpio-cells"
	set soc_subsystem_gpio_interrupts_key "interrupts"
	set soc_subsystem_gpio_interrupt_parent_key "interrupt-parent"
	set soc_subsystem_gpio_clock_frequency_key "clock-frequency"
	set soc_subsystem_gpio_reg_shift_key "reg-shift"
	set soc_subsystem_gpio_status_key "status"

	# getting gpio node properties values
	
	# generic node properties
	set soc_subsystem_id [dict get $soc_subsystem $soc_subsystem_id_key]
	set soc_subsystem_phandle [dict get $soc_subsystem $soc_subsystem_phandle_key]
	set soc_subsystem_compatible [dict get $soc_subsystem $soc_subsystem_compatible_key]
	# set soc_subsystem_address_cells [dict get $soc_subsystem $soc_subsystem_address_cells_key]
	set soc_subsystem_reg [dict get $soc_subsystem $soc_subsystem_reg_key]
	# set soc_subsystem_reg_names [dict get $soc_subsystem $soc_subsystem_reg_names_key]
	set soc_subsystem_reg_address [dict get $soc_subsystem $soc_subsystem_reg_address_key]
	set soc_subsystem_reg_size [dict get $soc_subsystem $soc_subsystem_reg_size_key]
	
	# gpio specific node properties 
	set soc_subsystem_gpio_gpio_controller [dict get $soc_subsystem $soc_subsystem_gpio_gpio_controller_key]
	set soc_subsystem_gpio_ngpios [dict get $soc_subsystem $soc_subsystem_gpio_ngpios_key]
	set soc_subsystem_gpio_gpio_cells [dict get $soc_subsystem $soc_subsystem_gpio_gpio_cells_key]
	set soc_subsystem_gpio_interrupts [dict get $soc_subsystem $soc_subsystem_gpio_interrupts_key]
	set soc_subsystem_gpio_interrupt_parent [dict get $soc_subsystem $soc_subsystem_gpio_interrupt_parent_key]
	set soc_subsystem_gpio_status [dict get $soc_subsystem $soc_subsystem_gpio_status_key]

	set soc_subsystem_starting_address "undefined"

	# extracting starting addresss from multiple addresses in the reg address 
	if {[llength $soc_subsystem_reg_address] > 1} {
		set soc_subsystem_starting_address [lindex $soc_subsystem_reg_address 0] 
	} elseif {[llength $soc_subsystem_reg_address] == 1} {
		set soc_subsystem_starting_address $soc_subsystem_reg_address
	}

	# generating generic sub-node properties
	puts $dtsFile "\n\n$sub_node_spacing $soc_subsystem_phandle: $soc_subsystem_name@$soc_subsystem_starting_address \{"
	puts $dtsFile "\n$sub_node_spacing\t $soc_subsystem_compatible_key = \"$soc_subsystem_compatible\";"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_key = $soc_subsystem_reg;"

	# generating gpio specific sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_interrupts_key = <$soc_subsystem_gpio_interrupts>;"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_interrupt_parent_key = <$soc_subsystem_gpio_interrupt_parent>;"	
	if {$soc_subsystem_gpio_gpio_controller == ""} {
		puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_gpio_controller_key;"
	}
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_ngpios_key = <$soc_subsystem_gpio_ngpios>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_gpio_cells_key = <$soc_subsystem_gpio_gpio_cells>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_gpio_status_key = \"$soc_subsystem_gpio_status\";"
	puts $dtsFile "\n$sub_node_spacing \};"

	# exit 2


}


proc gen_soc_subsystem_syscon {&dtsFile soc_subsystem soc_subsystem_name} {

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	# generic node properties keys
	set soc_subsystem_id_key "id"
	set soc_subsystem_phandle_key "phandle"
	set soc_subsystem_compatible_key "compatible"
	# set soc_subsystem_address_cells_key "#address-cells"
	set soc_subsystem_reg_key "reg"
	# set soc_subsystem_reg_names_key "reg-names"
	set soc_subsystem_reg_address_key "reg_address"
	set soc_subsystem_reg_size_key "reg_size"

	# syscon specific node properties keys
	set soc_subsystem_syscon_status_key "status"

	# getting syscon node properties values
	
	# generic node properties
	set soc_subsystem_id [dict get $soc_subsystem $soc_subsystem_id_key]
	set soc_subsystem_phandle [dict get $soc_subsystem $soc_subsystem_phandle_key]
	set soc_subsystem_compatible [dict get $soc_subsystem $soc_subsystem_compatible_key]
	# set soc_subsystem_address_cells [dict get $soc_subsystem $soc_subsystem_address_cells_key]
	set soc_subsystem_reg [dict get $soc_subsystem $soc_subsystem_reg_key]
	# set soc_subsystem_reg_names [dict get $soc_subsystem $soc_subsystem_reg_names_key]
	set soc_subsystem_reg_address [dict get $soc_subsystem $soc_subsystem_reg_address_key]
	set soc_subsystem_reg_size [dict get $soc_subsystem $soc_subsystem_reg_size_key]
	
	# syscon specific node properties 
	set soc_subsystem_syscon_status [dict get $soc_subsystem $soc_subsystem_syscon_status_key]

	set soc_subsystem_starting_address "undefined"

	# extracting starting addresss from multiple addresses in the reg address 
	if {[llength $soc_subsystem_reg_address] > 1} {
		set soc_subsystem_starting_address [lindex $soc_subsystem_reg_address 0] 
	} elseif {[llength $soc_subsystem_reg_address] == 1} {
		set soc_subsystem_starting_address $soc_subsystem_reg_address
	}

	# generating generic sub-node properties
	puts $dtsFile "\n\n$sub_node_spacing $soc_subsystem_phandle: $soc_subsystem_name@$soc_subsystem_starting_address \{"
	puts $dtsFile "\n$sub_node_spacing\t $soc_subsystem_compatible_key = \"$soc_subsystem_compatible\";"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_key = $soc_subsystem_reg;"	

	# generating syscon specific sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_syscon_status_key = \"$soc_subsystem_syscon_status\";"
	puts $dtsFile "\n$sub_node_spacing \};"

	# exit 2
}


proc gen_soc_subsystem_timer {&dtsFile soc_subsystem soc_subsystem_name} {

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	# generic node properties keys
	set soc_subsystem_id_key "id"
	set soc_subsystem_phandle_key "phandle"
	set soc_subsystem_compatible_key "compatible"
	# set soc_subsystem_address_cells_key "#address-cells"
	set soc_subsystem_reg_key "reg"
	# set soc_subsystem_reg_names_key "reg-names"
	set soc_subsystem_reg_address_key "reg_address"
	set soc_subsystem_reg_size_key "reg_size"

	# timer specific node properties keys
	set soc_subsystem_timer_interrupts_extended_key "interrupts-extended"
	set soc_subsystem_timer_status_key "status"

	# getting timer node properties values
	
	# generic node properties
	set soc_subsystem_id [dict get $soc_subsystem $soc_subsystem_id_key]
	set soc_subsystem_phandle [dict get $soc_subsystem $soc_subsystem_phandle_key]
	set soc_subsystem_compatible [dict get $soc_subsystem $soc_subsystem_compatible_key]
	# set soc_subsystem_address_cells [dict get $soc_subsystem $soc_subsystem_address_cells_key]
	set soc_subsystem_reg [dict get $soc_subsystem $soc_subsystem_reg_key]
	# set soc_subsystem_reg_names [dict get $soc_subsystem $soc_subsystem_reg_names_key]
	set soc_subsystem_reg_address [dict get $soc_subsystem $soc_subsystem_reg_address_key]
	set soc_subsystem_reg_size [dict get $soc_subsystem $soc_subsystem_reg_size_key]
	
	# timer specific node properties 
	set soc_subsystem_timer_interrupts_extended [dict get $soc_subsystem $soc_subsystem_timer_interrupts_extended_key]
	set soc_subsystem_timer_status [dict get $soc_subsystem $soc_subsystem_timer_status_key]

	set soc_subsystem_starting_address "undefined"

	# extracting starting addresss from multiple addresses in the reg address 
	if {[llength $soc_subsystem_reg_address] > 1} {
		set soc_subsystem_starting_address [lindex $soc_subsystem_reg_address 0] 
	} elseif {[llength $soc_subsystem_reg_address] == 1} {
		set soc_subsystem_starting_address $soc_subsystem_reg_address
	}

	# generating generic sub-node properties
	puts $dtsFile "\n\n$sub_node_spacing $soc_subsystem_phandle: $soc_subsystem_name@$soc_subsystem_starting_address \{"
	puts $dtsFile "\n$sub_node_spacing\t $soc_subsystem_compatible_key = \"$soc_subsystem_compatible\";"	
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_reg_key = $soc_subsystem_reg;"	

	# generating timer specific sub-node properties
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_timer_interrupts_extended_key = <$soc_subsystem_timer_interrupts_extended>;"
	puts $dtsFile "$sub_node_spacing\t $soc_subsystem_timer_status_key = \"$soc_subsystem_timer_status\";"
	puts $dtsFile "\n$sub_node_spacing \};"

	# exit 2

}


# generate soc subsystem
proc gen_soc_subsystem	{&dtsFile soc_subsystem soc_subsystem_name} {

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	if {$soc_subsystem_name == "interrupt-controller"} {

		gen_soc_subsystem_int_controller dtsFile $soc_subsystem $soc_subsystem_name

	} elseif {$soc_subsystem_name == "uart"} {
		
		gen_soc_subsystem_uart dtsFile $soc_subsystem $soc_subsystem_name

	} elseif {$soc_subsystem_name == "gpio"} {

		gen_soc_subsystem_gpio dtsFile $soc_subsystem $soc_subsystem_name

	} elseif {$soc_subsystem_name == "syscon"} {

		gen_soc_subsystem_syscon dtsFile $soc_subsystem $soc_subsystem_name

	} elseif {$soc_subsystem_name == "timer"} {

		gen_soc_subsystem_timer dtsFile $soc_subsystem $soc_subsystem_name

	}

}

# generate soc sdt node
proc gen_soc_node {&dtsFile soc_address_cells soc_size_cells soc_compatible soc_ranges soc_subsystems_list} {
	#This proc generates the soc node in the DTS

	set node_spacing "\t\t"
	set sub_node_spacing "\t\t\t\t\t"
	set sub_sub_node_spacing "\t\t\t\t\t\t\t"

	# the & is for denoting that we are passing these args by reference
	upvar ${&dtsFile} dtsFile

	puts $dtsFile "\n$node_spacing soc \{"

	puts $dtsFile "$sub_node_spacing compatible = \"$soc_compatible\";"
	puts $dtsFile "$sub_node_spacing #address-cells = $soc_address_cells;"
	# puts $dtsFile "$sub_node_spacing #address-cells = $current_file_top_module_address;"
	puts $dtsFile "$sub_node_spacing #size-cells = $soc_size_cells;"
	# puts $dtsFile "$sub_node_spacing #size-cells = $current_file_top_module_size;"
	# puts $dtsFile "$sub_node_spacing reg = <0 0 0 0x1000>;"

	if {$soc_ranges == ""} {
		puts $dtsFile "$sub_node_spacing ranges;"	
	}
	
	# puts $soc_subsystems_list

	set i 0
	foreach soc_subsystem $soc_subsystems_list {
		# prints all subsystems successfully
		puts "\n\n soc subsystem #$i:"
		puts [lindex $soc_subsystems_list $i]
		incr i
		# exit 2

		set soc_subsystem_name [dict get $soc_subsystem "subsystem"]


		# function to generate soc subsystem
		gen_soc_subsystem	dtsFile $soc_subsystem $soc_subsystem_name
			# we can also have a get_soc_subsystem tcl api function separately as well

	}

	puts $dtsFile "\n$node_spacing \};"
	# end soc node

}

proc gen_mem_node {} {
	#This proc generates the MEMORY node in the DTS
	
}

proc gen_amba_node {} {
	#This proc generates all of the AMBA/AXI nodes in the DTS
	
}

proc gen_domain_node {} {
	#This proc generates any relevant DOMAIN nodes in the DTS
	
}




# ################ MAIN EXECUTION IS BELOW #####################3

#Get the full path of the current script
set script_path [ file dirname [ file normalize [ info script ] ] ]
set output_file ""
set file_io_mode "r"
	
#Check number of arguments
if { $argc < 2 } {
	puts "The SDTGen script requires AT LEAST an output file"
	puts "Example: $script_path -o <output_file>"
	puts "Options:"
	puts "-o <output_file> : the full path of the output file"
	puts "-f               : force the creation of the file.  will overwrite existing file"
	exit
}

#iterate over all of the command line arguments
for {set i 0} {$i < $argc} {incr i} {
	set tempVar [lindex $argv $i]
	switch $tempVar {
	"-o" {
		set output_file [lindex $argv [expr $i + 1] ]
		incr i
	}
	
	"-f" {
		set file_io_mode "w"
	}
	
	}
}	
	
	
#check if the output file exists
if { [file exists $output_file] == 1} {
	if { $file_io_mode != "w" } {
		puts "Output file ($output_file) exists"
		puts "Use the -f (force) argument to allow overwriting"
		exit
	}

}

#open the output file target
set dtsFile [open $output_file w+]

#start building a DTS
puts $dtsFile [gen_copyright_header]

puts $dtsFile "/dts-v1/;"
puts $dtsFile " "
puts $dtsFile "/ {"

#Generate the DTS file pre-amble (compatible, model, version, etc)
gen_dts_preamble dtsFile

set json_file_path "./JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v3.json"


# get dict of all SDT nodes from JSON file
set sdt_nodes_dict [get_sdt_nodes_dict_from_json $json_file_path]

# get cpus node meta-data
get_cpus $sdt_nodes_dict cpus_inst_list cpus_size_cell cpus_address_cell cpus_timebase_frequency

# get cpu-clusters node meta-data
# Right now cpu clusters means the BCPU ... we can combine cpus and cpu-cluster nodes later 
get_cpus_clusters $sdt_nodes_dict cpus_cluster_inst_list cpus_cluster_size_cell cpus_cluster_address_cell  cpus_cluster_compatible 


# get memory nodes meta-data
get_memory_nodes $sdt_nodes_dict memory_modules_inst_list

# get soc user-logic meta-data (ips)
	# get_ips now named "get_soc" function updated according to new JSON file gold_hier_v3.json 
get_soc $sdt_nodes_dict soc_address_cells soc_size_cells soc_compatible soc_ranges soc_subsystems_list

# writing cpus node
gen_cpu_node dtsFile $cpus_inst_list $cpus_address_cell $cpus_size_cell $cpus_timebase_frequency
	# needed to remove $ with the dtsFile arg

# writing cpus_cluster node
gen_cpu_cluster_node dtsFile $cpus_cluster_address_cell $cpus_cluster_size_cell $cpus_cluster_compatible $cpus_cluster_inst_list

# writing memory node 
gen_memory_nodes dtsFile $memory_modules_inst_list 

# Wrtiting user-logic under SOC sdt node
gen_soc_node dtsFile $soc_address_cells $soc_size_cells $soc_compatible $soc_ranges $soc_subsystems_list

puts $dtsFile ""


puts $dtsFile "\};"

#close the output file target
close $dtsFile

