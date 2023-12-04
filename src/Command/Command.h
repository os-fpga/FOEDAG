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
#ifndef COMMAND_H
#define COMMAND_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace FOEDAG {

class Command {
 private:
 public:
  Command(const std::string& cmd, const std::string& undo = "");
  const std::string& do_cmd();
  const std::string& undo_cmd();

  ~Command();

 private:
  std::string m_do;
  std::string m_undo;
};

}  // namespace FOEDAG

#endif
