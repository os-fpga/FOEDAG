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
#include <memory>
#include <thread>

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

void programmer_entry(const CFGCommon_ARG* cmdarg) {
  if (cmdarg->compilerName == "dummy") {
    CFG_POST_MSG("  ProjectName : %s", cmdarg->projectName.c_str());
    CFG_POST_MSG("  ProjectPath : %s", cmdarg->projectPath.c_str());
    CFG_POST_MSG("  Device      : %s", cmdarg->device.c_str());
    CFG_POST_MSG("  command     : %s", cmdarg->command.c_str());
    for (int i = 0; i < 5; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      CFG_POST_MSG("Looping to test multithread - %d", i);
    }
  } else {
    const std::filesystem::path openOcdExecPath = cmdarg->toolPath;
    const std::filesystem::path configFileSearchPath = cmdarg->searchPath;

    auto buildCommand = [&openOcdExecPath](const std::string& config_file,
                                           const std::string& bitstream_file,
                                           int pld_id) -> std::string {
      std::string openocd = openOcdExecPath.string();
      return openocd + " -f " + config_file + " -c \"pld load " +
             std::to_string(pld_id) + " " + bitstream_file + "\"" +
             " -l /dev/stdout -c exit";
    };
    std::error_code ec;
    if (!std::filesystem::exists(openOcdExecPath, ec)) {
      CFG_POST_ERR("Cannot find openocd executable: %s. %s",
                   openOcdExecPath.string().c_str(),
                   (ec ? ec.message().c_str() : ""));
      return;
    }

    auto arg = std::static_pointer_cast<CFGArg_PROGRAM_DEVICE>(cmdarg->arg);

    if (!std::filesystem::exists(arg->bitstream, ec)) {
      CFG_POST_ERR("Cannot find bitstream file: %s. %s", arg->bitstream.c_str(),
                   (ec ? ec.message().c_str() : ""));
      return;
    }

    auto configFile = CFG_find_file(arg->config, configFileSearchPath);
    if (configFile.empty()) {
      CFG_POST_ERR("Cannot find config file: %s", configFile.c_str());
      return;
    }

    std::string command =
        buildCommand(configFile.string(), arg->bitstream, arg->index);

    int return_code = CFG_compiler_execute_cmd(command, "", false);
    if (return_code) {
      CFG_POST_ERR("Bitstream programming failed. Error code: %d", return_code);
    }
  }
}