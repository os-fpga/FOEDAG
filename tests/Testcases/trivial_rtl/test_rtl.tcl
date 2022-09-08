create_design test_rtl
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_macro P1=10  P2=20
add_include_path  inc/
add_design_file -work lib1 bottom.v -SV_2012
add_design_file -work lib2 center.v -SV_2012
add_design_file -L lib1 -L lib2 test.v  -SV_2012
set_top_module top
synth
packing
place
route
sta
power
bitstream
puts "done!"


