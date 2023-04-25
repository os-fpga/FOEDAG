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

#include "Compiler/Compiler.h"
#include "Compiler/Log.h"
#include "Compiler/WorkerThread.h"
#include "IPGenerate/IPCatalog.h"
#include "IPGenerate/IPGenerator.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

bool IPGenerator::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  auto add_litex_ip_catalog = [](void* clientData, Tcl_Interp* interp, int argc,
                                 const char* argv[]) -> int {
    IPGenerator* generator = (IPGenerator*)clientData;
    Compiler* compiler = generator->GetCompiler();
    if (argc < 2) {
      compiler->ErrorMessage(
          "Missing directory path for LiteX ip generator(s)");
    }
    const std::filesystem::path file = argv[1];
    std::filesystem::path expandedFile = file;
    bool use_orig_path = false;
    if (FileUtils::FileExists(expandedFile) && expandedFile != "./") {
      use_orig_path = true;
    }

    if ((!use_orig_path) &&
        (!compiler->GetSession()->CmdLine()->Script().empty())) {
      std::filesystem::path script =
          compiler->GetSession()->CmdLine()->Script();
      std::filesystem::path scriptPath = script.parent_path();
      std::filesystem::path fullPath = scriptPath;
      fullPath = fullPath / file;
      expandedFile = fullPath;
    }
    std::filesystem::path the_path = expandedFile;
    if (!the_path.is_absolute()) {
      const auto& path = std::filesystem::current_path();
      expandedFile = path / expandedFile;
    }
    auto fn = [compiler, expandedFile]() -> bool {
      return compiler->BuildLiteXIPCatalog(expandedFile.lexically_normal());
    };
    WorkerThread* thread = new WorkerThread{
        {}, Compiler::Action::NoAction, generator->GetCompiler()};
    bool res = thread->Start(fn);
    return res ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("add_litex_ip_catalog", add_litex_ip_catalog, this, 0);

  auto ip_catalog = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    IPGenerator* generator = (IPGenerator*)clientData;
    Compiler* compiler = generator->GetCompiler();

    // Load IPs if no definitions are available
    if (!compiler->HasIPDefinitions()) {
      std::filesystem::path path =
          GlobalSession->Context()->DataPath() / "IP_Catalog";
      compiler->TclInterp()->evalCmd("add_litex_ip_catalog {" +
                                     path.lexically_normal().string() + "}");
    }

    bool status = true;
    if (argc == 1) {
      // List all IPs
      std::string ips;
      for (auto def : generator->Catalog()->Definitions()) {
        ips += def->Name() + " ";
      }
      compiler->TclInterp()->setResult(ips);
    } else if (argc == 2) {
      std::string ip_def;
      for (auto def : generator->Catalog()->Definitions()) {
        if (argv[1] == def->Name()) {
          for (auto param : def->Parameters()) {
            std::string defaultValue;
            switch (param->GetType()) {
              case Value::Type::ParamIpVal:
                defaultValue = param->GetSValue();
                break;
              case Value::Type::ParamInt:
                defaultValue = param->GetSValue();
                break;
              case Value::Type::ParamString:
                defaultValue = param->GetSValue();
                break;
              case Value::Type::ConstInt:
                defaultValue = param->GetSValue();
                break;
            }
            ip_def += "{" + param->Name() + " " + defaultValue + "} ";
          }
        }
      }
      compiler->TclInterp()->setResult(ip_def);
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("ip_catalog", ip_catalog, this, 0);

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

    // Load IPs if no definitions are available
    if (!compiler->HasIPDefinitions()) {
      std::filesystem::path path =
          GlobalSession->Context()->DataPath() / "IP_Catalog";
      compiler->TclInterp()->evalCmd("add_litex_ip_catalog {" +
                                     path.lexically_normal().string() + "}");
    }

    std::string ip_name;
    std::string mod_name;
    std::string out_file;
    std::string version;
    std::vector<SParameter> parameters;
    bool generated{true};
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
      } else if (arg == "-template") {
        generated = false;
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
          SParameter param(def, value);
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
    instance->Generated(generated);
    if (!generator->AddIPInstance(instance)) {
      ok = false;
    }
    return ok ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("configure_ip", configure_ip, this, 0);

  auto remove_ip = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    IPGenerator* generator = (IPGenerator*)clientData;
    if (argc > 1) {
      generator->RemoveIPInstance(argv[1]);
    }

    return TCL_OK;
  };
  interp->registerCmd("remove_ip", remove_ip, this, 0);

  auto delete_ip = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    IPGenerator* generator = (IPGenerator*)clientData;
    if (argc > 1) {
      generator->DeleteIPInstance(argv[1]);
    }

    return TCL_OK;
  };
  interp->registerCmd("delete_ip", delete_ip, this, 0);

  auto simulate_ip = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    IPGenerator* generator = (IPGenerator*)clientData;
    if (argc < 2) {
      Tcl_AppendResult(interp, "Wrong arguments. Please read simulate_ip help",
                       nullptr);
      return TCL_ERROR;
    }
    const std::string ipName{argv[1]};
    WorkerThread* thread = new WorkerThread{
        {}, Compiler::Action::Analyze, generator->GetCompiler()};
    std::string message{};
    auto fn = [generator, &message](const std::string& ipName) -> bool {
      auto [ok, res] = generator->SimulateIpTcl(ipName);
      if (!ok) message = res;
      return ok;
    };
    bool res = thread->Start(fn, ipName);
    if (!res) Tcl_AppendResult(interp, message.c_str(), nullptr);
    return res ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("simulate_ip", simulate_ip, this, nullptr);

  return true;
}

