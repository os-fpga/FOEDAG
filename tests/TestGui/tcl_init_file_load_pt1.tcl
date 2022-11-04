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

exit 0
