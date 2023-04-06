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
#ifndef IPCATALOGBUILDER_H
#define IPCATALOGBUILDER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "IPGenerate/IPCatalog.h"

namespace FOEDAG {
class Compiler;

class IPCatalogBuilder {
 public:
  IPCatalogBuilder(Compiler* compiler) : m_compiler(compiler) {}
  bool buildLiteXCatalog(IPCatalog* catalog,
                         const std::filesystem::path& litexIPgenPath,
                         bool namesOnly = false);

  virtual ~IPCatalogBuilder() {}

  bool buildLiteXIPFromGenerator(
      IPCatalog* catalog, const std::filesystem::path& pythonConverterScript);

  bool buildLiteXIPFromJson(IPCatalog* catalog,
                            const std::filesystem::path& pythonConverterScript,
                            const std::string& jsonStr,
                            const std::string& command = std::string{});

 protected:
  bool buildLiteXIPFromGeneratorInternal(
      IPCatalog* catalog, const std::filesystem::path& pythonConverterScript);
  Compiler* m_compiler = nullptr;
};

}  // namespace FOEDAG

#endif
