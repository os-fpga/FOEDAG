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
    if (argc >= 2) {
      device_programmer->m_bitstreamFilename = argv[1];
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
    device_programmer->m_bitstreamFilename = argv[1];
    return TCL_OK;
  };
  interp->registerCmd("load_bitstream_file", load_bitstream_file, this, 0);

  return status;
}

std::string DeviceProgrammer::GetBitstreamFilename() const {
  return m_bitstreamFilename;
}