bool IPGenerator::AddIPInstance(IPInstance* instance) {
  bool status = true;
  const IPDefinition* def = instance->Definition();

  // Remove old IP Instance if an instance with the same ModuleName is passed
  auto isMatch = [instance](IPInstance* targetInstance) {
    return targetInstance->ModuleName() == instance->ModuleName();
  };
  auto it = std::find_if(m_instances.begin(), m_instances.end(), isMatch);
  if (it != m_instances.end()) {
    m_instances.erase(it);
  }

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
            case Value::Type::ParamIpVal: {
              IPParameter* param = (IPParameter*)val;
              legalParams.insert(param->Name());
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

  for (const SParameter& param : instance->Parameters()) {
    if (legalParams.find(param.Name()) == legalParams.end()) {
      GetCompiler()->ErrorMessage("Unknown parameter: " + param.Name());
      status = false;
    }
  }
  m_instances.push_back(instance);
  return status;
}

IPInstance* IPGenerator::GetIPInstance(const std::string& moduleName) {
  IPInstance* retVal{};
  // Search instances based off moduleName
  auto isMatch = [moduleName](IPInstance* instance) {
    return instance->ModuleName() == moduleName;
  };
  auto it = std::find_if(m_instances.begin(), m_instances.end(), isMatch);

  // return result
  if (it != m_instances.end()) {
    retVal = *it;
  }

  return retVal;
}

FOEDAG::Value* IPGenerator::GetCatalogParam(IPInstance* instance,
                                            const std::string& paramName) {
  // Searches a given instance's definition parameters which contain additional
  // meta data stored during catalog generation
  FOEDAG::Value* retVal{};

  // Search based off parameter name
  auto isMatch = [paramName](FOEDAG::Value* param) {
    return param->Name() == paramName;
  };
  if (instance && instance->Definition()) {
    auto params = instance->Definition()->Parameters();
    auto it = std::find_if(params.begin(), params.end(), isMatch);

    // return result
    if (it != params.end()) {
      retVal = *it;
    }
  }

  return retVal;
}

void IPGenerator::RemoveIPInstance(IPInstance* instance) {
  auto it = std::find(m_instances.begin(), m_instances.end(), instance);
  if (it != m_instances.end()) {
    m_instances.erase(it);
  }

  // search for stored configure/generate commands stored for this module and
  // remove them
  Compiler* compiler = GetCompiler();
  ProjectManager* projManager = nullptr;
  if (compiler && (projManager = compiler->ProjManager())) {
    // match if the command contains "ipgenerate -modules <moduleName>"
    std::string modName = "ipgenerate -modules " + instance->ModuleName();
    auto isMatch = [modName](const std::string& ipGenStr) {
      return (ipGenStr.find(modName) == std::string::npos);
    };

    // Remove any found matches
    auto cmds = projManager->ipInstanceCmdList();
    cmds.erase(std::remove_if(cmds.begin(), cmds.end(), isMatch), cmds.end());
    // Store the updated instance list
    projManager->setIpInstanceCmdList(cmds);
  }
}

void IPGenerator::RemoveIPInstance(const std::string& moduleName) {
  RemoveIPInstance(GetIPInstance(moduleName));
}

void IPGenerator::DeleteIPInstance(IPInstance* instance) {
  // Delete the build folder if it exists
  auto buildPath = GetBuildDir(instance);
  if (FileUtils::FileExists(buildPath) &&
      FileUtils::FileIsDirectory(buildPath)) {
    std::filesystem::remove_all(buildPath);
  }
  // Delete the cached json file
  auto filePath = GetCachePath(instance);
  if (FileUtils::FileExists(filePath)) {
    std::filesystem::remove_all(filePath);
  }

  RemoveIPInstance(instance);
}

void IPGenerator::DeleteIPInstance(const std::string& moduleName) {
  DeleteIPInstance(GetIPInstance(moduleName));
}

bool IPGenerator::Generate() {
  bool status = true;
  Compiler* compiler = GetCompiler();
  std::vector<IPInstance*> instances{};

  if (compiler->IPGenOpt() == Compiler::IPGenerateOpt::List) {
    // Take a list of moduleNames and only generate those IPs
    std::vector<std::string> modules;
    StringUtils::tokenize(compiler->IPGenMoreOpt(), " ", modules);
    for (const auto& moduleName : modules) {
      IPInstance* inst = GetIPInstance(moduleName);
      if (inst) {
        instances.push_back(inst);
      }
    }
  } else {
    // Generate all IPs
    instances = m_instances;
  }

  for (IPInstance* inst : instances) {
    // Create output directory
    const std::filesystem::path& out_path = inst->OutputFile();
    if (!std::filesystem::exists(out_path)) {
      std::filesystem::create_directories(out_path.parent_path());
    }

    const IPDefinition* def = inst->Definition();
    switch (def->Type()) {
      case IPDefinition::IPType::Other: {
        break;
      }
      case IPDefinition::IPType::LiteXGenerator: {
        const std::filesystem::path executable = def->FilePath();
        std::filesystem::path jsonFile = GetCachePath(inst);
        std::stringstream previousbuffer;
        if (FileUtils::FileExists(jsonFile)) {
          std::ifstream previous(jsonFile);
          std::stringstream buffer;
          previousbuffer << previous.rdbuf();
        }

        // Create directory path if it doesn't exist otherwise the following
        // ofstream command will fail
        FileUtils::MkDirs(jsonFile.parent_path());
        std::ofstream jsonF(jsonFile);
        jsonF << "{" << std::endl;
        for (const auto& param : inst->Parameters()) {
          std::string value;
          // The configure_ip command loses type info because we go from full
          // json meta data provided by the ip_catalog generators to a single
          // -Pname=val argument in a tcl command line. As such, we'll use the
          // ip catalog's definition for parameter type info
          auto catalogParam = GetCatalogParam(inst, param.Name());
          if (catalogParam) {
            switch (catalogParam->GetType()) {
              case Value::Type::ParamIpVal: {
                value = param.GetSValue();
                auto type = ((IPParameter*)catalogParam)->GetParamType();
                if (type == IPParameter::ParamType::FilePath ||
                    type == IPParameter::ParamType::String) {
                  value = "\"" + value + "\"";
                }
                break;
              }
              case Value::Type::ParamString:
                value = param.GetSValue();
                value = "\"" + value + "\"";
                break;
              case Value::Type::ParamInt:
                value = param.GetSValue();
                break;
              case Value::Type::ConstInt:
                value = param.GetSValue();
            }
          }
          jsonF << "   \"" << param.Name() << "\": " << value << ","
                << std::endl;
        }
        jsonF << "   \"build_dir\": " << inst->OutputFile().parent_path() << ","
              << std::endl;
        jsonF << "   \"build_name\": " << inst->OutputFile().filename() << ","
              << std::endl;
        jsonF << "   \"build\": true," << std::endl;
        jsonF << "   \"json\": \"" << jsonFile.filename().string() << "\","
              << std::endl;
        jsonF << "   \"json_template\": false" << std::endl;
        jsonF << "}" << std::endl;
        jsonF.close();
        std::stringstream newbuffer;
        if (FileUtils::FileExists(jsonFile)) {
          std::ifstream newfile(jsonFile);
          std::stringstream buffer;
          newbuffer << newfile.rdbuf();
        }
        if (newbuffer.str() == previousbuffer.str()) {
          m_compiler->Message("IP Generate, reusing IP " +
                              GetBuildDir(inst).string());
          continue;
        }

        // Find path to litex enabled python interpreter
        std::filesystem::path pythonPath = IPCatalog::getPythonPath();
        if (pythonPath.empty()) {
          std::filesystem::path python3Path =
              FileUtils::LocateExecFile("python3");
          if (python3Path.empty()) {
            m_compiler->ErrorMessage(
                "IP Generate, unable to find python interpreter in local "
                "environment.\n");
            return false;
          } else {
            pythonPath = python3Path;
            m_compiler->ErrorMessage(
                "IP Generate, unable to find python interpreter in local "
                "environment, using system copy '" +
                python3Path.string() +
                "'. Some IP Catalog features might not work with this "
                "interpreter.\n");
          }
        }

        StringVector args{executable.string(), "--build", "--json",
                          FileUtils::GetFullPath(jsonFile).string()};
        std::ostringstream help;
        m_compiler->Message("IP Generate, generating IP " +
                            GetBuildDir(inst).string());
        if (FileUtils::ExecuteSystemCommand(pythonPath.string(), args, &help)
                .code) {
          m_compiler->ErrorMessage("IP Generate, " + help.str());
          return false;
        }

        break;
      }
    }
  }
  return status;
}

std::pair<bool, std::string> IPGenerator::IsSimulateIpSupported(
    const std::string& name) const {
  auto it =
      std::find_if(m_instances.begin(), m_instances.end(),
                   [name](IPInstance* i) { return name == i->ModuleName(); });
  if (it == m_instances.end())
    return {false, "No IP generated with name " + name};

  IPInstance* inst{*it};
  auto path = GetSimDir(inst);

  if (!FileUtils::FileExists(path / "Makefile"))
    return {false, "Simulation not available for " + name};
  return {true, {}};
}

std::pair<bool, std::string> IPGenerator::SimulateIpTcl(
    const std::string& name) {
  auto it =
      std::find_if(m_instances.begin(), m_instances.end(),
                   [name](IPInstance* i) { return name == i->ModuleName(); });
  if (it == m_instances.end())
    return {false, "No IP generated with name " + name};

  IPInstance* inst{*it};
  auto path = GetSimDir(inst);

  auto [supported, message] = IsSimulateIpSupported(name);
  if (!supported) return {supported, message};

  auto artifactsPath{GetSimArtifactsDir(inst)};
  if (!FileUtils::MkDirs(artifactsPath)) {
    return {false, "Failed to create folder " + artifactsPath.string()};
  }

  std::string command = "make";
  StringVector args{"OUT_DIR=" + artifactsPath.string()};
  if (auto ret = FileUtils::ExecuteSystemCommand(
          command, args, m_compiler->GetOutStream(), -1, path.string(),
          m_compiler->GetErrStream());
      ret.code != 0) {
    return {false, ret.message};
  }
  return {true, std::string{}};
}

void IPGenerator::SimulateIp(const std::string& name) {
  int returnVal{};
  auto resultStr =
      GlobalSession->TclInterp()->evalCmd("simulate_ip " + name, &returnVal);
  if (returnVal != TCL_OK) {
    if (m_compiler) m_compiler->ErrorMessage(resultStr);
  }
}

std::pair<bool, std::string> IPGenerator::OpenWaveForm(
    const std::string& name) {
  auto it =
      std::find_if(m_instances.begin(), m_instances.end(),
                   [name](IPInstance* i) { return name == i->ModuleName(); });
  if (it == m_instances.end())
    return {false, "No IP generated with name " + name};

  IPInstance* inst{*it};
  auto path = GetSimDir(inst);

  auto [supported, message] = IsSimulateIpSupported(name);
  if (!supported) return {supported, message};

  auto artifactsPath{GetSimArtifactsDir(inst)};
  StringVector extensions{".fst", ".vcd"};
  for (const auto& ext : extensions) {
    auto file = FileUtils::FindFileByExtension(artifactsPath, ext);
    if (!file.empty()) {
      const std::string cmd = "wave_open " + file.string();
      bool ok = GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
      return {ok, "Command \'" + cmd + "\' failed."};
    }
  }
  return {false,
          "Waveform file does not exist at " + artifactsPath.string() +
              ".\nSupported files: " + StringUtils::join(extensions, ", ")};
}

// This will return the expected VLNV path for the given instance
std::filesystem::path IPGenerator::GetBuildDir(IPInstance* instance) const {
  auto projectIPsPath = GetProjectIPsPath();
  if (!projectIPsPath.empty())
    return GetMetaPath(projectIPsPath, instance) / instance->ModuleName();
  return {};
}

std::filesystem::path IPGenerator::GetSimDir(IPInstance* instance) const {
  return GetBuildDir(instance) / "sim";
}

std::filesystem::path IPGenerator::GetSimArtifactsDir(
    IPInstance* instance) const {
  std::filesystem::path dir{};
  auto projectIPsPath = GetProjectIPsPath();
  if (!projectIPsPath.empty())
    dir = GetMetaPath(projectIPsPath / "simulation", instance) /
          instance->ModuleName();
  return dir;
}

// This will return the path to this instance's cached json file
std::filesystem::path IPGenerator::GetCachePath(IPInstance* instance) const {
  std::filesystem::path dir{};

  if (m_compiler && m_compiler->ProjManager()) {
    std::filesystem::path ipPath = GetBuildDir(instance);
    auto def = instance->Definition();
    std::string ip_config_file =
        def->Name() + "_" + instance->ModuleName() + ".json";
    dir = ipPath / ip_config_file;
  }

  return dir;
}

std::filesystem::path IPGenerator::GetTmpCachePath(IPInstance* instance) const {
  std::filesystem::path dir{};
  if (m_compiler && m_compiler->ProjManager()) {
    auto ipPath = GetMetaPath(GetTmpPath(), instance) / instance->ModuleName();
    auto def = instance->Definition();
    std::string ip_config_file =
        def->Name() + "_" + instance->ModuleName() + ".json";
    dir = ipPath / ip_config_file;
  }
  return dir;
}

std::filesystem::path IPGenerator::GetTmpPath() const {
  auto projectIPsPath = GetProjectIPsPath();
  if (!projectIPsPath.empty()) return projectIPsPath / ".tmp";
  return {};
}

std::filesystem::path IPGenerator::GetProjectIPsPath() const {
  if (m_compiler && m_compiler->ProjManager()) {
    ProjectManager* projManager{m_compiler->ProjManager()};
    std::filesystem::path baseDir(projManager->projectPath());
    std::string projIpDir = projManager->projectName() + ".IPs";
    return baseDir / projIpDir;
  }
  return {};
}

std::filesystem::path IPGenerator::GetMetaPath(
    const std::filesystem::path& base, IPInstance* inst) const {
  auto meta = FOEDAG::getIpInfoFromPath(inst->Definition()->FilePath());
  auto ipPath = base / meta.vendor / meta.library / meta.name / meta.version;
  return ipPath;
}

// This will return a vector of paths to ./*.json and ./src/* in a given IP
// instance's build dir
std::vector<std::filesystem::path> IPGenerator::GetDesignFiles(
    IPInstance* instance) {
  std::vector<std::filesystem::path> paths{};

  // Get this IP's build path
  auto buildPath = GetBuildDir(instance);

  if (FileUtils::FileExists(buildPath)) {
    // Find the ip cache json file
    for (const auto& entry : std::filesystem::directory_iterator(buildPath)) {
      if (entry.path().extension() == ".json") {
        paths.push_back(entry.path());
      }
    }
    // Find any files in the ip src dir
    auto srcPath = buildPath / "src";
    if (FileUtils::FileExists(srcPath)) {
      for (const auto& entry : std::filesystem::directory_iterator(srcPath)) {
        paths.push_back(entry.path());
      }
    }
  }

  return paths;
}
