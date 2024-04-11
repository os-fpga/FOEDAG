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

source tests/TestGui/userInteraction.tcl

set customDevice testDevice
set home_dir [file normalize ~]
set folder [file join $home_dir ".foedag/layouts/"]
set testFile [file join $folder "$customDevice.xml"]
set custom_dev [file join $home_dir ".foedag/custom_device.xml"]

puts "GUI START" ; flush stdout ; newproject_gui_open
puts "NEXT" ; flush stdout ; next
puts "NEXT" ; flush stdout ; next
puts "NEXT" ; flush stdout ; next
puts "NEXT" ; flush stdout ; next
puts "NEXT" ; flush stdout ; next
puts "Create device" ; flush stdout ; createdevice

sendData lineEditName $customDevice
sendData spinBoxWidth 7
sendData spinBoxHeight 6
sendData lineEditDsp 1,2
sendData lineEditBram 2,3
sendData CustomLayoutOk 1

if {![file exists $testFile]} {error "$testFile does not exists" }

puts "Edit device" ; flush stdout ; editdevice

set lineEditName [qt_getWidget lineEditName]
set modifyDeviceName [qt_getWidgetData $lineEditName]
EXPECT_EQ $customDevice $modifyDeviceName

set spinBoxWidth [qt_getWidget spinBoxWidth]
set width [qt_getWidgetData $spinBoxWidth]
EXPECT_EQ $width 7

set spinBoxHeight [qt_getWidget spinBoxHeight]
set height [qt_getWidgetData $spinBoxHeight]
EXPECT_EQ $height 6

set lineEditDsp [qt_getWidget lineEditDsp]
set dsp [qt_getWidgetData $lineEditDsp]
EXPECT_EQ $dsp 1,2

set lineEditBram [qt_getWidget lineEditBram]
set bram [qt_getWidgetData $lineEditBram]
EXPECT_EQ $bram 2,3

# cleanup
file delete $testFile
file delete $custom_dev
puts "GUI STOP"  ; flush stdout ; newproject_gui_close
