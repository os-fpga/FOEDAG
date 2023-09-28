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

#ifndef DESIGNQUERY_H
#define DESIGNQUERY_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "nlohmann_json/json.hpp"

namespace FOEDAG {

class TclInterpreter;
class Compiler;

class DesignQuery {
 public:
  DesignQuery(Compiler* compiler) : m_compiler(compiler) {}
  virtual ~DesignQuery() {}
  Compiler* GetCompiler() { return m_compiler; }
  const nlohmann::ordered_json& getHierJson() const { return m_hier_json; }
  const nlohmann::ordered_json& getPortJson() const { return m_port_json; }
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  std::filesystem::path GetProjDir() const;
  std::filesystem::path GetHierInfoPath() const;
  std::filesystem::path GetPortInfoPath() const;
  bool LoadPortInfo();
  bool LoadHierInfo();

  std::vector<std::string> GetPorts(int portType, bool& portsParsed) const;

  void SetReadSdc(bool read_sdc);

 protected:
  Compiler* m_compiler = nullptr;
  nlohmann::ordered_json m_hier_json;
  nlohmann::ordered_json m_port_json;
  bool m_parsed_portinfo = false;
  bool m_parsed_hierinfo = false;
  bool m_read_sdc{false};  // temporary solution for reading sdc
};

}  // namespace FOEDAG

#endif
