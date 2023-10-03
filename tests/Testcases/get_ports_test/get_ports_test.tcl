proc verify {input output} {
  if {$input != $output} {
    error "Expected output: $output, but got this: $input"
  }
}

create_design get_ports_test
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_top_module top
add_design_file get_ports_test.v

set hier_info [file join [file dirname [info script]] hier_info.json]

add_constraint_file get_ports_test.sdc
# create folder structure
analyze 

file copy -force $hier_info get_ports_test/run_1/synth_1_1/analysis/

verify [get_ports {*}] "d rstn clock0 q"
verify [get_ports {cl*}] "clock0"
verify [get_ports {d}] "d"
verify [get_ports {clock0}] "clock0"
verify [get_ports {clock1}] ""
verify [get_ports [all_inputs]] "d rstn clock0"
verify [get_ports [all_outputs]] "q"
