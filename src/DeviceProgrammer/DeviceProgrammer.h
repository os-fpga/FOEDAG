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

#ifndef DEVICEPROGRAMMER_H
#define DEVICEPROGRAMMER_H

#include <filesystem>
#include <iostream>
#include <string>

namespace FOEDAG {

class TclInterpreter;
class Compiler;

class DeviceProgrammer {
 public:
  DeviceProgrammer(Compiler* compiler)
      : m_compiler(compiler),
        m_bitstreamFile(""),
        m_configFile(""),
        m_pldId(0) {}
  virtual ~DeviceProgrammer() {}
  Compiler* GetCompiler() { return m_compiler; }
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);

  std::filesystem::path GetBitstreamFilename() const;
  std::filesystem::path GetConfigFilename() const;
  uint16_t GetPldId() const;

 protected:
  Compiler* m_compiler = nullptr;

 private:
  std::filesystem::path m_bitstreamFile;
  std::filesystem::path m_configFile;
  uint16_t m_pldId;
};

}  // namespace FOEDAG

#endif
