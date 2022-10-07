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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "IPGenerate/IPCatalog.h"

#ifndef IPGENERATOR_H
#define IPGENERATOR_H

namespace FOEDAG {

class TclInterpreter;
class Compiler;
class IPInstance;

class IPGenerator {
 public:
  IPGenerator(IPCatalog* catalog, Compiler* compiler)
      : m_catalog(catalog), m_compiler(compiler) {}
  virtual ~IPGenerator() {}
  IPCatalog* Catalog() { return m_catalog; }
  Compiler* GetCompiler() { return m_compiler; }
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  std::vector<IPInstance*> IPInstances() { return m_instances; }
  bool AddIPInstance(IPInstance* instance);
  IPInstance* GetIPInstance(const std::string& moduleName);
  void RemoveIPInstance(IPInstance* instance);
  void RemoveIPInstance(const std::string& moduleName);
  void DeleteIPInstance(IPInstance* instance);
  void DeleteIPInstance(const std::string& moduleName);
  void ResetIPList() {
    m_instances.erase(m_instances.begin(), m_instances.end());
  }
  bool Generate();
  std::filesystem::path GetBuildDir(IPInstance* instance) const;

 protected:
  IPCatalog* m_catalog = nullptr;
  Compiler* m_compiler = nullptr;
  std::vector<IPInstance*> m_instances;
};

}  // namespace FOEDAG

#endif
