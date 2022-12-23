#Copyright 2021 The Foedag team

#GPL License

#Copyright (c) 2021 The Open-Source FPGA Foundation

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.

create_design simulation_flow_test
set_top_module toto
ipgenerate
simulation_options verilator simulation rtl "some options"
# doesn't work for now
# simulate rtl rtl_file.fst
synth
simulation_options verilator elab gate "some options"
# doesn't work for now
# simulate gate gate_file.fst
packing
globp
place
route
simulation_options verilator comp pnr "some options"
# doesn't work for now
# simulate pnr pnr_file.fst
sta
power
bitstream
simulation_options verilator comp bitstream "some options"
# doesn't work for now
# simulate bitstream bitstream_file.fst
exit
