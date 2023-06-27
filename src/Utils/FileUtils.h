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

class QProcess;

namespace FOEDAG {

struct Return {
  int code{};
  std::string message{};
};

class FileUtils final {
 public:
  static bool FileExists(const std::filesystem::path& name);
  static bool FileIsRegular(const std::filesystem::path& name);
  static bool FileIsDirectory(const std::filesystem::path& name);

  static bool MkDirs(const std::filesystem::path& path);
  static bool RmDirRecursively(const std::filesystem::path& path);
  static std::filesystem::path GetFullPath(const std::filesystem::path& path);
  static bool GetFullPath(const std::filesystem::path& path,
                          std::filesystem::path* result);
  static std::filesystem::path GetPathName(const std::filesystem::path& path);
  static std::filesystem::path Basename(const std::filesystem::path& str);
  static uint64_t FileSize(const std::filesystem::path& name);

  static std::string GetFileContent(const std::filesystem::path& name);
  static void WriteToFile(const std::filesystem::path& path,
                          const std::string& content, bool newLine = true);

  static std::filesystem::path GetPreferredPath(
      const std::filesystem::path& path);

  static std::filesystem::path LocateExecFile(
      const std::filesystem::path& path);

  static std::filesystem::path LocateFileRecursive(
      const std::filesystem::path& searchPath, const std::string filename);

  static std::vector<std::filesystem::path> FindFileInDirs(
      const std::string& filename,
      const std::vector<std::filesystem::path>& searchPaths,
      bool caseInsensitive);

  static std::filesystem::path FindFileByExtension(
      const std::filesystem::path& path, const std::string& extension);

  static Return ExecuteSystemCommand(const std::string& command,
                                     const std::vector<std::string>& args,
                                     std::ostream* out, int timeout_ms = -1,
                                     const std::string& workingDir = {},
                                     std::ostream* err = nullptr,
                                     bool startDetached = false);

  static time_t Mtime(const std::filesystem::path& path);

  static bool IsUptoDate(const std::string& sourceFile,
                         const std::string& outputFile);

  static std::string AdjustPath(const std::string& p, const std::string& base);
  static std::string AdjustPath(const std::filesystem::path& p,
                                const std::filesystem::path& base);

  // return true if file was removed otherwise return false
  static bool removeFile(const std::string& file) noexcept;
  static bool removeFile(const std::filesystem::path& file) noexcept;
  static bool removeAll(const std::filesystem::path& path);

  // for the debug purposes, this function prints arguments
  static void printArgs(int argc, const char* argv[]);

  static void terminateSystemCommand();

 private:
  FileUtils() = delete;
  FileUtils(const FileUtils& orig) = delete;
  ~FileUtils() = delete;

  static std::vector<QProcess*> m_processes;
};

};  // namespace FOEDAG

#endif /* FOEDAG_FILEUTILS_H */
