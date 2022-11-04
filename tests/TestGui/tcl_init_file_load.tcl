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



# This test is intended to be run w/ --replay so that we can create a
# foedag_init.tcl file (if it doesn't exist already) before the gui loads

# open file w/ append so we don't overwrite a real foedag_init.tcl if we find one
set initFile "foedag_init.tcl"
set fp [open $initFile a+];
close $fp;

puts [glob *.tcl]
puts [pwd]

# load gui which should now find foedag_init.tcl locally
gui_start
gui_stop

set fp [open "foedag_cmd.tcl" r]
set file_data [read $fp]
# check the log for "source <somePath>/foedag_init.tcl"
set found [regexp "source.*$initFile" $file_data]

puts $file_data
puts $found

# Error out if the source command wasn't found
# This is one line as --replay currently doesn't work with multi-line expressions like conditionals
if {!$found} { puts "TEST FAILED: source \"$initFile\" not found in foedag_cmd.tcl"; exit 1 }

# pass if we make it this far
exit 0
