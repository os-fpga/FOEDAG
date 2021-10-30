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

set errorInfo ""

catch {
gui_start ; puts "GUI START" ; flush stdout

after 500  "gui_stop; puts \"GUI STOP\"; flush stdout"
after 1000 "gui_start; puts \"GUI START\"; flush stdout"
after 1500 "gui_stop; puts \"GUI STOP\"; flush stdout"

after 2000 "puts \"GUI EXIT\" ; flush stdout; set CONT 0"

set CONT 1 
while {$CONT} {
    set a 0
    after 100 set a 1
    vwait a
}

puts "Tcl Exit" ; flush stdout
}  

if {$errorInfo != ""} {
 puts $errorInfo
 exit 1
}
tcl_exit

