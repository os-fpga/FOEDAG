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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Main/CommandLine.h"
#include "MainWindow/Session.h"
#include "TaskManager.h"
#include "Tcl/TclInterpreter.h"

namespace FOEDAG {

/* This class preprocess SDC contraints to keep all names used in the
 * constraints and the constraints themselves */

class Constraints {
 public:
  Constraints();
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() { return m_session; }
  ~Constraints();
  bool evaluateConstraints(const std::filesystem::path& path);
  bool evaluateConstraint(const std::string& constraint);
  void reset();
  const std::vector<std::string>& getConstraints() { return m_constraints; }
  const std::set<std::string>& GetKeeps() { return m_keeps; }
  void registerCommands(TclInterpreter* interp);
  void addKeep(const std::string& name) { m_keeps.insert(name); }
  void addConstraint(const std::string& name) { m_constraints.push_back(name); }

 protected:
  std::ostream* m_out = &std::cout;
  TclInterpreter* m_interp = nullptr;
  Session* m_session = nullptr;
  std::vector<std::string> m_constraints;
  std::set<std::string> m_keeps;
};

}  // namespace FOEDAG

#endif
