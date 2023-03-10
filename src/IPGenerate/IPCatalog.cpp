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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QProcess>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/Log.h"
#include "Compiler/WorkerThread.h"
#include "IPGenerate/IPCatalog.h"
#include "MainWindow/Session.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

std::string Constant::m_name;

bool IPCatalog::addIP(IPDefinition* def) {
  if (m_definitionMap.find(def->Name()) == m_definitionMap.end()) {
    m_definitionMap.emplace(def->Name(), def);
    m_definitions.push_back(def);
    return true;
  } else {
    return false;
  }
}

IPDefinition* IPCatalog::Definition(const std::string& name) {
  std::map<std::string, IPDefinition*>::iterator itr =
      m_definitionMap.find(name);
  if (itr == m_definitionMap.end()) {
    return nullptr;
  } else {
    return (*itr).second;
  }
}

void IPCatalog::WriteCatalog(std::ostream& out) {
  for (auto def : m_definitions) {
    out << "IP Name: " << def->Name() << std::endl;
  }
}

// This takes a path to an IP and parses the vendor, library, name, and version
// info from the parent directories. This assumes that the directory format is
// Vendor/Library/Name/Version
VLNV FOEDAG::getIpInfoFromPath(std::filesystem::path path) {
  std::string vendor, library, name, version;

  std::string separator =
      std::string(1, std::filesystem::path::preferred_separator);

  // Specify our container variables in the order we will read into them
  // Note we will read from the back of the path first
  std::vector<std::string*> values = {&version, &name, &library, &vendor};

  // split the path into tokens
  std::vector<std::string> tokens;
  StringUtils::tokenize(path.string(), separator, tokens);

  // Take off the child node if it is a python file
  if (StringUtils::endsWith(tokens.back(), ".py")) {
    tokens.pop_back();
  }

  // Read each path section into a variable, starting from the back of the path
  for (std::string* value : values) {
    if (!tokens.empty()) {
      *value = tokens.back();
      tokens.pop_back();
    }
  }

  return VLNV{vendor, library, name, version};
}

// This will return a path to the litex enabled python interpreter used by the
// ip catalog. This will cache the return value the first time it's not empty
std::filesystem::path IPCatalog::getPythonPath() {
  static std::filesystem::path s_pythonPath{};
  if (s_pythonPath.empty()) {
    std::filesystem::path searchPath =
        GlobalSession->Context()->DataPath() / "../envs/litex";
    searchPath = FileUtils::GetFullPath(searchPath);
    s_pythonPath = FileUtils::LocateFileRecursive(searchPath, "python");
  }
  return s_pythonPath;
}
