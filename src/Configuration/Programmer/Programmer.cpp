/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Programmer.h"

#include <chrono>
#include <thread>

#include "CFGCommon/CFGCommon.h"

void programmer_entry(const CFGCommon_ARG* cmdarg) {
  CFG_POST_MSG("This is Programmer entry");
  CFG_POST_MSG("  ProjectName : %s", cmdarg->projectName.c_str());
  CFG_POST_MSG("  ProjectPath : %s", cmdarg->projectPath.c_str());
  CFG_POST_MSG("  Device      : %s", cmdarg->device.c_str());
  for (int i = 0; i < 20; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    CFG_POST_MSG("Looping to test multithread - %d", i);
  }
}