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

#include "nlohmann_json/json.hpp"

#ifndef NETLIST_EDIT_DATA_H
#define NETLIST_EDIT_DATA_H

namespace FOEDAG {

/* This class contains information about the periphery netlist edting step */

class NetlistEditData {
 public:
  NetlistEditData();
  ~NetlistEditData();

  void ReadData(std::filesystem::path configJsonFile,
                std::filesystem::path fabricPortInfo);
  void ResetData();

  const std::map<std::string, std::string>& getInputOutputMap() const {
    return m_input_output_map;
  }
  const std::map<std::string, std::string>& getOutputInputMap() const {
    return m_output_input_map;
  }

  std::string PIO2InnerNet(const std::string& orig);
  std::string InnerNet2PIO(const std::string& orig);

  std::string FindAliasInInputOutputMap(const std::string& orig);

  const std::map<std::string, std::string>& getPrimaryInputMap() const {
    return m_primary_input_map;
  }
  const std::map<std::string, std::string>& getPrimaryOutputMap() const {
    return m_primary_output_map;
  }
  const std::map<std::string, std::string>& getReversePrimaryInputMap() const {
    return m_reverse_primary_input_map;
  }
  const std::map<std::string, std::string>& getReversePrimaryOutputMap() const {
    return m_reverse_primary_output_map;
  }
  const std::set<std::string>& getPIs() const { return m_primary_inputs; }
  const std::set<std::string>& getPOs() const { return m_primary_outputs; }

  bool isPrimaryClock(const std::string& name);
  bool isPllRefClock(const std::string& name);
  bool isGeneratedClock(const std::string& name);
  bool isFabricClock(const std::string& name);

 protected:
  void ComputePrimaryMaps(nlohmann::json& netlist_instances);
  std::set<std::string> m_linked_objects;
  std::set<std::string> m_primary_inputs;
  std::set<std::string> m_primary_outputs;
  std::map<std::string, std::string> m_input_output_map;
  std::map<std::string, std::string> m_output_input_map;
  std::map<std::string, std::string> m_primary_input_map;
  std::map<std::string, std::string> m_primary_output_map;
  std::map<std::string, std::string> m_reverse_primary_input_map;
  std::map<std::string, std::string> m_reverse_primary_output_map;
  std::set<std::string> m_generated_clocks;
  std::set<std::string> m_reference_clocks;
  std::set<std::string> m_primary_generated_clocks;
  std::map<std::string, std::string> m_primary_generated_clocks_map;
  std::map<std::string, std::string> m_reverse_primary_generated_clocks_map;
  std::set<std::string> m_clocks;
  std::set<std::string> m_fabric_clocks;
};

}  // namespace FOEDAG

#endif
