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

#include "CFGCommon/CFGCommon.h"

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

ModelConfig_IO_RESOURCE::ModelConfig_IO_RESOURCE(
    std::vector<ModelConfig_IO_PLL> plls,
    std::vector<ModelConfig_IO_FCLK> fclks)
    : m_plls(plls), m_fclks(fclks){};

bool ModelConfig_IO_RESOURCE::use_resource(
    std::vector<const ModelConfig_IO_MODEL*>& models,
    const std::string& instantiator, const std::string& name,
    const std::string& type) {
  CFG_ASSERT(instantiator.size());
  CFG_ASSERT(name.size());
  m_msg = "";
  bool status = false;
  for (auto& model : models) {
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

bool ModelConfig_IO_RESOURCE::use_fclk(const std::string& instantiator,
                                       const std::string& name) {
  std::vector<const ModelConfig_IO_MODEL*> models;
  for (auto& fclk : m_fclks) {
    models.push_back(&fclk);
  }
  return use_resource(models, instantiator, name, "FCLK");
}

bool ModelConfig_IO_RESOURCE::use_pll(const std::string& instantiator,
                                      const std::string& name) {
  std::vector<const ModelConfig_IO_MODEL*> models;
  for (auto& pll : m_plls) {
    models.push_back(&pll);
  }
  return use_resource(models, instantiator, name, "PLL");
}

uint32_t ModelConfig_IO_RESOURCE::fclk_use_pll(const std::string& fclk) {
  uint32_t pll_resource = 0;
  if (fclk.find("hvl_fclk_") == 0) {
    pll_resource = 0;
  } else if (fclk.find("hvr_fclk_") == 0) {
    pll_resource = 1;
  } else if (fclk.find("hp_fclk_0") == 0) {
    pll_resource = 0;
  } else if (fclk.find("hp_fclk_1") == 0) {
    pll_resource = 1;
  } else {
    CFG_INTERNAL_ERROR("Unknown FCLK %s", fclk.c_str());
  }
  return pll_resource;
}

std::vector<const ModelConfig_IO_MODEL*>
ModelConfig_IO_RESOURCE::get_used_resource(
    std::vector<const ModelConfig_IO_MODEL*>& models,
    const std::string& instantiator) {
  CFG_ASSERT(instantiator.size());
  std::vector<const ModelConfig_IO_MODEL*> resources;
  for (auto& model : models) {
    if (model->m_instantiator == instantiator ||
        (model->m_instantiator.size() > 0 && instantiator == "__ALL__")) {
      resources.push_back(model);
    }
  }
  return resources;
}

std::vector<const ModelConfig_IO_MODEL*> ModelConfig_IO_RESOURCE::get_used_fclk(
    const std::string& instantiator) {
  std::vector<const ModelConfig_IO_MODEL*> models;
  for (auto& fclk : m_fclks) {
    models.push_back(&fclk);
  }
  return get_used_resource(models, instantiator);
}

std::vector<const ModelConfig_IO_MODEL*> ModelConfig_IO_RESOURCE::get_used_pll(
    const std::string& instantiator) {
  std::vector<const ModelConfig_IO_MODEL*> models;
  for (auto& pll : m_plls) {
    models.push_back(&pll);
  }
  return get_used_resource(models, instantiator);
}

uint32_t ModelConfig_IO_RESOURCE::get_pll_availability() {
  uint32_t availability = 0;
  uint32_t index = 0;
  for (auto& pll : m_plls) {
    if (pll.m_instantiator.size() == 0) {
      availability |= (1 << index);
    }
    index++;
  }
  return availability;
}

void ModelConfig_IO_RESOURCE::backup() {
  for (auto& pll : m_plls) {
    pll.backup();
  }
  for (auto& fclk : m_fclks) {
    fclk.backup();
  }
}

void ModelConfig_IO_RESOURCE::restore() {
  for (auto& pll : m_plls) {
    pll.restore();
  }
  for (auto& fclk : m_fclks) {
    fclk.restore();
  }
}

ModelConfig_IO_62x44_RESOURCE::ModelConfig_IO_62x44_RESOURCE()
    : ModelConfig_IO_RESOURCE(
          {ModelConfig_IO_PLL("pll_0",
                              "u_GBOX_HP_40X2.u_gbox_PLLTS16FFCFRACF_0", "HP",
                              "hp", 0),
           ModelConfig_IO_PLL("pll_1",
                              "u_GBOX_HP_40X2.u_gbox_PLLTS16FFCFRACF_1", "HP",
                              "hp", 1)},
          {// HP
           ModelConfig_IO_FCLK("hp_fclk_0_A",
                               "u_GBOX_HP_40X2.u_gbox_fclk_mux_all", "HP", "hp",
                               0),
           ModelConfig_IO_FCLK("hp_fclk_0_B",
                               "u_GBOX_HP_40X2.u_gbox_fclk_mux_all", "HP", "hp",
                               0),
           ModelConfig_IO_FCLK("hp_fclk_1_A",
                               "u_GBOX_HP_40X2.u_gbox_fclk_mux_all", "HP", "hp",
                               1),
           ModelConfig_IO_FCLK("hp_fclk_1_B",
                               "u_GBOX_HP_40X2.u_gbox_fclk_mux_all", "HP", "hp",
                               1),
           // HVL
           ModelConfig_IO_FCLK("hvl_fclk_0_A",
                               "u_GBOX_HV_40X2_VL.u_gbox_fclk_mux_all", "HV",
                               "hvl", 0),
           ModelConfig_IO_FCLK("hvl_fclk_0_B",
                               "u_GBOX_HV_40X2_VL.u_gbox_fclk_mux_all", "HV",
                               "hvl", 0),
           ModelConfig_IO_FCLK("hvl_fclk_1_A",
                               "u_GBOX_HV_40X2_VL.u_gbox_fclk_mux_all", "HV",
                               "hvl", 1),
           ModelConfig_IO_FCLK("hvl_fclk_1_B",
                               "u_GBOX_HV_40X2_VL.u_gbox_fclk_mux_all", "HV",
                               "hvl", 1),
           // HVR
           ModelConfig_IO_FCLK("hvr_fclk_0_A",
                               "u_GBOX_HV_40X2_VR.u_gbox_fclk_mux_all", "HV",
                               "hvr", 0),
           ModelConfig_IO_FCLK("hvr_fclk_0_B",
                               "u_GBOX_HV_40X2_VR.u_gbox_fclk_mux_all", "HV",
                               "hvr", 0),
           ModelConfig_IO_FCLK("hvr_fclk_1_A",
                               "u_GBOX_HV_40X2_VR.u_gbox_fclk_mux_all", "HV",
                               "hvr", 1),
           ModelConfig_IO_FCLK("hvr_fclk_1_B",
                               "u_GBOX_HV_40X2_VR.u_gbox_fclk_mux_all", "HV",
                               "hvr", 1)}) {}

PIN_INFO::PIN_INFO(std::string name) {
  // Pin name have been validated
  // Whenever this is called, the name should be valid
  // Do something hardcoded, can study how to use regex
  if (name.find("BOOT_CLOCK#") == 0) {
    type = "BOOT_CLOCK";
    rtl_name = "boot_clock";
    name = name.erase(0, 11);
    bank = (uint32_t)(CFG_convert_string_to_u64(name));
  } else {
    if (name.find("HP_1_") == 0) {
      type = "HP";
      rtl_name = "hp_0";
      bank = 0;
    } else if (name.find("HP_2_") == 0) {
      type = "HP";
      rtl_name = "hp_1";
      bank = 1;
    } else if (name.find("HR_1_") == 0) {
      type = "HVL";
      rtl_name = "hv_vl_0";
      bank = 0;
    } else if (name.find("HR_2_") == 0) {
      type = "HVL";
      rtl_name = "hv_vl_1";
      bank = 1;
    } else if (name.find("HR_3_") == 0) {
      type = "HVR";
      rtl_name = "hv_vr_0";
      bank = 0;
    } else if (name.find("HR_5_") == 0) {
      type = "HVR";
      rtl_name = "hv_vr_1";
      bank = 1;
    } else {
      CFG_INTERNAL_ERROR("Unknown location name \"%s\"", name.c_str());
    }
    name = name.erase(0, 5);
    if (name.find("CC_") == 0) {
      is_clock = true;
      name = name.erase(0, 3);
    }
    size_t temp = name.find("_");
    CFG_ASSERT(temp != 0 && temp != std::string::npos);
    name = name.substr(0, temp);
    index = (uint32_t)(CFG_convert_string_to_u64(name));
    pair_index = index / 2;
    rx_io = (pair_index < 10) ? 0 : 1;
  }
}

}  // namespace FOEDAG
