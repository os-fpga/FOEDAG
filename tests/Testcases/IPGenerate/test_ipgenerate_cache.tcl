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

proc readLog {} {
    set fp [open "foedag.log" r]
    set file_data [read $fp]
    close $fp
    return $file_data
}

set platform $::tcl_platform(platform)
if { $platform == "windows" } {
    # TODO @skyler-rs Oct2022 Enable in windows once GH-661 is resolved
    puts "SKIPPING ON WINDOWS: This test requires python which FileUtils::ExecuteSystemCommand() currently fails to find on Windows. Disabling windows run of test until python issues on windows are resolved.\n"
    # returning a pass condition because this is an expected failure for now
    exit 0
} else {
    # This test creates and uses cache files so we need to delete them otherwise the test will fail on re-run
    file delete -force ip_gen_cache

    # Load IPs
    create_design ip_gen_cache
    architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
    add_litex_ip_catalog ./IP_Catalog

    # Configure an IP w/ module
    configure_ip axis_converter_V1_0 -mod_name inst1 -version V1_0 -Pcore_in_width=128 -Pcore_out_width=64 -Pcore_user_width=0 -Pcore_reverse=0 -out_file rs_ips/inst1

    # Generate our IP for the first time
    ipgenerate

    # Error out if reusing message was printed
    set file_data [readLog]
    set found [regexp "reusing IP" $file_data]
    if { $found } {
        puts "TEST FAILED: ipgenerate shouldn't have reused an IP at this stage"
        exit 1
    }

    # Generate our IP again, this will use the cached copy
    ipgenerate

    # Ensure that reusing IP has been printed
    set file_data [readLog]
    set found [regexp "reusing IP" $file_data]
    if { $found == 0 } {
        puts "TEST FAILED: ipgenerate should have re-used the IP on second generate"
        exit 1
    }

    # remove the IPs dir which will remove the cached IP as well
    file delete -force ./ip_gen_cache/run_1/IPs/

    # Generate again, the cache file has been removed so it shouldn't re-use
    ipgenerate

    # ensure a new reusing IP message isn't printed
    set file_data [readLog]
    set foundCount [regexp -all "reusing IP" $file_data]
    if { $foundCount > 1 } {
        puts "TEST FAILED: ipgenerate should not have re-used the IP after the IPs dir was removed"
        exit 1
    }

    exit 0
}
