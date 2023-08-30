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

#include "LogUtils.h"

#include <fstream>

#include "NewProject/ProjectManager/config.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;

extern const char* foedag_build;
extern const char* foedag_git_hash;
extern const char* foedag_build_type;
extern const char* release_version;

void LogUtils::AddHeaderToLog(const std::filesystem::path& logPath) {
  // Grab first 2 lines of copyright incase the first is a block comment
  auto lines = GetCopyrightLines(2);

  if (FileUtils::FileExists(logPath) && !HasHeader(logPath, lines)) {
    std::ifstream srcLog(logPath);

    // new file path w/ temp name
    std::filesystem::path tempPath = logPath.string() + ".NEW";
    std::ofstream destLog(tempPath);

    PrintHeader(&destLog);

    // Add original log contents
    destLog << srcLog.rdbuf();

    // Close new log and replace old
    srcLog.close();
    destLog.close();
    std::filesystem::remove(logPath);
    std::filesystem::rename(tempPath, logPath);
  }
}

// This will search a given directory (non-recursively) for files ending in the
// given extension and add a header to the log if it doesn't already have one
void LogUtils::AddHeadersToLogs(const std::filesystem::path& logDir,
                                const std::string extension /* .rpt */) {
  if (FileUtils::FileExists(logDir)) {
    // Find files in this dir that have the given extension
    for (auto file : std::filesystem::directory_iterator(logDir)) {
      if (file.path().extension() == extension) {
        AddHeaderToLog(file);
      }
    }
  }
}

static std::vector<std::string> copyrightLines{};
std::vector<std::string> LogUtils::GetCopyrightLines(int lineCount) {
  // Assume that copyright file doesn't change so we'll only read in the lines
  // once and then just store them in copyrightLines
  if (((int)copyrightLines.size()) != lineCount) {
    // Clear the cache incase a new lineCount was requested
    copyrightLines.clear();

    // Get copyright path
    std::filesystem::path datapath = Config::Instance()->dataPath();
    std::filesystem::path copyrightFile =
        datapath / std::string("etc") / std::string("copyright.txt");

    // Read and store the first X lines of the file
    if (FileUtils::FileExists(copyrightFile)) {
      std::fstream file(copyrightFile);
      for (int i = 0; i < lineCount; i++) {
        std::string line{};
        getline(file, line);
        copyrightLines.push_back(line);
      }
    }
  }

  return copyrightLines;
}

std::string LogUtils::GetLogHeader(std::string commentPrefix /* "" */,
                                   bool withLogTime /*true*/) {
  std::stringstream temp;
  PrintHeader(&temp, withLogTime);

  std::stringstream result;
  if (commentPrefix == "") {
    // If no prefix, just copy the result and return it
    result << temp.str() << "\n";
  } else {
    // If a commentPrefix was provided, add it in front of every line
    std::vector<std::string> lines{};
    StringUtils::tokenize(temp.str(), "\n", lines);

    for (auto line : lines) {
      result << commentPrefix << line << std::endl;
    }
  }

  return result.str();
}

bool LogUtils::HasHeader(const std::filesystem::path& logPath,
                         const std::vector<std::string>& firstLines) {
  // default to true so we no-op if the file doesn't exist
  bool result = true;
  if (FileUtils::FileExists(logPath)) {
    std::fstream file(logPath);
    for (auto expected : firstLines) {
      std::string line{};
      getline(file, line);
      // Check if expected and actual line contents match
      result &= (expected == line);
    }
  }

  return result;
}

void LogUtils::PrintHeader(std::ostream* out, bool printTime /*true*/) {
  // Add Copyright
  PrintCopyright(out);

  // Add version info
  PrintVersion(out);

  // Add Current Time in UTC/GMT
  if (printTime) {
    auto time =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    (*out) << "Log Time   : " << std::put_time(std::gmtime(&time), "%c %Z")
           << "\n";
  }
}

void LogUtils::PrintCopyright(std::ostream* out) {
  std::filesystem::path datapath = Config::Instance()->dataPath();
  std::filesystem::path copyrightFile =
      datapath / std::string("etc") / std::string("copyright.txt");

  // Add contents from <projectRoot>/etc/copyright.txt
  if (FileUtils::FileExists(copyrightFile)) {
    std::ifstream file(copyrightFile);
    (*out) << file.rdbuf() << "\n";
  }
}

void LogUtils::PrintVersion(std::ostream* out) {
  (*out) << "Version  : " << release_version << "\n";
  if (std::string(foedag_build) != "${VERSION_NUMBER}")
    (*out) << "Build    : " << foedag_build << "\n";
  if (std::string(foedag_git_hash) != "${GIT_HASH}")
    (*out) << "Hash     : " << foedag_git_hash << "\n";
  (*out) << "Date     : " << __DATE__ << "\n";
  (*out) << "Type     : " << foedag_build_type << "\n";
}
