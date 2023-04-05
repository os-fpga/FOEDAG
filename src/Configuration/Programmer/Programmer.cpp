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
#include <iostream>
#include <thread>

#include "CFGCommon/CFGCommon.h"
#include "CFGCompiler/CFGCompiler.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"

using namespace FOEDAG;

void programmer_entry(const CFGCommon_ARG* cmdarg, std::ostream* std_out,
                      std::ostream* std_err) {
  std::filesystem::path m_openOcdExecutablePath = "openocd";
  CFG_POST_MSG("This is Programmer entry");
  const std::string compilerName = cmdarg->compilerName;
  if (compilerName == "" || compilerName == "dummy") {
    CFG_POST_MSG("  ProjectName : %s", cmdarg->projectName.c_str());
    CFG_POST_MSG("  ProjectPath : %s", cmdarg->projectPath.c_str());
    CFG_POST_MSG("  Device      : %s", cmdarg->device.c_str());
    CFG_POST_MSG("  command     : %s", cmdarg->command.c_str());
    for (int i = 0; i < 5; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      CFG_POST_MSG("Looping to test multithread - %d", i);
    }
  } else {
    // <TODO> figure out how to get the path of openocd
    // like how it is being done in main.cpp
    if (!FileUtils::FileExists(m_openOcdExecutablePath)) {
    }
  }
  // <> code below is working
  //   auto buildCommand = [](std::string config_file,
  //                         std::string bitstream_file,
  //                         int pld_id) -> std::string {
  //   // command to invoke openocd to program the bitstream
  //   // openocd -f gemini.cfg -c "pld load 0 hello.bit"
  //   std::string openocd = "/home/guanyung/dev/FOEDAG/build/bin/openocd";
  //   return openocd + " -f " + config_file +
  //          " -c \"pld load " + std::to_string(pld_id) + " " + bitstream_file
  //          +
  //          "\"" + " -c \"exit\"";
  //   };
  // }

  std::string command =
      "/home/guanyung/dev/FOEDAG/build/bin/openocd -v -l /dev/stdout";
  // int status = ExecuteAndMonitorSystemCommand(command, cmdarg->projectPath,
  // std_out, std_err); if (status)
  // {
  //   CFG_POST_ERR("bitstream programming failed");
  // }

  // <TODO>
  CFG_execute_and_monitor_system_command(command, "", false);
}