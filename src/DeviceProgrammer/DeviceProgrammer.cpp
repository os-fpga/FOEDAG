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
    ProjectManager* projManager = compiler->ProjManager();
    std::string projectName{"noname"};
    std::string activeTargetDevice = projManager->getTargetDevice();
    if (projManager->HasDesign()) {
      projectName = projManager->projectName();
    }
    constexpr int step{10};
    constexpr int totalProgress{100};
    for (int i = 0; i <= totalProgress; i = i + step) {
      std::stringstream outStr;
      outStr << std::setw(3) << i << "% [";
      std::string s1(i / 10, '=');
      outStr << s1 << ">" << std::setw(step + 1 - i / (step)) << "]";
      outStr << " just for test";
      compiler->Message(outStr.str());
      std::this_thread::sleep_for(100ms);
    };
    compiler->Message(projectName + " " + activeTargetDevice +
                      " Bitstream is programmed");
    return 0;
  };
  interp->registerCmd("program_device", program_device, this, 0);

  auto load_bitstream_file = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;
    DeviceProgrammer* device_programmer = (DeviceProgrammer*)clientData;
    Compiler* compiler = device_programmer->GetCompiler();
    compiler->Message("Pending Implementation:: load_bitstream_file");
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("load_bitstream_file", load_bitstream_file, this, 0);

  auto verify = [](void* clientData, Tcl_Interp* interp, int argc,
                   const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;
    DeviceProgrammer* device_programmer = (DeviceProgrammer*)clientData;
    Compiler* compiler = device_programmer->GetCompiler();
    compiler->Message("Pending Implementation:: verify");
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("verify", verify, this, 0);
  return status;
}
