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
######################################################################

# This is a test for GEMINIEDA-352
# This test ensures that the IP_Catalog is automatically loaded when a user
# runs a script with configure_ip, but doesn't bother to call add_litex_ip_catalog

create_design ip_load
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml

# Typically you should load your IP_Catalog via add_litex_ip_catalog, but GEMINIEDA-352
# requests new functionality to auto load the ip library before a configure_ip

configure_ip axis_converter_V1_0 -mod_name axis_converter -version V1_1 -Pcore_in_width=128 -Pcore_out_width=64 -Pcore_user_width=0 -Pcore_reverse=0 -out_file ./ip_load/ip_load.IPs/RapidSilicon/IP/axis_converter/V1_1/axis_converter/src/axis_converter.v
ipgenerate

# If the IP fails to generate, it means that axis_converter_V1_0 never got
# automatically loaded into the IP_Catalog

# Error out if "IPs are generated" wasn't printed
set fp [open "foedag.log" r]
set file_data [read $fp]
close $fp
set failed [regexp "IPs are generated" $file_data]
# invert the return value so it the script will error properly
set failed [expr {!$failed}]
if { $failed == 1 } {
    puts "ERROR: configure_ip command failed to generate ip"
}
exit $failed


