#Copyright 2021 The Foedag team

#GPL License

#Copyright (c) 2021 The Open-Source FPGA Foundation

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

puts "CONSOLE GUI: console_pwd"       ; flush stdout ; console_pwd
puts "CONSOLE GUI: console_proc"      ; flush stdout ; console_proc tests/TestGui/gui_console_proc.tcl "\# source tests/TestGui/gui_console_proc.tcl\nHello world1\nHello world2\n\# "
puts "CONSOLE GUI: console_multiline" ; flush stdout ; console_multiline
puts "CONSOLE GUI: console_cancel"    ; flush stdout ; console_cancel
puts "CONSOLE GUI: console_history"   ; flush stdout ; console_history
