/**
 * @file DeviceModeling.cpp
 * @author Manadher Kharroubi (manadher@gmail.com)
 * @brief
 * @version 1.0
 * @date 2024-06-7
 *
 * @copyright Copyright (c) 2023
 *
 */

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
#include <direct.h>
#include <process.h>
#include <windows.h>

#include <ctime>
#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif
#else
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QProcess>
#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <typeinfo>

#include "Compiler/Compiler.h"
#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "DeviceModeling/DeviceModeling.h"
#include "MainWindow/Session.h"
#include "Model.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/StringUtils.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

std::filesystem::path DeviceModeling::GetProjDir() const {
  ProjectManager* projManager = m_compiler->ProjManager();
  std::filesystem::path dir(projManager->getProjectPath().toStdString());
  return dir;
}

bool DeviceModeling::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  auto test_device_modeling_tcl = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string ret = "return from test_device_modeling_tcl";
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("test_device_modeling_tcl", test_device_modeling_tcl,
                      this, 0);

  auto example_command_ret_i = [](void* clientData, Tcl_Interp* interp,
                                  int argc, const char* argv[]) -> int {
    bool status = true;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(argc));
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("example_command_ret_i", example_command_ret_i, this, 0);

  auto example_command_ret_list = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    bool status = true;
    Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
    // Append each argv element to the list.
    for (int i = 0; i < argc; ++i) {
      Tcl_ListObjAppendElement(interp, resultList,
                               Tcl_NewStringObj(argv[i], -1));
    }
    Tcl_SetObjResult(interp, resultList);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("example_command_ret_list", example_command_ret_list,
                      this, 0);

  auto device_name = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().device_name(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("device_name", device_name, this, 0);

  auto device_version = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().device_version(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("device_version", device_version, this, 0);

  auto schema_version = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().schema_version(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("schema_version", schema_version, this, 0);

  auto define_enum_type = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_enum_type(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_enum_type", define_enum_type, this, 0);

  auto define_block = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_block(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_block", define_block, this, 0);

  auto undefine_device = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().undefine_device(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("undefine_device", undefine_device, this, 0);

  auto define_ports = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_ports(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_ports", define_ports, this, 0);

  auto define_param_type = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_param_type(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_param_type", define_param_type, this, 0);

  auto define_param = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_param(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_param", define_param, this, 0);

  auto define_attr = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_attr(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_attr", define_attr, this, 0);

  auto define_constraint = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_constraint(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_constraint", define_constraint, this, 0);

  auto create_instance = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().create_instance(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("create_instance", create_instance, this, 0);

  auto define_properties = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_properties(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_properties", define_properties, this, 0);

  auto get_property = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      std::string ret = Model::get_modler().get_property(argc, argv);
      compiler->TclInterp()->setResult(ret);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_property", get_property, this, 0);

  auto add_block_to_chain_type = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().add_block_to_chain_type(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("add_block_to_chain_type", add_block_to_chain_type, this,
                      0);

  auto append_instance_to_chain = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().append_instance_to_chain(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("append_instance_to_chain", append_instance_to_chain,
                      this, 0);

  auto create_chain_instance = [](void* clientData, Tcl_Interp* interp,
                                  int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("create_chain_instance", create_chain_instance, this, 0);

  auto create_instance_chain = [](void* clientData, Tcl_Interp* interp,
                                  int argc, const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().create_instance_chain(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("create_instance_chain", create_instance_chain, this, 0);

  auto define_chain = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_chain(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_chain", define_chain, this, 0);

  auto define_net = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().define_net(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("define_net", define_net, this, 0);

  auto drive_net = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("drive_net", drive_net, this, 0);

  auto drive_port = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("drive_port", drive_port, this, 0);

  auto get_attributes = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_attributes(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_attributes", get_attributes, this, 0);

  auto get_parameters = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_parameters(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_parameters", get_parameters, this, 0);

  auto get_parameter_types = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_parameter_types(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_parameter_types", get_parameter_types, this, 0);

  auto get_block_names = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_block_names(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_block_names", get_block_names, this, 0);

  auto get_instance_chain_names = [](void* clientData, Tcl_Interp* interp,
                                     int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_chain_names", get_instance_chain_names,
                      this, 0);

  auto get_instance_chain_by_name = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_chain_by_name", get_instance_chain_by_name,
                      this, 0);

  auto get_constraint_names = [](void* clientData, Tcl_Interp* interp, int argc,
                                 const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_constraint_names(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_constraint_names", get_constraint_names, this, 0);

  auto get_constraint_by_name = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    bool status = true;
    auto c_name = Model::get_modler().get_constraint_by_name(argc, argv);
    // Append each block name to the list.
    Tcl_SetObjResult(interp, Tcl_NewStringObj(c_name.c_str(), -1));
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_constraint_by_name", get_constraint_by_name, this,
                      0);

  auto get_instance_block_name = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_block_name", get_instance_block_name, this,
                      0);
  auto get_instance_block_type = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      auto c_name = Model::get_modler().get_instance_block_type(argc, argv);
      // Append each block name to the list.
      Tcl_SetObjResult(interp, Tcl_NewStringObj(c_name.c_str(), -1));
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_block_type", get_instance_block_type, this,
                      0);

  auto get_instance_by_id = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_by_id", get_instance_by_id, this, 0);

  auto get_instance_id = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_id", get_instance_id, this, 0);

  auto get_instance_id_set = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_id_set", get_instance_id_set, this, 0);

  auto get_instance_names = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_instance_names(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_names", get_instance_names, this, 0);

  auto get_instance_chains_names = [](void* clientData, Tcl_Interp* interp,
                                      int argc, const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_instance_chains_names(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_chains_names", get_instance_chains_names,
                      this, 0);

  auto get_instance_chain = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_instance_chain(argc, argv);
      // Append each block name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_chain", get_instance_chain, this, 0);

  auto get_instance_name_set = [](void* clientData, Tcl_Interp* interp,
                                  int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_instance_name_set", get_instance_name_set, this, 0);

  auto get_io_bank = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      auto c_name = Model::get_modler().get_io_bank(argc, argv);
      // Append each block name to the list.
      Tcl_SetObjResult(interp, Tcl_NewStringObj(c_name.c_str(), -1));
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_io_bank", get_io_bank, this, 0);

  auto get_logic_address = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      auto i_add = Model::get_modler().get_logic_address(argc, argv);
      Tcl_SetObjResult(interp, Tcl_NewIntObj(i_add));
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_logic_address", get_logic_address, this, 0);

  auto set_logic_address = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().set_logic_address(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("set_logic_address", set_logic_address, this, 0);

  auto get_logic_location = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto locations = Model::get_modler().get_logic_location(argc, argv);
      // Append each port name to the list.
      for (auto n : locations) {
        Tcl_ListObjAppendElement(interp, resultList, Tcl_NewIntObj(n));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_logic_location", get_logic_location, this, 0);

  auto get_net_sink_set = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_net_sink_set", get_net_sink_set, this, 0);

  auto get_net_source = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_net_source", get_net_source, this, 0);

  auto get_parent = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_parent", get_parent, this, 0);

  auto get_phy_address = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      auto i_add = Model::get_modler().get_phy_address(argc, argv);
      Tcl_SetObjResult(interp, Tcl_NewIntObj(i_add));
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_phy_address", get_phy_address, this, 0);

  auto get_port_connections = [](void* clientData, Tcl_Interp* interp, int argc,
                                 const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_port_connections", get_port_connections, this, 0);

  auto get_port_connection_sink_set = [](void* clientData, Tcl_Interp* interp,
                                         int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_port_connection_sink_set",
                      get_port_connection_sink_set, this, 0);

  auto get_port_connection_source = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_port_connection_source", get_port_connection_source,
                      this, 0);

  auto get_port_list = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      Tcl_Obj* resultList = Tcl_NewListObj(0, NULL);
      auto b_names = Model::get_modler().get_port_list(argc, argv);
      // Append each port name to the list.
      for (auto n : b_names) {
        Tcl_ListObjAppendElement(interp, resultList,
                                 Tcl_NewStringObj(n.c_str(), -1));
      }
      Tcl_SetObjResult(interp, resultList);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_port_list", get_port_list, this, 0);

  auto link_chain = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    // TODO: Implement this API
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = true;
    std::string cmd(argv[0]);
    std::string ret = "__Not Yet Integrated " + cmd;
    compiler->TclInterp()->setResult(ret);
    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("link_chain", link_chain, this, 0);

  auto map_rtl_user_names = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().map_rtl_user_names(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;  // map_rtl_user_names
  };
  interp->registerCmd("map_rtl_user_names", map_rtl_user_names, this, 0);

  auto get_rtl_name = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      std::string ret = Model::get_modler().get_rtl_name(argc, argv);
      compiler->TclInterp()->setResult(ret);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_rtl_name", get_rtl_name, this, 0);

  auto map_model_user_names = [](void* clientData, Tcl_Interp* interp, int argc,
                                 const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().map_model_user_names(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }
    return (status) ? TCL_OK : TCL_ERROR;  // map_rtl_user_names
  };
  interp->registerCmd("map_model_user_names", map_model_user_names, this, 0);

  auto get_model_name = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      std::string ret = Model::get_modler().get_model_name(argc, argv);
      compiler->TclInterp()->setResult(ret);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_model_name", get_model_name, this, 0);

  auto get_user_name = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      std::string ret = Model::get_modler().get_user_name(argc, argv);
      compiler->TclInterp()->setResult(ret);
      status = true;
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("get_user_name", get_user_name, this, 0);

  auto set_io_bank = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().set_io_bank(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("set_io_bank", set_io_bank, this, 0);

  auto set_logic_location = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().set_logic_location(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("set_logic_location", set_logic_location, this, 0);

  auto set_phy_address = [](void* clientData, Tcl_Interp* interp, int argc,
                            const char* argv[]) -> int {
    DeviceModeling* device_modeling = (DeviceModeling*)clientData;
    Compiler* compiler = device_modeling->GetCompiler();
    bool status = false;
    try {
      status = Model::get_modler().set_phy_address(argc, argv);
    } catch (const std::exception& ex) {
      compiler->ErrorMessage(ex.what());
    } catch (...) {
      compiler->ErrorMessage("Unknown Exception");
    }

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("set_phy_address", set_phy_address, this, 0);

  return true;
}
