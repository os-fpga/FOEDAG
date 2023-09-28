proc verify {input output} {
  if {$input != $output} {
    error "Extected output: $output, but got this: $input"
  }
}

create_design get_ports_buses
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_top_module top
add_design_file test.v

set hier_info [file join [file dirname [info script]] hier_info.json]

# create folder structure
analyze 

file copy -force $hier_info get_ports_buses/run_1/synth_1_1/analysis/

verify [get_ports [all_inputs]] "clk_i rst_ni tl_i alert_rx_i cio_gpio_i"
verify [get_ports {tl_i[0] tl_i[108] tl_i[109]}] "tl_i\[0\] tl_i\[108\]"
verify [get_ports {rst_ni[0]}] ""
