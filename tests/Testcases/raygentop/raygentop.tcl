create_design RAYGENTOP
set_top_module paj_raygentop_hierarchy_no_mem
add_design_file raygentop.v
batch {
  synth
  packing
  place
  route
  sta
  power
  bitstream
  puts "done!"
  exit
}


