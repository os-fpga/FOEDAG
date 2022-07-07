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


# opening all the settings dialg non-modally (last arg of EditSettings) so that we can continue to execute tcl commands after the dialog shows
gui_start
puts "OPEN MAIN SETTINGS DLG GENERICALLY" ; flush stdout ; EditSettings "" 1
puts "OPEN MAIN SETTINGS DLG FOR SYNTHESIS" ; flush stdout ; EditSettings "Synthesis" 1
puts "OPEN MAIN SETTINGS DLG FOR PLACEMENT" ; flush stdout ; EditSettings "Placement" 1
puts "OPEN MAIN SETTINGS DLG FOR ROUTING" ; flush stdout ; EditSettings "Routing" 1
