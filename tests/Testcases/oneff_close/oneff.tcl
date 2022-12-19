create_design oneff
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_top_module top
add_design_file oneff.v
add_constraint_file oneff.sdc
synth
packing
place
route
sta
power
bitstream
puts "done!"

close_design

create_design oneff
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_top_module top
add_design_file oneff.v
add_constraint_file oneff.sdc
synth
packing
place
route
sta
power
bitstream
puts "done!"


