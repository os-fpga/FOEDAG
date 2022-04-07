create_design BLAH
set_top_module toto
batch {
ipgenerate
synth
packing
globp
place
route
sta
power
bitstream
}


after 22000 {set CONT 0}
set CONT 1 
while {$CONT} {
  set a 0
  after 100 set a 1
  vwait a
}
exit