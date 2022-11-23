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

gui_start
# invoke gtkwave and send it a command asking it to get the wave width and print it w/ _RETURN_ in front
wave_open
after 1000
set test [wave_cmd set w \[gtkwave::getWaveWidth\]\nputs \"_RETURN_\$w\"]
# retrieve returned results from the gtkwave process
after 1000
set result [wave_get_return]
puts $result
after 1000
# close gtkwave via the menu
wave_cmd gtkwave::/File/Quit

# If we got a number result and it's greater than 0, then we know communication is working
# This must be 1 line because --replay doesn't support multi-line tcl
if { $result != "" && [expr 0 < $result] } { puts "Successfully retrieved result from gtkwave process" } else { puts "FAILED to communicate with gtkwave process"; exit 1 }
