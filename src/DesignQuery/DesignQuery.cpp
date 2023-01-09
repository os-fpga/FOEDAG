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
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "DesignQuery/DesignQuery.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/StringUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
using json = nlohmann::ordered_json;

std::filesystem::path DesignQuery::GetProjDir() const {
  ProjectManager* projManager = m_compiler->ProjManager();
  std::filesystem::path dir(projManager->getProjectPath().toStdString());
  return dir;
}

std::filesystem::path DesignQuery::GetHierInfoPath() const {
  std::filesystem::path dir = GetProjDir();
  std::filesystem::path hier_info = "hier_info.json";
  return dir / hier_info;
}

std::filesystem::path DesignQuery::GetPortInfoPath() const {
  std::filesystem::path dir = GetProjDir();
  std::filesystem::path port_info = "port_info.json";
  return dir / port_info;
}

bool DesignQuery::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  auto parse_design_data = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    json& hier_json = design_query->getHierJson();
    json& port_json = design_query->getPortJson();

    std::filesystem::path port_info_path = design_query->GetPortInfoPath();
    std::filesystem::path hier_info_path = design_query->GetHierInfoPath();
    if (!FileUtils::FileExists(port_info_path)) {
      status = false;
      compiler->Message("port_info.json not found");
    } else {
      compiler->Message("port_info.json found!");
      std::ifstream port_info_f(port_info_path);
      port_json = json::parse(port_info_f);
      
    }
    if (!FileUtils::FileExists(hier_info_path)) {
      status = false;
      compiler->Message("hier_info.json not found");
    } else {
      compiler->Message("hier_info.json found!");
      std::ifstream hier_info_f(hier_info_path);
      hier_json = json::parse(hier_info_f);
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("parse_design_data", parse_design_data, this, 0);

  auto get_file_ids = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    compiler->Message("Called get_file_ids");

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_file_ids", get_file_ids, this, 0);

  auto get_modules = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    compiler->Message("Called get_modules");

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_modules", get_modules, this, 0);

  auto get_file_name = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    // Parse Argument file_id

    compiler->Message("Called get_file_name");

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_file_name", get_file_name, this, 0);

  auto get_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    compiler->Message("Called get_top_module");

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_top_module", get_top_module, this, 0);

  auto get_ports = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    // Parse Argument module_name

    compiler->Message("Called get_ports");

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_ports", get_ports, this, 0);
  return true;
}
