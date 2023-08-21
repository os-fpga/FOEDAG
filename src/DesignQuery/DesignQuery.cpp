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

#include "DesignQuery/DesignQuery.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QProcess>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/StringUtils.h"
#include "sdtgen_cpp_nlohman_lib_v6.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
// using json = nlohmann::ordered_json;
using json = nlohmann::json;

int SdtCpuInstSubNode::total_instances;
int SdtCpuClusterInstSubNode::total_instances;
int SdtMemoryInstSubNode::total_instances;
int SdtSocInstSubNode::total_instances;

// string sdt_file_path_global = "output_sdtgen_cpp_nlohman_lib_v6.sdt";
// int verbose_flag_global = 0; //1;

// int SdtCpuInstSubNode::total_instances = 0;
// int SdtCpuClusterInstSubNode::total_instances = 0;
// int SdtMemoryInstSubNode::total_instances = 0;
// int SdtSocInstSubNode::total_instances = 0;

// // int SdtCpuInstSubNode::total_instances;
// // int SdtCpuClusterInstSubNode::total_instances;
// // int SdtMemoryInstSubNode::total_instances;
// // int SdtSocInstSubNode::total_instances;

// // SdtCpuInstSubNode::total_instances = 0;
// // SdtCpuClusterInstSubNode::total_instances = 0;
// // SdtMemoryInstSubNode::total_instances = 0;
// // SdtSocInstSubNode::total_instances = 0;

// SdtCpusNode cpus_node_obj;

// SdtCpusClusterNode cpus_cluster_node_obj;

// SdtMemoryNode memory_node_obj;

// SdtSocNode soc_node_obj;

// SdtRootMetaDataNode rootmetadata_node_obj;

// int size;

// // read JSON file
// ifstream inputFile;

// // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
// inputFile.open("gold_hier_v5.json");

// // ifstream inputFile("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");

// // std::ifstream::inputFile.open();

// // calculating size of inputFile
// inputFile.seekg(0, inputFile.end);
// size = inputFile.tellg();
// inputFile.seekg (0, inputFile.beg);

// // allocate memory:
// char * data_file = new char[size];

// // read data as a block:
// inputFile.read (data_file, size);

// inputFile.close();

