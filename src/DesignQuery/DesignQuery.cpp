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
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/Constraints.h"
#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Utils/FileUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/StringUtils.h"
#include "sdtgen.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;
using json_sdt = nlohmann::json;
using json = nlohmann::ordered_json;

int SdtCpuInstSubNode::total_instances;
int SdtCpuClusterInstSubNode::total_instances;
int SdtMemoryInstSubNode::total_instances;
int SdtSocInstSubNode::total_instances;

std::filesystem::path DesignQuery::GetProjDir() const {
  ProjectManager* projManager = m_compiler->ProjManager();
  std::filesystem::path dir(projManager->getProjectPath().toStdString());
  return dir;
}

std::filesystem::path DesignQuery::GetHierInfoPath() const {
  std::filesystem::path hier_info =
      m_compiler->FilePath(Compiler::Action::Analyze, "hier_info.json");
  return hier_info;
}

std::filesystem::path DesignQuery::GetPortInfoPath() const {
  std::filesystem::path port_info =
      m_compiler->FilePath(Compiler::Action::Analyze, "port_info.json");
  return port_info;
}

std::pair<bool, std::string> DesignQuery::LoadPortInfo() {
  if (!m_parsed_portinfo) {
    std::filesystem::path port_info_path = GetPortInfoPath();
    if (!FileUtils::FileExists(port_info_path)) {
      return std::make_pair(false,
                            StringUtils::format(R"(Unable to locate file "%")",
                                                port_info_path.string()));
    } else {
      std::ifstream port_info_f(port_info_path);
      try {
        m_port_json = json::parse(port_info_f);
      } catch (std::exception&) {
        return std::make_pair(false,
                              StringUtils::format("Failed to parse file %",
                                                  port_info_path.string()));
      }
      m_parsed_portinfo = true;
    }
  }
  return std::make_pair(true, std::string{});
}

std::pair<bool, std::string> DesignQuery::LoadHierInfo() {
  if (!m_parsed_hierinfo) {
    std::filesystem::path hier_info_path = GetHierInfoPath();
    if (!FileUtils::FileExists(hier_info_path)) {
      return std::make_pair(false,
                            StringUtils::format(R"(Unable to locate file "%")",
                                                hier_info_path.string()));
    } else {
      std::ifstream hier_info_f(hier_info_path);
      try {
        m_hier_json = json::parse(hier_info_f);
      } catch (std::exception&) {
        return std::make_pair(false,
                              StringUtils::format("Failed to parse file %",
                                                  hier_info_path.string()));
      }
      m_parsed_hierinfo = true;
    }
  }
  return std::make_pair(true, std::string{});
}

std::vector<string> DesignQuery::GetPorts(int portType,
                                          bool& portsParsed) const {
  if (portType == 0) return {};
  static const int PortsInput{1};
  static const int PortsOutput{2};
  static const std::string input{"Input"};
  static const std::string output{"Output"};
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  try {
    const json& hier_info = getHierJson();
    auto hierTree = hier_info.at("hierTree");
    for (const auto& item : hierTree) {
      auto portsArr = item.at("ports");
      for (auto it{portsArr.cbegin()}; it != portsArr.cend(); ++it) {
        auto direction = it->at("direction");
        if (((portType & PortsInput) != 0) && direction == input) {
          inputs.push_back(it->at("name"));
        }
        if (((portType & PortsOutput) != 0) && direction == output) {
          outputs.push_back(it->at("name"));
        }
      }
    }
  } catch (std::exception& exception) {
    portsParsed = false;
    qWarning() << exception.what();
    return {};
  }

  portsParsed = true;
  std::vector<std::string> ports = inputs;
  ports.insert(ports.end(), outputs.begin(), outputs.end());
  return ports;
}

