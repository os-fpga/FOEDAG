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


# This test has been split into two because GitHub CI fails for some reason
# even though the original test works fine locally.
# Pt 1 of the test pre-emptively creates an empty foedag_init.tcl if
# one doesn't exist
# Pt 2 Starts/stops the gui and checks the command log to ensure that
# the init file was sourced

# this ensures foedag_init.tcl exists before tcl_init_file_load_pt2.tcl gets run
set initFile "foedag_init.tcl"
# open file w/ append so we don't overwrite a real foedag_init.tcl if we find one
set fp [open $initFile a+];
close $fp;

exit 0
