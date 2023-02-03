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

# Helper Delay function
proc delay { time } { set delayDone 0; after $time set delayDone 1; vwait delayDone; }
# Helper function load and return the contents of foedag.log
proc getLog {} {
    # Read log contents
    set fp [open "foedag.log" r]
    set file_data [read $fp]
    close $fp

    return $file_data
}
# This will search foedag.log for a given regex up to 5 times on each failure,
# it waits for 3 seconds before trying again
# NOTE: Don't print a message about searching for your regex or the that print
# might find a false positive the next time the log is searched.
proc findInLog { regex } {
    set found 0
    set tries 0
    # retry up to 5 times
    while { !$found && $tries < 5 } {
        # Check if we found our regex
        set found [regexp $regex [getLog]]
        if { !$found } {
            delay 3000
            incr tries 1
        }
    }
    return $found
}

# open gtkwave
wave_open

# close gtkwave via the menu
wave_cmd gtkwave::/File/Quit

# Check the log for GTKWave start up message
set found [findInLog "GTKWave - Interpreter id is"]
if { $found } {
    puts "Successfully launched gtkwave process";
    exit 0
} else {
    puts "FAILED to launch gtkwave process";
    exit 1
}