std::vector<Bus> DesignQuery::GetBuses(int portType, bool& portsParsed) const {
  if (portType == 0) return {};
  static const int PortsInput{1};
  static const int PortsOutput{2};
  static const std::string input{"Input"};
  static const std::string output{"Output"};
  std::vector<Bus> inputs;
  std::vector<Bus> outputs;
  try {
    const json& hier_info = getHierJson();
    auto hierTree = hier_info.at("hierTree");
    for (const auto& item : hierTree) {
      auto portsArr = item.at("ports");
      for (auto it{portsArr.cbegin()}; it != portsArr.cend(); ++it) {
        auto direction = it->at("direction");
        if (((portType & PortsInput) != 0) && direction == input) {
          const auto range = it->at("range");
          const int msb = range["msb"];
          const int lsb = range["lsb"];
          if (msb != lsb) inputs.push_back({it->at("name"), lsb, msb});
        }
        if (((portType & PortsOutput) != 0) && direction == output) {
          const auto range = it->at("range");
          const int msb = range["msb"];
          const int lsb = range["lsb"];
          if (msb != lsb) outputs.push_back({it->at("name"), lsb, msb});
        }
      }
    }
  } catch (std::exception& exception) {
    portsParsed = false;
    qWarning() << exception.what();
    return {};
  }

  portsParsed = true;
  std::vector<Bus> ports = inputs;
  ports.insert(ports.end(), outputs.begin(), outputs.end());
  return ports;
}

