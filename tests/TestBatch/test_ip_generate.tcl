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

set platform $::tcl_platform(platform)
if { $platform == "windows" } {
    # TODO @skyler-rs Oct2022 Enable in windows once GH-661 is resolved
    puts "SKIPPING ON WINDOWS: This test requires python which FileUtils::ExecuteSystemCommand() currently fails to find on Windows. Disabling windows run of test until python issues on windows are resolved.\n"
    # returning a pass condition because this is an expected failure for now
    exit 0
} else {
    # Load IPs
    create_design ip_test
    architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
    add_litex_ip_catalog ../Testcases/IPGenerate/IP_Catalog

    # Configure an IP w/ module named inst1
    configure_ip axis_converter_V1_0 -mod_name inst1 -version V1_0 -Pcore_in_width=128 -Pcore_out_width=64 -Pcore_user_width=0 -Pcore_reverse=0 -out_file rs_ips/inst1

    # Generate ip
    ipgenerate

    set fp [open "foedag.log" r]
    set file_data [read $fp]
    close $fp
    # Error out if inst1 wasn't generated
    set found [regexp "V1_0/inst1" $file_data]
    if { !$found } {
        puts "TEST FAILED: ipgenerate failed to generate inst1"
        exit 1
    }

    exit 0
}
