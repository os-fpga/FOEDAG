/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

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
#include "DeviceProgrammer/DeviceProgrammer.h"

#include "Compiler/Compiler.h"
#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "NewProject/ProjectManager/project_manager.h"

using namespace FOEDAG;
using namespace std::literals;

bool DeviceProgrammer::RegisterCommands(TclInterpreter* interp,
                                        bool batchMode) {
  bool status{true};

  auto program_device = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceProgrammer* device_programmer = (DeviceProgrammer*)clientData;
    Compiler* compiler = device_programmer->GetCompiler();
    constexpr bool append = false;
    const std::string prefix{"PDV: "};
    if (argc == 2) {
      // just take the first argument as the bitstream file
      // the rest of the arguments use default values
      device_programmer->m_bitstreamFile = argv[1];
    } else if (argc == 7 || argc == 5) {
      // user specified all the arguments
      // -c <config_file> -b <bitstream_file> -n <pld_id>
      for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-c" && i + 1 < argc) {
          device_programmer->m_configFile = argv[++i];
        } else if (std::string(argv[i]) == "-n" && i + 1 < argc) {
          try {
            int idx = std::stoi(argv[++i]);
            if (idx >= 0) {
              device_programmer->m_pldId = idx;
            } else {
              compiler->ErrorMessage("Invalid argument for -n", append, prefix);
              return TCL_ERROR;
            }
          } catch (const std::invalid_argument& e) {
            compiler->ErrorMessage("Invalid argument for -n", append, prefix);
            return TCL_ERROR;
          } catch (const std::out_of_range& e) {
            compiler->ErrorMessage("Out of range argument for -n", append,
                                   prefix);
            return TCL_ERROR;
          }
        } else if (argc != 5 && std::string(argv[i]) == "-b" && i + 1 < argc) {
          device_programmer->m_bitstreamFile = argv[++i];
        }
      }
    } else {
      compiler->ErrorMessage(
          "Please specify all the arguments for program_device\n"
          "program_device -c <config_file> -b <bitstream_file> -n <pld_id>",
          append, prefix);
      return TCL_ERROR;
    }

    bool status = compiler->Compile(Compiler::Action::ProgramDevice);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("program_device", program_device, this, 0);

  auto load_bitstream_file = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceProgrammer* device_programmer = (DeviceProgrammer*)clientData;
    Compiler* compiler = device_programmer->GetCompiler();
    if (argc != 2) {
      compiler->ErrorMessage("Please specify the bitstream file");
      return TCL_ERROR;
    }
    device_programmer->m_bitstreamFile = argv[1];
    return TCL_OK;
  };
  interp->registerCmd("load_bitstream_file", load_bitstream_file, this, 0);

  return status;
}

std::filesystem::path DeviceProgrammer::GetBitstreamFilename() const {
  return m_bitstreamFile;
}

std::filesystem::path DeviceProgrammer::GetConfigFilename() const {
  return m_configFile;
}

uint16_t DeviceProgrammer::GetPldId() const { return m_pldId; }