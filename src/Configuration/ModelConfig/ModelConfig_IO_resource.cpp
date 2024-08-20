/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

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

#include "ModelConfig_IO_resource.h"

namespace FOEDAG {

ModelConfig_IO_MODEL::ModelConfig_IO_MODEL(const std::string& name,
                                           const std::string& ric_name,
                                           const std::string& type,
                                           const std::string& subtype,
                                           uint32_t bank)
    : m_name(name),
      m_ric_name(ric_name),
      m_type(type),
      m_subtype(subtype),
      m_bank(bank) {}

void ModelConfig_IO_MODEL::assign(const std::string* const_ptr,
                                  const std::string& value) const {
  std::string* ptr = const_cast<std::string*>(const_ptr);
  (*ptr) = value;
}

void ModelConfig_IO_MODEL::backup() const {
  assign(&m_backup_instantiator, m_instantiator);
}

void ModelConfig_IO_MODEL::restore() const {
  assign(&m_instantiator, m_backup_instantiator);
}

void ModelConfig_IO_MODEL::set_instantiator(
    const std::string& instantiator) const {
  assign(&m_instantiator, instantiator);
}

ModelConfig_IO_RESOURCE::ModelConfig_IO_RESOURCE() {}

ModelConfig_IO_RESOURCE::~ModelConfig_IO_RESOURCE() {
  for (auto& iter : m_resources) {
    while (iter.second->size()) {
      delete iter.second->back();
      iter.second->pop_back();
    }
    delete iter.second;
  }
}

/*
  Add resource, retrieving from the config map
*/
void ModelConfig_IO_RESOURCE::add_resource(const std::string& resource,
                                           const std::string& name,
                                           const std::string& ric_name,
                                           const std::string& type,
                                           const std::string& subtype,
                                           uint32_t bank) {
  if (m_resources.find(resource) == m_resources.end()) {
    m_resources[resource] = new std::vector<const ModelConfig_IO_MODEL*>;
  }
  m_resources[resource]->push_back(
      new ModelConfig_IO_MODEL(name, ric_name, type, subtype, bank));
}

/*
  Get resource count
*/
size_t ModelConfig_IO_RESOURCE::get_resource_count(
    const std::string& resource) {
  CFG_ASSERT(m_resources.find(resource) != m_resources.end());
  return m_resources[resource]->size();
}

/*
  Get index of resource availability
*/
uint64_t ModelConfig_IO_RESOURCE::get_resource_availability_index(
    const std::string& resource) {
  CFG_ASSERT(m_resources.find(resource) != m_resources.end());
  uint64_t availability = 0;
  uint64_t index = 0;
  for (auto& r : *m_resources[resource]) {
    if (r->m_instantiator.size() == 0) {
      availability |= ((uint64_t)(1) << index);
    }
    index++;
  }
  return availability;
}

/*
  Get the resource list that being used by instantiator
*/
std::vector<const ModelConfig_IO_MODEL*>
ModelConfig_IO_RESOURCE::get_used_resource(
    std::vector<const ModelConfig_IO_MODEL*>* models,
    const std::string& instantiator) {
  CFG_ASSERT(instantiator.size());
  std::vector<const ModelConfig_IO_MODEL*> resources;
  for (auto& model : *models) {
    if (model->m_instantiator == instantiator ||
        (model->m_instantiator.size() > 0 && instantiator == "__ALL__")) {
      resources.push_back(model);
    }
  }
  return resources;
}

/*
  Entry function to get the resource list that being used by instantiator
*/
std::vector<const ModelConfig_IO_MODEL*>
ModelConfig_IO_RESOURCE::get_used_resource(const std::string& resource,
                                           const std::string& instantiator) {
  CFG_ASSERT(m_resources.find(resource) != m_resources.end());
  return get_used_resource(m_resources[resource], instantiator);
}

/*
  Try to use the resource
*/
bool ModelConfig_IO_RESOURCE::use_resource(
    std::vector<const ModelConfig_IO_MODEL*>* models,
    const std::string& instantiator, const std::string& name,
    const std::string& type) {
  CFG_ASSERT(instantiator.size());
  CFG_ASSERT(name.size());
  m_msg = "";
  bool status = false;
  for (auto& model : *models) {
    if (model->m_name == name) {
      if (model->m_instantiator.size()) {
        if (model->m_instantiator == instantiator) {
          m_msg = CFG_print("Use %s: %s", type.c_str(), model->m_name.c_str());
          status = true;
        } else {
          m_msg = CFG_print("Attemp to use %s: %s, but it had been used by %s",
                            type.c_str(), model->m_name.c_str(),
                            model->m_instantiator.c_str());
        }
      } else {
        model->set_instantiator(instantiator);
        m_msg = CFG_print("Use %s: %s", type.c_str(), model->m_name.c_str());
        status = true;
      }
      break;
    }
  }
  if (!status && m_msg.size() == 0) {
    m_msg =
        CFG_print("Cannot find %s resource: %s", type.c_str(), name.c_str());
  }
  return status;
}

/*
  Entry function to try to use the resource
*/
bool ModelConfig_IO_RESOURCE::use_resource(const std::string& resource,
                                           const std::string& instantiator,
                                           const std::string& name) {
  CFG_ASSERT(m_resources.find(resource) != m_resources.end());
  std::string r = resource;
  return use_resource(m_resources[resource], instantiator, name,
                      CFG_string_toupper(r));
}

/*
  Entry function to try to use the resource
*/
std::pair<bool, std::string> ModelConfig_IO_RESOURCE::use_root_bank_clkmux(
    const std::string& module, const std::string& location,
    PIN_INFO& pin_info) {
  std::pair<bool, std::string> status;
  if (m_root_bank_clkmuxes.find(pin_info.root_bank_mux_location) !=
      m_root_bank_clkmuxes.end()) {
    status = std::make_pair(
        false,
        CFG_print(
            "%s is already used by %s", pin_info.root_bank_mux_location.c_str(),
            m_root_bank_clkmuxes.at(pin_info.root_bank_mux_location).c_str()));
  } else {
    m_root_bank_clkmuxes[pin_info.root_bank_mux_location] =
        CFG_print("module %s (location: %s)", module.c_str(), location.c_str());
    status = std::make_pair(true, pin_info.root_bank_mux_location);
  }
  return status;
}

/*
  Fail-safe mechanism
*/
void ModelConfig_IO_RESOURCE::backup() {
  for (auto& iter : m_resources) {
    for (auto& item : *(iter.second)) {
      item->backup();
    }
  }
  m_backup_root_bank_clkmuxes = m_root_bank_clkmuxes;
}

/*
  Fail-safe mechanism
*/
void ModelConfig_IO_RESOURCE::restore() {
  for (auto& iter : m_resources) {
    for (auto& item : *(iter.second)) {
      item->restore();
    }
  }
  m_root_bank_clkmuxes = m_backup_root_bank_clkmuxes;
}

}  // namespace FOEDAG
