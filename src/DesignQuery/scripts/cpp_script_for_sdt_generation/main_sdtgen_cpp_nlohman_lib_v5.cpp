
//**********************************************************************************************************************************
// JSON to SDT cpp script written by ZaidTahir, for questions please email:
// zaid.butt.tahir@gmail.com or zaidt@bu.edu **************
// *********************************************************************************************************************************

#include <cstdio>
// #include <nlohmann_json/json.hpp>
#include "nlohmann_json/json.hpp"
// #include "json.hpp"
// #include "json/single_include/nlohmann/json.hpp"
// #include <json/single_include/nlohmann/json_fwd.hpp>

#include "sdtgen_cpp_nlohman_lib_v5.h"
// #include "sdtgen_cpp_nlohman_lib_v5.cpp"

using json = nlohmann::json;

using namespace std;

#include <fstream>
// #include <ofstream>
#include <cstring>
#include <string>
// #include <print>
#include <iostream>
#include <limits>
#include <sstream>  // header file for stringstream
#include <stdexcept>

string sdt_file_path_global = "output_sdtgen_cpp_nlohman_lib_v5.sdt";
int verbose_flag_global = 0;  // 1;

int SdtCpuInstSubNode::total_instances = 0;
int SdtCpuClusterInstSubNode::total_instances = 0;
int SdtMemoryInstSubNode::total_instances = 0;
int SdtSocInstSubNode::total_instances = 0;
// works as well

int main(int, char*[]) {
  SdtCpusNode cpus_node_obj;

  SdtCpusClusterNode cpus_cluster_node_obj;

  SdtMemoryNode memory_node_obj;

  SdtSocNode soc_node_obj;

  SdtRootMetaDataNode rootmetadata_node_obj;

  int size;

  // read JSON file
  ifstream inputFile;

  inputFile.open(
      "JSON_Files/GPIO_Yosis_Ver_Example/"
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

  json data = json::parse(data_file);

  get_rootmetadata_node(data, rootmetadata_node_obj, verbose_flag_global);

  get_cpus_node(data, cpus_node_obj, verbose_flag_global);

  get_cpus_cluster_node(data, cpus_cluster_node_obj, verbose_flag_global);

  get_memory_node(data, memory_node_obj, verbose_flag_global);

  get_soc_node(data, soc_node_obj, verbose_flag_global);

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

  gen_rootmetadata_node(outfile, rootmetadata_node_obj, verbose_flag_global);

  gen_cpus_node(outfile, cpus_node_obj, verbose_flag_global);

  gen_cpus_cluster_node(outfile, cpus_cluster_node_obj, verbose_flag_global);

  gen_memory_node(outfile, memory_node_obj, verbose_flag_global);

  gen_soc_node(outfile, soc_node_obj, verbose_flag_global);

  outfile << "\n\n};" << endl;

  outfile.flush();

  string string_outfile =
      return_string_from_ofstream_file(outfile, sdt_file_path_global);

  cout << "Priting outfile = \n\n" << string_outfile << endl;

  outfile.close();

  return 0;
}
