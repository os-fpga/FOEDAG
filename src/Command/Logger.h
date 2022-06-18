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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef LOGGER_H
#define LOGGER_H

namespace FOEDAG {

class Logger {
 private:
 public:
  Logger(const std::string& filePath);
  void open();
  void close();
  void log(const std::string& text);
  void appendLog(const std::string& text);

  ~Logger();
  Logger& operator<<(const std::string& log);

 private:
  std::ofstream* m_stream = nullptr;
  std::string m_fileName;
};

}  // namespace FOEDAG

#endif
