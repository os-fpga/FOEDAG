/**
  * @file main_sdtgen_cpp_nlohman_lib_v5.cpp
  * @author Zaid Tahir (zaid.butt.tahir@gmail.com or zaidt@bu.edu or
  * https://github.com/zaidtahirbutt)
  * @date 2023-08-30
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <cstdio>

#include "../../../../third_party/nlohmann_json/json.hpp"
#include "sdtgen_cpp_nlohman_lib_v5.h"

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

// debugging using GDB
// §
// https://cs.franklin.edu/~shaffstj/oldcs2/debugging.htm#:~:text=Use%20a%20symbolic%20debugger%3A&text=You%20can%20examine%20the%20contents,code%20that%20caused%20the%20crash.
//                 □ v cool for line by line debugging of C++ code using
//                 breakpoint from gdb API □ used -g flag to use gdb
//                 functionality after checking that gdb is installed in my
//                 Ubuntu OS □ use "break (line number) to set up break points.
//                 □ Use "run" to start prog execution until first breakpoint
//                 □ Then use "next" to step through breakpoints.
// more functionality can be checked here
// https://web.eecs.umich.edu/~sugih/pointers/summary.html#:~:text=Gdb%20is%20a%20debugger%20for,variable%20after%20executing%20each%20line.

// // Global variables
// string node_tab = "\t";
// string subnode_tab = "\t\t";
// string subsubnode_tab = "\t\t\t";
string sdt_file_path_global = "output_sdtgen_cpp_nlohman_lib_v5.sdt";
int verbose_flag_global = 0;  // 1;

int SdtCpuInstSubNode::total_instances = 0;
int SdtCpuClusterInstSubNode::total_instances = 0;
int SdtMemoryInstSubNode::total_instances = 0;
int SdtSocInstSubNode::total_instances = 0;
// works as well

int main(int, char*[]) {
  ////////////////////////////////////////////////////////////////////////////

  // SdtCpuInstSubNode::total_instances = 0;
  // SdtCpuClusterInstSubNode::total_instances = 0;
  // SdtMemoryInstSubNode::total_instances = 0;
  // SdtSocInstSubNode::total_instances = 0;

  SdtCpusNode cpus_node_obj;

  SdtCpusClusterNode cpus_cluster_node_obj;

  SdtMemoryNode memory_node_obj;

  SdtSocNode soc_node_obj;

  SdtRootMetaDataNode rootmetadata_node_obj;

  int size;

  // read JSON file
  ifstream inputFile;

  // inputFile.open("JSON_Files/GPIO_Yosis_Ver_Example/gold_hier_v5.json");
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

  // get meta-data of all SDT nodes from JSON file
  json data = json::parse(data_file);

  // ************************** Getting SDT NODES!!!!!!!!!!!!!!!!!!!!!!
  // ***************************************

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
  // outfile.close();

  // string buffer_gen_rootmetadata_node;
  // string buffer_gen_cpus_node;
  // string buffer_gen_cpus_cluster_node;
  // string buffer_gen_memory_node;
  // string buffer_gen_soc_node;

  // setting verbose flag to 0 and testing out printing the string returned by
  // gen functions cx Raptor software doesnt allow cout directly

  gen_rootmetadata_node(outfile, rootmetadata_node_obj, verbose_flag_global);
  // buffer_gen_rootmetadata_node =
  // gen_rootmetadata_node(outfile,rootmetadata_node_obj, verbose_flag_global);

  // cout << "gen_rootmetadata_node output = \n\n" <<
  // buffer_gen_rootmetadata_node << endl;

  gen_cpus_node(outfile, cpus_node_obj, verbose_flag_global);
  // buffer_gen_cpus_node = gen_cpus_node(outfile, cpus_node_obj,
  // verbose_flag_global); cout << "gen_cpus_node output = \n\n" <<
  // buffer_gen_cpus_node << endl;

  gen_cpus_cluster_node(outfile, cpus_cluster_node_obj, verbose_flag_global);
  // buffer_gen_cpus_cluster_node = gen_cpus_cluster_node(outfile,
  // cpus_cluster_node_obj, verbose_flag_global); cout << "gen_cpus_cluster_node
  // output = \n\n" << buffer_gen_cpus_cluster_node << endl;

  gen_memory_node(outfile, memory_node_obj, verbose_flag_global);
  // buffer_gen_memory_node = gen_memory_node(outfile, memory_node_obj,
  // verbose_flag_global); cout << "gen_memory_node output = \n\n" <<
  // buffer_gen_memory_node << endl;

  gen_soc_node(outfile, soc_node_obj, verbose_flag_global);
  // buffer_gen_soc_node = gen_soc_node(outfile, soc_node_obj,
  // verbose_flag_global); cout << "gen_soc_node output = \n\n" <<
  // buffer_gen_soc_node << endl;

  outfile << "\n\n};" << endl;

  outfile.flush();

  string string_outfile =
      return_string_from_ofstream_file(outfile, sdt_file_path_global);

  cout << "Priting outfile = \n\n" << string_outfile << endl;

  outfile.close();

  return 0;
}
