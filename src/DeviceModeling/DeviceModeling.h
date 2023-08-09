/**
 * @file DeviceModeling.h
 * @author Manadher Kharroubi (Manadher.Kharroubi@rapidsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

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

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "DeviceModelerInterface.h"

namespace FOEDAG {

class TclInterpreter;
class Compiler;

class DeviceModeling {
 public:
  DeviceModeling(Compiler* compiler) : m_compiler(compiler) {}
  virtual ~DeviceModeling() {}
  Compiler* GetCompiler() { return m_compiler; }
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  std::filesystem::path GetProjDir() const;
 protected:
  Compiler* m_compiler = nullptr;
};

}  // namespace FOEDAG
