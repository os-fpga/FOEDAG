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

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || \
    defined(_MSC_VER) || defined(__CYGWIN__)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <direct.h>
#include <process.h>
#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif
#else
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
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

bool DesignQuery::LoadPortInfo() {
  bool status = true;

  if (!m_parsed_portinfo) {
    std::filesystem::path port_info_path = GetPortInfoPath();
    if (!FileUtils::FileExists(port_info_path)) {
      status = false;
      m_compiler->Message(
          "Unable to locate port_info.json in design directory: \"" +
          GetProjDir().string() + "\"");
    } else {
      std::ifstream port_info_f(port_info_path);
      m_port_json = json::parse(port_info_f);
      m_parsed_portinfo = true;
    }
  }

  return status;
}

bool DesignQuery::LoadHierInfo() {
  bool status = true;

  if (!m_parsed_hierinfo) {
    std::filesystem::path hier_info_path = GetHierInfoPath();
    if (!FileUtils::FileExists(hier_info_path)) {
      status = false;
      m_compiler->Message(
          "Unable to locate hier_info.json in design directory: \"" +
          GetProjDir().string() + "\"");
    } else {
      std::ifstream hier_info_f(hier_info_path);
      m_hier_json = json::parse(hier_info_f);
      m_parsed_hierinfo = true;
    }
  }

  return status;
}

bool DesignQuery::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  auto get_file_ids = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    if (!design_query->LoadHierInfo()) {
      status = false;
    } else {
      json& hier_info = design_query->getHierJson();
      json file_ids_obj = hier_info["fileIDs"];
      if (!file_ids_obj.is_object()) {
        status = false;
      } else {
        std::string ret = "";
        for (auto it = file_ids_obj.begin(); it != file_ids_obj.end(); it++) {
          ret += " ";
          ret += it.key();
        }
        compiler->TclInterp()->setResult(ret);
      }
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_file_ids", get_file_ids, this, 0);

  auto get_modules = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_modules", get_modules, this, 0);

  auto get_file_name = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_file_name", get_file_name, this, 0);

  auto get_top_module = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_top_module", get_top_module, this, 0);

  auto get_ports = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    // TODO: Implement this API
    bool status = true;

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_ports", get_ports, this, 0);
  return true;
}
