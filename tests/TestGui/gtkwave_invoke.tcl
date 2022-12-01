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

##############################################################################
# The purpose of this test is to verify that we only open one GTKWave process
# and continue to use that process until it is closed at which point a new
# instance will be started on the next wave related command
##############################################################################

# these must be one line for --replay to work
proc assertStrCount {str expected} { set fp [open "foedag.log" r]; set file_data [read $fp]; close $fp; set count [regexp -all "$str" $file_data]; if {$count != $expected} { puts "FAILED - Expected $expected instances of \"$str\", got $count\n"; exit 1} }
proc assertInvokeCount {count} { assertStrCount "GTKWave - Interpreter" $count }
proc assertExitCount {count} { assertStrCount "GTKWave - Exiting" $count }
proc assertStr { str expected } { if { $str != $expected} { puts "FAILED - Expected $expected, got $str\n"; exit 1}}
proc checkForWaveOpen {} { set fp [open "foedag.log" r]; set file_data [read $fp]; close $fp; set count [regexp -all "start time." $file_data]; return $count; }

gui_start

# Ensure gtkwave hasn't been opened
assertInvokeCount 0

# Open wave window w/o a file specified
wave_open

# Ensure we've opened 1 instance of GTKWave
assertInvokeCount 1

# Call wave_open again while an instance is already running
wave_open

# Ensure we've still only opened 1 instance of GTKWave
assertInvokeCount 1

# Ensure GTKWave hasn't been closed before
assertExitCount 0

# close gtkwave via wave_cmd and gtkwave menu commands
wave_cmd gtkwave::/File/Quit

# Ensure GTKWave has been closed
assertExitCount 1

# Ensure we've still only opened 1 instance of GTKWave
assertInvokeCount 1

# Call wave_open now that gtkwave instance is closed
wave_open

# Ensure the invoke count is now 2
assertInvokeCount 2

# close gtkwave via the menu
wave_cmd gtkwave::/File/Quit

# Ensure the invoke count is still 2
assertInvokeCount 2

# call wave_cmd to open another instance of gtkwave
wave_cmd

# Ensure the invoke count is now 3
assertInvokeCount 3

# Ensure no file has been loaded yet
assertStrCount "start time." 0

# Open a vcd file
wave_open tests/TestGui/test.vcd

# test.vcd can take a moment to load so we'll delay a bit
after 1500 set ready 1
vwait ready

# Ensure the file loaded by checking for start time. in status message
assertStrCount "start time." 1

# Reload file
wave_refresh

# test.vcd can take a moment to load so we'll delay a bit
after 1500 set ready2 1
vwait ready2

# Ensure the file loaded by checking for start time. in status message
assertStrCount "start time." 2

# Get displayed signals
wave_cmd set signals \[gtkwave::getDisplayedSignals\]\nputs \"_RETURN_\$signals\"
set signals [wave_get_return]

# Ensure no signals are displayed
assertStr $signals ""

# Add a signal to the wave
wave_show top.co_sim_sdp_nsplit_ram_1024x36_R4W4.clk

# Get displayed signals
wave_cmd set signals \[gtkwave::getDisplayedSignals\]\nputs \"_RETURN_\$signals\"
set signals [wave_get_return]

# Ensure the added signal is now displayed
assertStr $signals "top.co_sim_sdp_nsplit_ram_1024x36_R4W4.clk"

# Ensure the wave time is 0 currently
wave_cmd set time \[gtkwave::getWindowStartTime\]\nputs \"_RETURN_\$time\"
set time [wave_get_return]
assertStr $time "0"

# Change the time
wave_time 10ps

# Ensure the wave time is now 10
wave_cmd set time \[gtkwave::getWindowStartTime\]\nputs \"_RETURN_\$time\"
set time [wave_get_return]
assertStr $time "10"

# close gtkwave via the menu
wave_cmd gtkwave::/File/Quit

# Test passed
exit 0
