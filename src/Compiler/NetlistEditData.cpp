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
#include "Compiler/NetlistEditData.h"

#include <fstream>
#include <iostream>
#include <set>

#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"

using namespace FOEDAG;

NetlistEditData::NetlistEditData() {}

NetlistEditData::~NetlistEditData() {}

void NetlistEditData::ReadData(std::filesystem::path configJsonFile) {
  if (FileUtils::FileExists(configJsonFile)) {
    ResetData();
    std::ifstream input;
    input.open(configJsonFile.c_str());
    nlohmann::json netlist_instances = nlohmann::json::parse(input);
    input.close();
    for (auto& instance : netlist_instances["instances"]) {
      if (instance.contains("linked_object")) {
        m_linked_objects.insert(std::string(instance["linked_object"]));
      }
      nlohmann::json connectivity = instance["connectivity"];
      if (connectivity.contains("I") && connectivity.contains("O")) {
        std::string input = std::string(connectivity["I"]);
        std::string output = std::string(connectivity["O"]);
        m_input_output_map.emplace(input, output);
        m_output_input_map.emplace(output, input);
      }
    }
    ComputePrimaryMaps();
  }
}

void NetlistEditData::ResetData() {
  m_input_output_map.clear();
  m_output_input_map.clear();
  m_primary_input_map.clear();
  m_primary_output_map.clear();
  m_reverse_primary_input_map.clear();
  m_reverse_primary_output_map.clear();
}

std::string NetlistEditData::FindAliasInInputOutputMap(
    const std::string& orig) {
  std::string newname = orig;
  std::set<std::string> visited;
  auto itr = m_input_output_map.find(newname);
  if (itr != m_input_output_map.end()) {
    while (true) {
      auto itr = m_input_output_map.find(newname);
      if (itr != m_input_output_map.end()) {
        std::string tmpname = (*itr).second;
        if (visited.find(tmpname) != visited.end()) {
          break;
        } else {
          visited.insert(tmpname);
          newname = tmpname;
        }
      } else {
        break;
      }
    }
  } else {
    while (true) {
      auto itr = m_output_input_map.find(newname);
      if (itr != m_output_input_map.end()) {
        std::string tmpname = (*itr).second;
        if (visited.find(tmpname) != visited.end()) {
          break;
        } else {
          visited.insert(tmpname);
          newname = tmpname;
        }
      } else {
        break;
      }
    }
  }
  return newname;
}

void NetlistEditData::ComputePrimaryMaps() {
  {
    std::set<std::string> outputs;
    for (auto pair : m_input_output_map) {
      outputs.insert(pair.second);
    }
    for (auto pair : m_input_output_map) {
      if (outputs.find(pair.first) == outputs.end()) {
        if (m_linked_objects.find(pair.first) != m_linked_objects.end()) {
          m_primary_inputs.insert(pair.first);
        }
      }
    }
    for (auto pi : m_primary_inputs) {
      m_primary_input_map.emplace(pi, FindAliasInInputOutputMap(pi));
    }
    for (auto pair : m_primary_input_map) {
      m_reverse_primary_input_map.emplace(pair.second, pair.first);
    }
  }
  {
    std::set<std::string> inputs;
    for (auto pair : m_output_input_map) {
      inputs.insert(pair.second);
    }
    for (auto pair : m_output_input_map) {
      if (inputs.find(pair.first) == inputs.end()) {
        if (m_linked_objects.find(pair.first) != m_linked_objects.end()) {
          m_primary_outputs.insert(pair.first);
        }
      }
    }
    for (auto po : m_primary_outputs) {
      m_primary_output_map.emplace(po, FindAliasInInputOutputMap(po));
    }
    for (auto pair : m_primary_output_map) {
      m_reverse_primary_output_map.emplace(pair.second, pair.first);
    }
  }
}

std::string NetlistEditData::PIO2InnerNet(const std::string& orig) {
  std::string result = orig;
  {
    auto itr = m_primary_input_map.find(orig);
    if (itr != m_primary_input_map.end()) {
      const std::string& target = (*itr).second;
      if (target != orig) return target;
    }
  }
  {
    auto itr = m_primary_output_map.find(orig);
    if (itr != m_primary_output_map.end()) {
      const std::string& target = (*itr).second;
      if (target != orig) return target;
    }
  }
  return result;
}

std::string NetlistEditData::InnerNet2PIO(const std::string& orig) {
  std::string result = orig;
  {
    auto itr = m_reverse_primary_input_map.find(orig);
    if (itr != m_reverse_primary_input_map.end()) {
      const std::string& target = (*itr).second;
      if (target != orig) return target;
    }
  }
  {
    auto itr = m_reverse_primary_output_map.find(orig);
    if (itr != m_reverse_primary_output_map.end()) {
      const std::string& target = (*itr).second;
      if (target != orig) return target;
    }
  }
  return result;
}