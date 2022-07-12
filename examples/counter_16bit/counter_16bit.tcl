create_design counter_16bit
set_top_module counter_16bit
add_design_file counter_16bit.v -V_2001
ipgenerate
synth
packing
place
route
#sta
#power
bitstream force
puts "done!"
show_settings

