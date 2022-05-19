create_design AES_DECRYPT
set_top_module decrypt
add_design_file aes_decrypt128.sv aes_decrypt256.sv  gfmul.sv InvMixCol_slice.sv InvSbox.sv InvSubBytes.sv KeyExpand192.sv  KschBuffer.sv  Sbox.sv aes_decrypt192.sv  decrypt.sv InvAddRoundKey.sv InvMixColumns.sv  InvShiftRows.sv KeyExpand128.sv KeyExpand256.sv RotWord.sv  SubWord.sv  -SV_2012
add_design_file generic_muxfx.v
add_constraint_file aes_decrypt.sdc
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
bitstream_config_files ../../Arch/bitstream_annotation.xml ../../Arch/fixed_sim_openfpga.xml ../../Arch/repack_design_constraint.xml
synthesize
packing
place
route
bitstream force
puts "done!"
