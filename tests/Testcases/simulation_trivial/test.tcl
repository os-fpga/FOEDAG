create_design test
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_macro P1=10  P2=20
add_design_file test.v -SV_2012
set_top_module top
add_simulation_file -CPP sim_main.cpp

# test clear
clear_simulation_files

# add again
add_simulation_file -CPP sim_main.cpp

# regular flow
synth
packing
place
route
sta
power
bitstream
puts "done!"
