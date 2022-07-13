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
#include <queue>
#include <sstream>
#include <thread>

#include "Compiler/Log.h"
#include "Compiler/ProcessUtils.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "IPGenerate/IPCatalog.h"
#include "IPGenerate/IPGenerator.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

bool IPGenerator::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  auto configure_ip = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    bool ok = true;
    IPGenerator* generator = (IPGenerator*)clientData;
    Compiler* compiler = generator->GetCompiler();
    if (!compiler->ProjManager()->HasDesign()) {
      compiler->ErrorMessage("Create a design first: create_design <name>");
      return TCL_ERROR;
    }
    if (argc < 6) {
      compiler->ErrorMessage(
          "Incorrect syntax for configure_ip <IP_NAME> -mod_name <name> "
          "-out_file <filename> -version <ver_name> -P<param>=\"<value>\"...");
      return TCL_ERROR;
    }
    std::string ip_name;
    std::string mod_name;
    std::string out_file;
    std::string version;
    std::vector<Parameter> parameters;
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (i == 1) {
        ip_name = arg;
      } else if (arg == "-mod_name") {
        i++;
        mod_name = argv[i];
      } else if (arg == "-out_file") {
        i++;
        out_file = argv[i];
      } else if (arg == "-version") {
        i++;
        version = argv[i];
      } else if (arg.find("-P") == 0) {
        std::string def;
        std::string value;
        const size_t loc = arg.find('=');
        if (loc == std::string::npos) {
          def = arg.substr(2);
        } else {
          def = arg.substr(2, loc - 2);
          value = arg.substr(loc + 1);
        }
        if (!def.empty()) {
          Parameter param(def, std::strtoul(value.c_str(), nullptr, 10));
          parameters.push_back(param);
        }
      }
    }
    IPDefinition* def = generator->Catalog()->Definition(ip_name);
    if (def == nullptr) {
      compiler->ErrorMessage("Unknown IP: " + ip_name);
      ok = false;
      return TCL_ERROR;
    }
    IPInstance* instance =
        new IPInstance(ip_name, version, def, parameters, mod_name, out_file);
    if (!generator->AddIPInstance(instance)) {
      ok = false;
    }
    return ok ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("configure_ip", configure_ip, this, 0);
  return true;
}

bool IPGenerator::AddIPInstance(IPInstance* instance) {
  bool status = true;
  const IPDefinition* def = instance->Definition();

  // Check parameters
  std::set<std::string> legalParams;

  for (Value* param : def->Parameters()) {
    legalParams.insert(param->Name());
  }
  std::queue<const Connector*> connectors;
  for (const Connector* conn : def->Connections()) {
    connectors.push(conn);
  }
  while (connectors.size()) {
    const Connector* conn = connectors.back();
    connectors.pop();
    switch (conn->GetType()) {
      case Connector::Type::Port: {
        Port* port = (Port*)conn;
        const Range& range = port->GetRange();
        for (const Value* val : {range.LRange(), range.RRange()}) {
          switch (val->GetType()) {
            case Value::Type::ParamInt: {
              Parameter* param = (Parameter*)val;
              legalParams.insert(param->Name());
              break;
            }
            case Value::Type::ParamString: {
              SParameter* param = (SParameter*)val;
              legalParams.insert(param->Name());
              break;
            }
            case Value::Type::ConstInt: {
              break;
            }
          }
        }
        break;
      }
      case Connector::Type::Interface: {
        Interface* intf = (Interface*)conn;
        for (Connector* sub : intf->Connections()) {
          connectors.push(sub);
        }
      }
    }
  }

  for (const Parameter& param : instance->Parameters()) {
    if (legalParams.find(param.Name()) == legalParams.end()) {
      GetCompiler()->ErrorMessage("Unknown parameter: " + param.Name());
      status = false;
    }
  }
  m_instances.push_back(instance);
  return status;
}

bool IPGenerator::Generate() {
  bool status = true;
  Compiler* compiler = GetCompiler();
  for (IPInstance* inst : m_instances) {
    // Create output directory
    const std::filesystem::path& out_path = inst->OutputFile();

    std::filesystem::path expandedFile = out_path;
    bool use_orig_path = false;
    if (compiler->FileExists(expandedFile)) {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath = fullPath / out_path;
      expandedFile = fullPath.string();
    }
    std::filesystem::path the_path = expandedFile;
    if (!the_path.is_absolute()) {
      expandedFile =
          std::filesystem::path(compiler->ProjManager()->projectPath() /
                                std::filesystem::path("..") / expandedFile);
    }
    std::string p = expandedFile.string();
    if (!std::filesystem::exists(expandedFile)) {
      std::filesystem::create_directories(expandedFile.parent_path());
    }

    const IPDefinition* def = inst->Definition();
    switch (def->Type()) {
      case IPDefinition::IPType::Other: {
        break;
      }
      case IPDefinition::IPType::LiteXGenerator: {
        std::string project = compiler->ProjManager()->projectPath();
        const std::filesystem::path executable = def->FilePath();
        std::string ip_config_file =
            def->Name() + "_" + inst->ModuleName() + ".json";
        std::filesystem::path jasonfile =
            std::filesystem::path(project) / ip_config_file;
        std::stringstream previousbuffer;
        if (FileUtils::fileExists(jasonfile)) {
          std::ifstream previous(jasonfile);
          std::stringstream buffer;
          previousbuffer << previous.rdbuf();
        }

        std::ofstream jsonF(jasonfile);
        jsonF << "{" << std::endl;
        for (auto param : inst->Parameters()) {
          std::string value;
          switch (param.GetType()) {
            case Value::Type::ParamString:
              value = param.GetSValue();
              break;
            case Value::Type::ParamInt:
              value = std::to_string(param.GetValue());
              break;
            case Value::Type::ConstInt:
              value = std::to_string(param.GetValue());
          }
          jsonF << "   \"" << param.Name() << "\": " << value << ","
                << std::endl;
        }
        jsonF << "   \"build_dir\": " << inst->OutputFile().parent_path() << ","
              << std::endl;
        jsonF << "   \"build_name\": " << inst->OutputFile().filename() << ","
              << std::endl;
        jsonF << "   \"build\": true," << std::endl;
        jsonF << "   \"json\": \"" << ip_config_file << "\"," << std::endl;
        jsonF << "   \"json_template\": false" << std::endl;
        jsonF << "}" << std::endl;
        jsonF.close();
        std::stringstream newbuffer;
        if (FileUtils::fileExists(jasonfile)) {
          std::ifstream newfile(jasonfile);
          std::stringstream buffer;
          newbuffer << newfile.rdbuf();
        }
        std::filesystem::path python3Path =
            FileUtils::locateExecFile("python3");
        std::string command = python3Path.string() + " " + executable.string() +
                              " --build --json " + jasonfile.string();
        std::ostringstream help;
        if (newbuffer.str() == previousbuffer.str()) {
          m_compiler->Message("IP Generate, reusing IP " +
                              inst->OutputFile().string());
          continue;
        }
        m_compiler->Message("IP Generate, generating IP " +
                            inst->OutputFile().string());
        if (FileUtils::ExecuteSystemCommand(command, &help)) {
          m_compiler->ErrorMessage("IP Generate, " + help.str());
          return false;
        }
        break;
      }
    }
  }
  return status;
}