// //get meta-data of all SDT nodes from JSON file
// json data = json::parse(data_file);

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
  auto sdt_gen_cpus_node = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    }

    // class SdtCpuInstSubNode variable total_instances
    SdtCpuInstSubNode::total_instances = 0;

    // SdtCpusNode class object
    SdtCpusNode cpus_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get cpus node from JSON file
    int result = get_cpus_node(data, cpus_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    int result2;

    // generate cpus node in SDT file
    result2 = gen_cpus_node(outfile, cpus_node_obj, verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_cpus_node", sdt_gen_cpus_node, this, 0);

  auto sdt_gen_cpus_cluster_node = [](void* clientData, Tcl_Interp* interp,
                                      int argc, const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    }

    // class SdtCpuClusterInstSubNode variable total_instances
    SdtCpuClusterInstSubNode::total_instances = 0;

    // SdtCpusClusterNode class object
    SdtCpusClusterNode cpus_cluster_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get cpus cluster node from JSON file
    int result =
        get_cpus_cluster_node(data, cpus_cluster_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    int result2;

    // generate cpus cluster node in SDT file
    result2 = gen_cpus_cluster_node(outfile, cpus_cluster_node_obj,
                                    verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_cpus_cluster_node", sdt_gen_cpus_cluster_node,
                      this, 0);

  auto sdt_gen_memory_node = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    }

    // class SdtMemoryInstSubNode variable total_intances
    SdtMemoryInstSubNode::total_instances = 0;

    // SdtMemoryNode class object
    SdtMemoryNode memory_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get memory node from JSON file
    int result = get_memory_node(data, memory_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    int result2;

    // generate memory node in SDT file
    result2 = gen_memory_node(outfile, memory_node_obj, verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_memory_node", sdt_gen_memory_node, this, 0);

  auto sdt_gen_soc_node = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    }

    // class SdtSocInstSubNode variable total_intances
    SdtSocInstSubNode::total_instances = 0;

    // SdtSocNode class object
    SdtSocNode soc_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get soc node from JSON file
    int result = get_soc_node(data, soc_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    int result2;

    // generate soc node in SDT file
    result2 = gen_soc_node(outfile, soc_node_obj, verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_soc_node", sdt_gen_soc_node, this, 0);

  auto sdt_gen_root_metadata_node = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    };

    // SdtRootMetaDataNode class object
    SdtRootMetaDataNode rootmetadata_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get root metadata from JSON file
    int result =
        get_rootmetadata_node(data, rootmetadata_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    int result2;

    // generate root metadata node in SDT file
    result2 = gen_rootmetadata_node(outfile, rootmetadata_node_obj,
                                    verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_root_metadata_node", sdt_gen_root_metadata_node,
                      this, 0);

  auto sdt_gen_system_device_tree = [](void* clientData, Tcl_Interp* interp,
                                       int argc, const char* argv[]) -> int {
    DesignQuery* design_query =
        (DesignQuery*)clientData;  // typecasting pointer
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;
    std::string cmd_name = std::string(argv[0]);
    // argv[0] is the main function name which in this case is
    // "sdt_get_sdt_nodes_dict_from_json"
    std::string ret = "\n\n" + cmd_name + "__IMPLEMENTED__";

    string sdt_file_path_global = cmd_name + "_output_sdt.sdt";

    int verbose_flag_global;  // = 1; //1;

    if ((argc > 1) && (string(argv[1]) == "verbose")) {
      verbose_flag_global = 1;
    } else {
      verbose_flag_global = 0;
    }

    // classes variable total_intances init
    SdtCpuInstSubNode::total_instances = 0;
    SdtCpuClusterInstSubNode::total_instances = 0;
    SdtMemoryInstSubNode::total_instances = 0;
    SdtSocInstSubNode::total_instances = 0;

    // SdtCpusNode class object
    SdtCpusNode cpus_node_obj;

    // SdtCpusClusterNode class object
    SdtCpusClusterNode cpus_cluster_node_obj;

    // SdtMemoryNode class object
    SdtMemoryNode memory_node_obj;

    // SdtSocNode class object
    SdtSocNode soc_node_obj;

    // SdtRootMetaDataNode class object
    SdtRootMetaDataNode rootmetadata_node_obj;

    int size;

    // read JSON file
    ifstream inputFile;

    // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
    // inputFile.open("gold_hier_v5.json");
    // inputFile.open("sdt_dev_zaid_rapidsilicon_example_soc_v5.json");
    inputFile.open(
        "./src/DesignQuery/data/JSON_Files/"
        "sdt_dev_zaid_rapidsilicon_example_soc_v5.json");

    // calculating size of inputFile
    inputFile.seekg(0, inputFile.end);
    size = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    // allocate memory:
    char* data_file = new char[size];

    // read data as a block:
    inputFile.read(data_file, size);

    inputFile.close();

    // get meta-data of all SDT nodes from JSON file
    json data = json::parse(data_file);

    // get rootmetadata node from JSON file
    int result =
        get_rootmetadata_node(data, rootmetadata_node_obj, verbose_flag_global);

    // get cpus node from JSON file
    int result3 = get_cpus_node(data, cpus_node_obj, verbose_flag_global);

    // get cpus-cluster node from JSON file
    int result4 =
        get_cpus_cluster_node(data, cpus_cluster_node_obj, verbose_flag_global);

    // get soc node from JSON file
    int result5 = get_memory_node(data, memory_node_obj, verbose_flag_global);

    // get cpus node from JSON file
    int result6 = get_soc_node(data, soc_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    *Copyright (c) 2023 Rapid Silicon\n \
    *SPDX-License-Identifier: rs-eula\n \
    *JSON to SDT cpp script written by ZaidTahir, for questions please email: zaid.butt.tahir@gmail.com or zaidt@bu.edu\n*/\n\n";

    outfile << "/dts-v1/;\n\n"
            << "/ {\n";

    outfile.flush();

    // generate SystemDeviceTree in SDT file

    // generate root metadata node in SDT file
    int result2 = gen_rootmetadata_node(outfile, rootmetadata_node_obj,
                                        verbose_flag_global);

    // generate cpus node in SDT file
    int result7 = gen_cpus_node(outfile, cpus_node_obj, verbose_flag_global);

    // generate cpus-cluster node in SDT file
    int result8 = gen_cpus_cluster_node(outfile, cpus_cluster_node_obj,
                                        verbose_flag_global);

    // generate memory node in SDT file
    int result9 =
        gen_memory_node(outfile, memory_node_obj, verbose_flag_global);

    // generate soc node in SDT file
    int result10 = gen_soc_node(outfile, soc_node_obj, verbose_flag_global);

    outfile << "\n\n};" << endl;

    outfile.flush();

    string string_outfile =
        return_string_from_ofstream_file(outfile, sdt_file_path_global);

    ret = ret + "\n\n\nOutput of tcl command \"" + cmd_name +
          "\" is shown below:\n\n" + string_outfile;

    outfile.close();

    status = result2 & result & result3 & result4 & result5 & result6 &
             result7 & result8 & result9 & result10;

    string return_status;

    if (status) {
      return_status = "True";
    } else {
      return_status = "False";
    }

    ret = ret + "\n\nReturn status of command \"" + cmd_name +
          "\" is = " + return_status + "\n\n";

    compiler->TclInterp()->setResult(ret);

    return (status) ? TCL_OK : TCL_ERROR;
  };
  interp->registerCmd("sdt_gen_system_device_tree", sdt_gen_system_device_tree,
                      this, 0);

  // auto get_file_ids = [](void* clientData, Tcl_Interp* interp, int argc,
  //                        const char* argv[]) -> int {
  //   DesignQuery* design_query = (DesignQuery*)clientData;
  //   Compiler* compiler = design_query->GetCompiler();
  //   bool status = true;

  //   if (!design_query->LoadHierInfo()) {
  //     status = false;
  //   } else {
  //     json& hier_info = design_query->getHierJson();
  //     json file_ids_obj = hier_info["fileIDs"];
  //     if (!file_ids_obj.is_object()) {
  //       status = false;
  //     } else {
  //       std::string ret = "";
  //       for (auto it = file_ids_obj.begin(); it != file_ids_obj.end(); it++)
  //       {
  //         ret += " ";
  //         ret += it.key();
  //       }
  //       compiler->TclInterp()->setResult(ret);
  //     }
  //   }

  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("get_file_ids", get_file_ids, this, 0);

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

  //  // first SDT API function implementation
  //   // get dict of all SDT nodes from JSON file
  // auto sdt_get_sdt_nodes_dict_from_json = [](void* clientData, Tcl_Interp*
  // interp, int argc,
  //                        const char* argv[]) -> int {
  //     // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]);
  //     // argv[0] is the main function name which in this case is
  //     "sdt_get_sdt_nodes_dict_from_json"
  //   std::string ret = cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_get_sdt_nodes_dict_from_json",
  // sdt_get_sdt_nodes_dict_from_json, this, 0);

  // // SDT API function implementation
  //   // get cpus node meta-data
  // auto sdt_get_cpus = [](void* clientData, Tcl_Interp* interp, int argc,
  //                        const char* argv[]) -> int {
  //     // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_get_cpus", sdt_get_cpus, this, 0);

  // // SDT API function implementation
  //   // get cpu-clusters node meta-data
  // auto sdt_get_cpus_clusters = [](void* clientData, Tcl_Interp* interp, int
  // argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_get_cpus_clusters", sdt_get_cpus_clusters, this,
  // 0);

  // // SDT API function implementation
  //   // get memory nodes meta-data
  // auto sdt_get_memory_nodes = [](void* clientData, Tcl_Interp* interp, int
  // argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_get_memory_nodes", sdt_get_memory_nodes, this, 0);

  // // SDT API function implementation
  //   // get soc user-logic meta-data (ips)
  // auto sdt_get_soc = [](void* clientData, Tcl_Interp* interp, int argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_get_soc", sdt_get_soc, this, 0);

  // // SDT API function implementation
  //   // generating/writing cpus node
  // auto sdt_gen_cpu_node = [](void* clientData, Tcl_Interp* interp, int argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_gen_cpu_node", sdt_gen_cpu_node, this, 0);

  // // SDT API function implementation
  //   // generating/writing cpus_cluster node
  // auto sdt_gen_cpu_cluster_node = [](void* clientData, Tcl_Interp* interp,
  // int argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_gen_cpu_cluster_node", sdt_gen_cpu_cluster_node,
  // this, 0);

  // // SDT API function implementation
  //   // generating/writing memory node
  // auto sdt_gen_memory_nodes = [](void* clientData, Tcl_Interp* interp, int
  // argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_gen_memory_nodes", sdt_gen_memory_nodes, this, 0);

  // // SDT API function implementation
  //   // generating/Wrtiting user-logic under SOC sdt node
  // auto sdt_gen_soc_node = [](void* clientData, Tcl_Interp* interp, int argc,
  //                        const char* argv[]) -> int {
  //    // TODO: Implement this API
  //   DesignQuery* design_query = (DesignQuery*)clientData;  // typecasting
  //   pointer Compiler* compiler = design_query->GetCompiler(); bool status =
  //   true; std::string cmd_name = std::string(argv[0]); std::string ret =
  //   cmd_name + "__NOT__YET__IMPLEMENTED__";
  //   compiler->TclInterp()->setResult(ret);
  //   return (status) ? TCL_OK : TCL_ERROR;
  // };
  // interp->registerCmd("sdt_gen_soc_node", sdt_gen_soc_node, this, 0);

  return true;
}
