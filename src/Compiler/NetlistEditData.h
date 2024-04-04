/*
Copyright 2021-2024 The Foedag team

GPL License

Copyright (c) 2021-2024 The Open-Source FPGA Foundation

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

#include <filesystem>
#include <map>
#include <set>
#include <string>

#ifndef NETLIST_EDIT_DATA_H
#define NETLIST_EDIT_DATA_H

namespace FOEDAG {

/* This class contains information about the periphery netlist edting step */

class NetlistEditData {
 public:
  NetlistEditData();
  ~NetlistEditData();

  void ReadData(std::filesystem::path configJsonFile);
  void ResetData();

  std::map<std::string, std::string>& getInputOutputMap() {
    return m_input_output_map;
  }
  std::map<std::string, std::string>& getOutputInputMap() {
    return m_output_input_map;
  }

  std::string PIO2InnerNet(const std::string& orig);
  std::string InnerNet2PIO(const std::string& orig);

  std::string FindAliasInInputOutputMap(const std::string& orig);
  void ComputePrimaryMaps();
  std::map<std::string, std::string>& getPrimaryInputMap() {
    return m_primary_input_map;
  }
  std::map<std::string, std::string>& getPrimaryOutputMap() {
    return m_primary_output_map;
  }
  std::map<std::string, std::string>& getReversePrimaryInputMap() {
    return m_reverse_primary_input_map;
  }
  std::map<std::string, std::string>& getReversePrimaryOutputMap() {
    return m_reverse_primary_output_map;
  }
  std::set<std::string>& getPIs() { return m_primary_inputs; }
  std::set<std::string>& getPOs() { return m_primary_outputs; }

 protected:
  std::set<std::string> m_linked_objects;
  std::set<std::string> m_primary_inputs;
  std::set<std::string> m_primary_outputs;
  std::map<std::string, std::string> m_input_output_map;
  std::map<std::string, std::string> m_output_input_map;
  std::map<std::string, std::string> m_primary_input_map;
  std::map<std::string, std::string> m_primary_output_map;
  std::map<std::string, std::string> m_reverse_primary_input_map;
  std::map<std::string, std::string> m_reverse_primary_output_map;
};

}  // namespace FOEDAG

#endif
