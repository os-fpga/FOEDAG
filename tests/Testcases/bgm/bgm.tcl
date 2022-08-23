create_design BGM
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
set_top_module bgm
add_design_file bgm.v
synth

