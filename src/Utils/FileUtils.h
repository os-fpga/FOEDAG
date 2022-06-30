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

#ifndef FOEDAG_FILEUTILS_H
#define FOEDAG_FILEUTILS_H
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace FOEDAG {

class FileUtils final {
 public:
  static bool fileExists(const std::filesystem::path& name);
  static bool fileIsRegular(const std::filesystem::path& name);
  static bool fileIsDirectory(const std::filesystem::path& name);

  static bool mkDirs(const std::filesystem::path& path);
  static bool rmDirRecursively(const std::filesystem::path& path);
  static std::filesystem::path getFullPath(const std::filesystem::path& path);
  static bool getFullPath(const std::filesystem::path& path,
                          std::filesystem::path* result);
  static std::filesystem::path getPathName(const std::filesystem::path& path);
  static std::filesystem::path basename(const std::filesystem::path& str);
  static uint64_t fileSize(const std::filesystem::path& name);
 
  static std::string getFileContent(const std::filesystem::path& name);

  static std::filesystem::path getPreferredPath(
      const std::filesystem::path& path);

  static std::filesystem::path locateExecFile(const std::filesystem::path& path);
 private:
  FileUtils() = delete;
  FileUtils(const FileUtils& orig) = delete;
  ~FileUtils() = delete;
};

}; 

#endif /* FOEDAG_FILEUTILS_H */
