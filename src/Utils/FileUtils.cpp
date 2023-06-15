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

#include "Utils/FileUtils.h"

#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <QDebug>
#include <QProcess>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "Utils/StringUtils.h"

std::vector<QProcess*> FOEDAG::FileUtils::m_processes{};

namespace FOEDAG {

bool FileUtils::FileExists(const std::filesystem::path& name) {
  std::error_code ec;
  return std::filesystem::exists(name, ec);
}

uint64_t FileUtils::FileSize(const std::filesystem::path& name) {
  std::error_code ec;
  return std::filesystem::file_size(name, ec);
}

bool FileUtils::FileIsDirectory(const std::filesystem::path& name) {
  return std::filesystem::is_directory(name);
}

bool FileUtils::FileIsRegular(const std::filesystem::path& name) {
  return std::filesystem::is_regular_file(name);
}

bool FileUtils::MkDirs(const std::filesystem::path& path) {
  // CAUTION: There is a known bug in VC compiler where a trailing
  // slash in the path will cause a false return from a call to
  // fs::create_directories.
  std::error_code err;
  std::filesystem::create_directories(path, err);
  return std::filesystem::is_directory(path);
}

bool FileUtils::RmDirRecursively(const std::filesystem::path& path) {
  static constexpr uintmax_t kErrorCondition = static_cast<std::uintmax_t>(-1);
  std::error_code err;
  return std::filesystem::remove_all(path, err) != kErrorCondition;
}

std::filesystem::path FileUtils::GetFullPath(
    const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::path fullPath = std::filesystem::canonical(path, ec);
  return ec ? path : fullPath;
}

bool FileUtils::GetFullPath(const std::filesystem::path& path,
                            std::filesystem::path* result) {
  std::error_code ec;
  std::filesystem::path fullPath = std::filesystem::canonical(path, ec);
  bool found = (!ec && FileIsRegular(fullPath));
  if (result != nullptr) {
    *result = found ? fullPath : path;
  }
  return found;
}

std::string FileUtils::GetFileContent(const std::filesystem::path& filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  std::string result;

  if (in) {
    std::error_code err;
    const size_t prealloc = std::filesystem::file_size(filename, err);
    if (err.value() == 0) result.reserve(prealloc);

    char buffer[4096];
    while (in.good() && !in.eof()) {
      in.read(buffer, sizeof(buffer));
      result.append(buffer, in.gcount());
    }
  } else {
    result = "FAILED_TO_LOAD_CONTENT";
  }
  return result;
}

std::filesystem::path FileUtils::GetPathName(
    const std::filesystem::path& path) {
  return path.has_parent_path() ? path.parent_path() : "";
}

std::filesystem::path FileUtils::Basename(const std::filesystem::path& path) {
  return path.filename();
}

std::filesystem::path FileUtils::GetPreferredPath(
    const std::filesystem::path& path) {
  return std::filesystem::path(path).make_preferred();
}

std::filesystem::path FileUtils::LocateExecFile(
    const std::filesystem::path& path) {
  std::filesystem::path result;
  char* envpath = getenv("PATH");
  char* dir = nullptr;

  for (dir = strtok(envpath, ":"); dir; dir = strtok(NULL, ":")) {
    std::filesystem::path a_path = std::string(dir) / path;
    if (FileUtils::FileExists(a_path)) {
      return a_path;
    }
  }

  for (std::filesystem::path dir :
       {"/usr/bin", "/usr/local/bin", "~/.local/bin", "./"}) {
    std::filesystem::path a_path = dir / path;
    if (FileUtils::FileExists(a_path)) {
      return a_path;
    }
  }

  return result;
}

// This recursively searches searchPath for a file that exactly matches
// filename. Partial matches and directory matches are not returned.
std::filesystem::path FileUtils::LocateFileRecursive(
    const std::filesystem::path& searchPath, const std::string filename) {
  std::filesystem::path result{};
  if (FileUtils::FileExists(searchPath)) {
    // Recursively search searchPath
    for (const std::filesystem::path& entry :
         std::filesystem::recursive_directory_iterator(
             searchPath,
             std::filesystem::directory_options::follow_directory_symlink)) {
      // If this is a file and the filename matches exactly
      if (FileIsRegular(entry) && entry.filename() == filename) {
        result = entry;
        break;
      }
    }
  }

  return result;
}

// This will search the given paths (non-recursively) for a child file.
// All matches will be returned in a vector
std::vector<std::filesystem::path> FileUtils::FindFileInDirs(
    const std::string& filename,
    const std::vector<std::filesystem::path>& searchPaths,
    bool caseInsensitive) {
  std::vector<std::filesystem::path> results{};
  for (auto path : searchPaths) {
    // Make sure search path is valid
    if (FileUtils::FileExists(path)) {
      // Iterate through files in path
      for (const std::filesystem::path& entry :
           std::filesystem::directory_iterator(
               path,
               std::filesystem::directory_options::follow_directory_symlink)) {
        // Ensure this is a file
        if (FileUtils::FileIsRegular(entry)) {
          std::string entryName = entry.filename().string();
          std::string searchName = filename;
          if (caseInsensitive) {
            // Convert both names to lowercase to ignore case
            entryName = StringUtils::toLower(entryName);
            searchName = StringUtils::toLower(searchName);
          }
          // Record the file on match
          if (entryName == searchName) {
            results.push_back(entry);
          }
        }
      }
    }
  }
  return results;
}

std::filesystem::path FileUtils::FindFileByExtension(
    const std::filesystem::path& path, const std::string& extension) {
  if (FileUtils::FileExists(path)) {
    for (const std::filesystem::path& entry :
         std::filesystem::directory_iterator(path)) {
      if (FileUtils::FileIsRegular(entry)) {
        if (StringUtils::toLower(entry.extension().string()) ==
            StringUtils::toLower(extension))
          return entry;
      }
    }
  }
  return {};
}

Return FileUtils::ExecuteSystemCommand(const std::string& command,
                                       const std::vector<std::string>& args,
                                       std::ostream* out, int timeout_ms,
                                       const std::string& workingDir,
                                       std::ostream* err, bool startDetached) {
  QProcess process;
  if (!workingDir.empty())
    process.setWorkingDirectory(QString::fromStdString(workingDir));

  std::ostream* errStream = err ? err : out;

  if (out) {
    QObject::connect(
        &process, &QProcess::readyReadStandardOutput, [out, &process]() {
          out->write(process.readAllStandardOutput(), process.bytesAvailable());
        });
  }

  if (errStream) {
    QObject::connect(&process, &QProcess::readyReadStandardError,
                     [errStream, &process]() {
                       QByteArray data = process.readAllStandardError();
                       errStream->write(data, data.size());
                     });
  }

  QString program = QString::fromStdString(command);
  QStringList args_{};
  for (const auto& ar : args) args_ << QString::fromStdString(ar);
  if (startDetached) {
    auto success = process.startDetached(program, args_);
    return {success ? 0 : -1,
            QString{"%1: Failed to start."}.arg(program).toStdString()};
  } else {
    m_processes.push_back(&process);
    process.start(program, args_);
  }

  bool finished = process.waitForFinished(timeout_ms);
  auto it = std::find(m_processes.begin(), m_processes.end(), &process);
  if (it != m_processes.end()) m_processes.erase(it);

  std::string message{};
  if (!finished) {
    message = process.errorString().toStdString();
    if (errStream) (*errStream) << message << std::endl;
  }

  auto status = process.exitStatus();
  auto exitCode = process.exitCode();
  int returnStatus =
      finished ? (status == QProcess::NormalExit) ? exitCode : -1 : -1;

  return {returnStatus, {message}};
}

time_t FileUtils::Mtime(const std::filesystem::path& path) {
  std::string cpath = path.string();
  struct stat statbuf;
  if (stat(cpath.c_str(), &statbuf) == -1) {
    return -1;
  }
  return statbuf.st_mtime;
}

bool FileUtils::IsUptoDate(const std::string& sourceFile,
                           const std::string& outputFile) {
  time_t time_output = -1;
  if (FileUtils::FileExists(outputFile)) {
    time_output = Mtime(outputFile);
    if (time_output == -1) {
      return false;
    }
  } else {
    return false;
  }

  if (FileUtils::FileExists(sourceFile)) {
    time_t time_source = Mtime(sourceFile);
    if (time_source == -1) {
      return false;
    }
    if (time_source > time_output) {
      return false;
    }
  } else {
    return false;
  }

  return true;
}

std::string FileUtils::AdjustPath(const std::string& p) {
  std::filesystem::path the_path = p;
  return AdjustPath(the_path);
}

std::string FileUtils::AdjustPath(const std::filesystem::path& p) {
  std::filesystem::path the_path = p;
  if (!the_path.is_absolute()) {
    the_path = std::filesystem::path("..") / p;
  }
  return the_path.string();
}

void FileUtils::printArgs(int argc, const char* argv[]) {
  std::string res{};
  for (int i = 0; i < argc; i++) res += std::string{argv[i]} + " ";
  qDebug() << res.c_str();
}

void FileUtils::terminateSystemCommand() {
  for (auto pr : m_processes) pr->terminate();
}

bool FileUtils::removeFile(const std::string& file) noexcept {
  const std::filesystem::path path{file};
  return removeFile(path);
}

bool FileUtils::removeFile(const std::filesystem::path& file) noexcept {
  if (!FileExists(file)) return false;
  std::error_code ec;
  std::filesystem::remove_all(file, ec);
  return ec.value() == 0;
}

}  // namespace FOEDAG
