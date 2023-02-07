#Copyright 2022 The Foedag team

#GPL License

#Copyright (c) 2022 The Open-Source FPGA Foundation

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

create_design testapis
file copy -force ./tests/Testcases/DesignQuery/hier_info.json ./testapis/.
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
add_design_file test.v -SV_2012
set_top_module top
synth

# Test get_file_ids and error out if the correct file ids weren't captured from the dummy data in hier_info.json
set ids [get_file_ids]
if { ![string compare $ids "1 10 11 12 2 3 4 5 6 7 8 9"] } {
    puts "TEST FAILED: get_file_ids should have generated 1 10 11 12 2 3 4 5 6 7 8 9"
    exit 1
}

exit 0
