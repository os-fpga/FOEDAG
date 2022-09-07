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


# This test is for https://rapidsilicon.atlassian.net/browse/GEMINIEDA-332
# The purpose of this test is to ensure that when no IP instaces have been
# created, the ipgenerate command doesn't inaccurately print
# "Design ip_gen_instances IPs are generated!"
# Based off our current harness, this test should be followed by:
#   grep -L "IPs are generated" foedag.log
# Which will error out if "IPs are generated" is found in foedag.log

create_design ip_gen_instances
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
add_litex_ip_catalog /usr/local/share/foedag/IP_Catalog
ipgenerate
