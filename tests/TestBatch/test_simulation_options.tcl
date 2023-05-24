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

# The following test calls each task's clean function before calling it's main action to check for an infinite loop caused by missing clean arg handling
# This is related to os-fpga/FOEDAG/issues/575

set cmd "simulation_options asd"
if {![catch {eval $cmd} result]} {
  error "TEST FAILED: '$cmd' should error out"
}

if {$result eq ""} {
  error "TEST FAILED: '$cmd' should print error message"
}

set cmd "simulation_options asd asd"
if {![catch {eval $cmd} result]} {
  error "TEST FAILED: '$cmd' should error out"
}

if {$result eq ""} {
  error "TEST FAILED: '$cmd' should print error message"
}

set cmd "simulation_options elab asd asd"
if {[catch {eval $cmd} result]} {
  error "TEST FAILED: '$cmd' should pass"
}

if {$result ne ""} {
  error "TEST FAILED: '$cmd' should print nothing but the output is \"$result\""
}
exit 0
