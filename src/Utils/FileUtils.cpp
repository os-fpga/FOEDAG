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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace FOEDAG {

namespace fs = std::filesystem;

bool FileUtils::fileExists(const fs::path& name) {
  std::error_code ec;
  return fs::exists(name, ec);
}

uint64_t FileUtils::fileSize(const fs::path& name) {
  std::error_code ec;
  return fs::file_size(name, ec);
}

bool FileUtils::fileIsDirectory(const fs::path& name) {
  return fs::is_directory(name);
}

bool FileUtils::fileIsRegular(const fs::path& name) {
  return fs::is_regular_file(name);
}

bool FileUtils::mkDirs(const fs::path& path) {
  // CAUTION: There is a known bug in VC compiler where a trailing
  // slash in the path will cause a false return from a call to
  // fs::create_directories.
  std::error_code err;
  fs::create_directories(path, err);
  return fs::is_directory(path);
}

bool FileUtils::rmDirRecursively(const fs::path& path) {
  static constexpr uintmax_t kErrorCondition = static_cast<std::uintmax_t>(-1);
  std::error_code err;
  return fs::remove_all(path, err) != kErrorCondition;
}

fs::path FileUtils::getFullPath(const fs::path& path) {
  std::error_code ec;
  fs::path fullPath = fs::canonical(path, ec);
  return ec ? path : fullPath;
}

bool FileUtils::getFullPath(const fs::path& path, fs::path* result) {
  std::error_code ec;
  fs::path fullPath = fs::canonical(path, ec);
  bool found = (!ec && fileIsRegular(fullPath));
  if (result != nullptr) {
    *result = found ? fullPath : path;
  }
  return found;
}

std::string FileUtils::getFileContent(const fs::path& filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  std::string result;

  if (in) {
    std::error_code err;
    const size_t prealloc = fs::file_size(filename, err);
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

fs::path FileUtils::getPathName(const fs::path& path) {
  return path.has_parent_path() ? path.parent_path() : "";
}

fs::path FileUtils::basename(const fs::path& path) { return path.filename(); }

fs::path FileUtils::getPreferredPath(const fs::path& path) {
  return fs::path(path).make_preferred();
}

std::filesystem::path FileUtils::locateExecFile(
    const std::filesystem::path& path) {
  std::filesystem::path result;
  char* envpath = getenv("PATH");
  char* dir = nullptr;

  for (dir = strtok(envpath, ":"); dir; dir = strtok(NULL, ":")) {
    fs::path a_path = std::string(dir) / path;
    if (FileUtils::fileExists(a_path)) {
      return a_path;
    }
  }

  for (fs::path dir : {"/usr/bin", "/usr/local/bin", "~/.local/bin", "./"}) {
    fs::path a_path = std::string(dir) / path;
    if (FileUtils::fileExists(a_path)) {
      return a_path;
    }
  }

  return result;
}

}  // namespace FOEDAG
