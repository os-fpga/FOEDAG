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
#include "CompilerFactory.h"

#include "CompilerOpenFPGA.h"
#include "Main/Foedag.h"
#include "Main/ToolContext.h"

namespace FOEDAG {

Compiler *CompilerFactory::CreateCompiler(const std::string &compiler,
                                          Foedag *foedag) {
  if (compiler == "openfpga") {
    CompilerOpenFPGA *opcompiler = new CompilerOpenFPGA;
    const std::string &binpath = foedag->Context()->BinaryPath().string();
    opcompiler->setYosysExecPath(binpath + "/yosys");
    opcompiler->setVprExecPath(binpath + "/vpr");
    return opcompiler;
  } else {
    return new Compiler();
  }
}

}  // namespace FOEDAG
