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

#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace FOEDAG {

class LogUtils final {
 public:
  static void AddHeaderToLog(const std::filesystem::path& logPath);
  static void AddHeadersToLogs(const std::filesystem::path& logPath,
                               const std::string extension = ".rpt");
  static std::vector<std::string> GetCopyrightLines(int lineCount);
  static std::string GetLogHeader(std::string commentPrefix = "",
                                  bool withLogTime = true);
  static bool HasHeader(const std::filesystem::path& logPath,
                        const std::vector<std::string>& firstLines);
  static void PrintHeader(std::ostream* out, bool printTime = true);
  static void PrintCopyright(std::ostream* out);
  static void PrintVersion(std::ostream* out);

 private:
  LogUtils() = delete;
  LogUtils(const LogUtils& orig) = delete;
  ~LogUtils() = delete;
};

};  // namespace FOEDAG
