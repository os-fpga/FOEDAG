create_design oneff
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


