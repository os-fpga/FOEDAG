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
#include <regex>
#include <sstream>
#include <thread>

#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "IPGenerate/IPCatalogBuilder.h"
#include "MainWindow/Session.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

void buildMockUpIPDef(IPCatalog* catalog) {
  std::vector<Connector*> connections;
  std::vector<Value*> parameters;
  Constant* lrange = new Constant(0);
  Parameter* rrange = new Parameter("Width", 0);
  Range range(lrange, rrange);
  Port* port = new Port("clk", Port::Direction::Input, Port::Function::Clock,
                        Port::Polarity::High, range);
  connections.push_back(port);
  IPDefinition* def =
      new IPDefinition(IPDefinition::IPType::Other, "MOCK_IP",
                       "path_to_nowhere", connections, parameters);
  catalog->addIP(def);
  // catalog->WriteCatalog(std::cout);
}

bool IPCatalogBuilder::buildLiteXCatalog(
    IPCatalog* catalog, const std::filesystem::path& litexIPgenPath) {
  bool result = true;
  buildMockUpIPDef(catalog);
  if (FileUtils::FileExists(litexIPgenPath)) {
    std::filesystem::path execPath = litexIPgenPath;
    if (!std::filesystem::is_directory(execPath)) {
      execPath = execPath.parent_path();
    }
    m_compiler->Message("IP Catalog, browsing directory for IP generator(s): " +
                        execPath.string());
    for (const std::filesystem::path& entry :
         std::filesystem::recursive_directory_iterator(
             execPath,
             std::filesystem::directory_options::follow_directory_symlink)) {
      const std::string& exec_name = entry.string();
      if (exec_name.find("__init__.py") != std::string::npos) continue;
      if (exec_name.find("_gen.py") != std::string::npos) {
        m_compiler->Message("IP Catalog, found IP compiler: " + exec_name);
        bool res = buildLiteXIPFromGenerator(catalog, entry);
        if (res == false) {
          result = false;
        }
      }
    }
  } else {
    result = false;
    m_compiler->ErrorMessage("IP Catalog, directory does not exist: " +
                             litexIPgenPath.string());
  }
  return result;
}

static std::string& rtrim(std::string& str, char c) {
  auto it1 = std::find_if(str.rbegin(), str.rend(),
                          [c](char ch) { return (ch == c); });
  if (it1 != str.rend()) str.erase(it1.base() - 1, str.end());
  return str;
}

bool IPCatalogBuilder::buildLiteXIPFromGenerator(
    IPCatalog* catalog, const std::filesystem::path& pythonConverterScript) {
  bool result = true;
  std::ostringstream help;
  std::filesystem::path python3Path = FileUtils::LocateExecFile("python3");
  std::string command = python3Path.string() + " " +
                        pythonConverterScript.string() + " --json-template";
  if (FileUtils::ExecuteSystemCommand(command, &help)) {
    m_compiler->ErrorMessage("IP Catalog, no IP information for " +
                             pythonConverterScript.string() + "\n" +
                             help.str());
    return false;
  }
  std::stringstream buffer;
  buffer << help.str();
  json jopts;
  try {
    jopts = json::parse(buffer);
  } catch (json::parse_error& e) {
    std::string msg = "Json Parse Error: " + std::string(e.what()) + "\n" +
                      "filePath: " + pythonConverterScript.string() + "\n";
    m_compiler->ErrorMessage(msg);
  }

  std::filesystem::path basepath = FileUtils::Basename(pythonConverterScript);
  std::string basename = basepath.string();
  std::string IPName = rtrim(basename, '.');
  std::vector<Value*> parameters;
  std::vector<Connector*> connections;
  for (auto& el : jopts.items()) {
    std::string key = el.key();
    if (key == "build_dir" || (key == "json") || (key == "json_template") ||
        (key == "build") || (key == "build_name")) {
      continue;
    }

    auto val = el.value();
    if (val.is_string()) {
      std::string value = el.value();
      SParameter* p = new SParameter(key, value);
      parameters.push_back(p);
    } else if (val.is_boolean()) {
      Parameter* p = new Parameter(key, val);
      parameters.push_back(p);
    } else {
      int64_t value = el.value();
      Parameter* p = new Parameter(key, value);
      parameters.push_back(p);
    }
  }
  m_compiler->Message("IP Catalog, adding IP: " + IPName);
  IPDefinition* def =
      new IPDefinition(IPDefinition::IPType::LiteXGenerator, IPName,
                       pythonConverterScript, connections, parameters);
  catalog->addIP(def);
  return result;
}
