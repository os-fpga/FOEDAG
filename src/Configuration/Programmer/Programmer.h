/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

static const std::vector<std::string> programmer_subcmd{
    "fpga_config", "fpga_status", "flash", "list_devices"};

struct ProgrammerCommand {
  std::string name;
  std::string executable_cmd;
  bool is_error = false;
};

std::string buildFpgaProgramCommand(const std::string& bitstream_file,
                                    const std::string& config_file,
                                    int pld_index);
std::string buildFpgaQueryStatusCommand(const std::string config_file,
                                        int pld_index);
std::string buildListDeviceCommand(const std::string config_file);
std::string buildFlashProgramCommand(const std::string& bitstream_file,
                                     const std::string& config_file,
                                     int pld_index, bool doErase,
                                     bool doBlankCheck, bool doProgram,
                                     bool doVerify);
ProgrammerCommand parseProgrammerCommand(const CFGCommon_ARG* cmdarg,
                                         std::filesystem::path configFile);
std::vector<std::string> parseOperationString(const std::string& operation);
bool isOperationRequested(const std::string& operation,
                          const std::vector<std::string>& supportedOperations);
void programmer_entry(const CFGCommon_ARG* cmdarg);

#endif
