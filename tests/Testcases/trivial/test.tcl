create_design test
set_top_module top
set_macro P1=10  P2=20
add_include_path ./ inc/
add_design_file test.v bottom.v -SV_2012
synth
packing
place
route
sta
power
bitstream
sync # Must use for now for proper exit
puts "done!"
exit