void DesignQuery::SetReadSdc(bool read_sdc) { m_read_sdc = read_sdc; }

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
    json_sdt data = json_sdt::parse(data_file);

    // get cpus node from JSON file
    int result = get_cpus_node(data, cpus_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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
    json_sdt data = json_sdt::parse(data_file);

    // get cpus cluster node from JSON file
    int result =
        get_cpus_cluster_node(data, cpus_cluster_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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
    json_sdt data = json_sdt::parse(data_file);

    // get memory node from JSON file
    int result = get_memory_node(data, memory_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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
    json_sdt data = json_sdt::parse(data_file);

    // get soc node from JSON file
    int result = get_soc_node(data, soc_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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
    json_sdt data = json_sdt::parse(data_file);

    // get root metadata from JSON file
    int result =
        get_rootmetadata_node(data, rootmetadata_node_obj, verbose_flag_global);

    // open a file in write mode.
    ofstream outfile;
    outfile.open(sdt_file_path_global);

    outfile << "/*\n \
    *\n \
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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
    json_sdt data = json_sdt::parse(data_file);

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
    * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or https://github.com/zaidtahirbutt)\n \
    * @date 2023-08-30\n \
    * @copyright Copyright 2021 The Foedag team\n \
    \n \
    * GPL License\n \
    \n \
    * Copyright (c) 2021 The Open-Source FPGA Foundation\n \
    \n \
    * This program is free software: you can redistribute it and/or modify\n \
    * it under the terms of the GNU General Public License as published by\n \
    * the Free Software Foundation, either version 3 of the License, or\n \
    * (at your option) any later version.\n \
    \n \
    * This program is distributed in the hope that it will be useful,\n \
    * but WITHOUT ANY WARRANTY; without even the implied warranty of\n \
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n \
    * GNU General Public License for more details.\n \
    \n \
    * You should have received a copy of the GNU General Public License\n \
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. \n*/\n\n";

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

  auto get_file_ids = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    DesignQuery* design_query = (DesignQuery*)clientData;
    Compiler* compiler = design_query->GetCompiler();
    bool status = true;

    if (const auto& [ok, message] = design_query->LoadHierInfo(); !ok) {
      Tcl_AppendResult(interp, message.c_str(), nullptr);
      return TCL_ERROR;
    } else {
      const json& hier_info = design_query->getHierJson();
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
    if (argc < 2) return TCL_OK;
    DesignQuery* designQuery = static_cast<DesignQuery*>(clientData);
    if (!designQuery || !designQuery->m_compiler) return TCL_ERROR;
    Constraints* constraints = designQuery->GetCompiler()->getConstraints();
    if (!constraints) return TCL_ERROR;

    if (designQuery->m_read_sdc) {
      StringVector arguments;
      arguments.push_back(argv[0]);
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::string tmp = StringUtils::replaceAll(arg, "@*@", "{*}");
        if (tmp != "{*}") constraints->addKeep(tmp);
        arguments.push_back(tmp);
      }
      std::string returnVal =
          StringUtils::format("[%]", StringUtils::join(arguments, " "));
      Tcl_AppendResult(interp, returnVal.c_str(), (char*)NULL);
      return TCL_OK;
    }

    if (const auto& [ok, message] = designQuery->LoadHierInfo(); !ok) {
      Tcl_AppendResult(interp, message.c_str(), nullptr);
      return TCL_ERROR;
    }

    static const int PortsOutput{2};
    static const int PortsInput{1};
    bool portsParsed{true};
    auto designPorts =
        designQuery->GetPorts(PortsInput | PortsOutput, portsParsed);
    if (!portsParsed) {
      Tcl_AppendResult(interp, "Failed to parse json file", nullptr);
      return TCL_ERROR;
    }

    StringVector get_ports;
    for (int i = 1; i < argc; i++) {
      std::string arg{argv[i]};
      arg = StringUtils::replaceAll(arg, "@*@", "*");
      if (arg == "*") {
        get_ports = designPorts;
        break;
      }
      auto buses = designQuery->GetBuses(PortsInput | PortsOutput, portsParsed);
      if (!portsParsed) {
        Tcl_AppendResult(interp, "Failed to parse json file", nullptr);
        return TCL_ERROR;
      }
      const std::regex portRegex{R"((.+)\[(\d+)\])"};
      StringVector portsList = StringUtils::tokenize(arg, " ", true);
      for (const auto& port : portsList) {
        if (std::regex_match(port, portRegex)) {
          // handle buses
          std::smatch sm;
          std::regex_search(port, sm, portRegex);
          auto busName = sm[1].str();
          auto bitNumber = StringUtils::to_number<int>(sm[2].str()).first;
          auto findBus = std::find_if(
              buses.begin(), buses.end(), [busName, bitNumber](const Bus& bus) {
                return (bus.name == busName) && (bitNumber >= bus.lsb) &&
                       (bitNumber <= bus.msb);
              });
          if (findBus != buses.end()) get_ports.push_back(port);
        } else if (StringUtils::contains(port, '*')) {
          auto regexpr = StringUtils::replaceAll(port, "*", ".+");
          const std::regex regexp{regexpr};
          for (const auto& existingPort : designPorts) {
            if (std::regex_match(existingPort, regexp))
              get_ports.push_back(existingPort);
          }
        } else {
          if (StringUtils::contains(designPorts, port))
            get_ports.push_back(port);
        }
      }
    }

    const std::string returnVal = StringUtils::join(get_ports, " ");
    Tcl_AppendResult(interp, returnVal.c_str(), nullptr);
    return TCL_OK;
  };
  interp->registerCmd("get_ports", get_ports, this, 0);

  auto all_inputs = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    DesignQuery* designQuery = static_cast<DesignQuery*>(clientData);
    if (!designQuery || !designQuery->m_compiler) return TCL_ERROR;
    if (const auto& [ok, message] = designQuery->LoadHierInfo(); !ok) {
      Tcl_AppendResult(interp, message.c_str(), nullptr);
      return TCL_ERROR;
    }
    static const int PortsInput{1};
    bool portsParsed{true};
    auto ports = designQuery->GetPorts(PortsInput, portsParsed);
    if (!portsParsed) {
      Tcl_AppendResult(interp, "Failed to parse json file", nullptr);
      return TCL_ERROR;
    }
    Tcl_AppendResult(interp, StringUtils::join(ports, " ").c_str(), nullptr);
    return TCL_OK;
  };
  interp->registerCmd("all_inputs", all_inputs, this, 0);

  auto all_outputs = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    DesignQuery* designQuery = static_cast<DesignQuery*>(clientData);
    if (!designQuery || !designQuery->m_compiler) return TCL_ERROR;
    if (const auto& [ok, message] = designQuery->LoadHierInfo(); !ok) {
      Tcl_AppendResult(interp, message.c_str(), nullptr);
      return TCL_ERROR;
    }
    static const int PortsOutput{2};
    bool portsParsed{true};
    auto ports = designQuery->GetPorts(PortsOutput, portsParsed);
    if (!portsParsed) {
      Tcl_AppendResult(interp, "Failed to parse json file", nullptr);
      return TCL_ERROR;
    }
    Tcl_AppendResult(interp, StringUtils::join(ports, " ").c_str(), nullptr);

    return TCL_OK;
  };
  interp->registerCmd("all_outputs", all_outputs, this, 0);

  return true;
}
