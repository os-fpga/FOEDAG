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

namespace FOEDAG {

class TclInterpreter;
class Compiler;

class DesignQuery {
 public:
  DesignQuery(Compiler* compiler)
      : m_compiler(compiler) {}
  virtual ~DesignQuery() {}
  Compiler* GetCompiler() { return m_compiler; }
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);

 protected:
  Compiler* m_compiler = nullptr;
};

}  // namespace FOEDAG

#endif
