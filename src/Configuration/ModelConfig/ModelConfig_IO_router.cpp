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

#include "ModelConfig_IO_router.h"

#include "CFGCommon/CFGCommon.h"

namespace FOEDAG {

ModelConfig_IO_MODEL::ModelConfig_IO_MODEL(const std::string& name,
                                           std::vector<std::string> inputs,
                                           const std::string& output)
    : m_name(name), m_inputs(inputs), m_output(output) {}

void ModelConfig_IO_MODEL::assign(const std::string* const_ptr,
                                  const std::string& value) const {
  std::string* ptr = const_cast<std::string*>(const_ptr);
  (*ptr) = value;
}

void ModelConfig_IO_MODEL::set_used(const std::string& used) const {
  assign(&m_used, used);
}
void ModelConfig_IO_MODEL::backup() const { assign(&m_backup_used, m_used); }

void ModelConfig_IO_MODEL::restore() const { assign(&m_used, m_backup_used); }

ModelConfig_IO_ROUTER::ModelConfig_IO_ROUTER(
    std::vector<ModelConfig_IO_PLL> plls,
    std::vector<ModelConfig_IO_FCLK> fclks)
    : m_plls(plls), m_fclks(fclks){};

bool ModelConfig_IO_ROUTER::use_fclk(const std::string& src,
                                     const std::string& output) {
  CFG_ASSERT(src.size());
  CFG_ASSERT(output.size());
  m_msg = "";
  bool status = false;
  for (auto& fclk : m_fclks) {
    if (fclk.m_output == output) {
      if (fclk.m_used.size()) {
        if (fclk.m_used == src) {
          m_msg = CFG_print("Use FCLK: %s", fclk.m_name.c_str());
          status = true;
        } else {
          m_msg =
              CFG_print("Attemp to use FCLK: %s, but it had been used by %s",
                        fclk.m_name.c_str(), fclk.m_used.c_str());
        }
      } else {
        fclk.set_used(src);
        m_msg = CFG_print("Use FCLK: %s", fclk.m_name.c_str());
        status = true;
      }
      break;
    }
  }
  if (!status && m_msg.size() == 0) {
    m_msg = CFG_print("Cannot find FCLK resource: %s", output.c_str());
  }
  return status;
}

std::vector<std::string> ModelConfig_IO_ROUTER::get_fclk_resource(
    const std::string& assigned) {
  std::vector<std::string> resources;
  for (auto& fclk : m_fclks) {
    if (fclk.m_used == assigned) {
      resources.push_back(fclk.m_name);
    }
  }
  return resources;
}

uint32_t ModelConfig_IO_ROUTER::get_pll_availability() {
  uint32_t availability = 0;
  uint32_t index = 0;
  for (auto& pll : m_plls) {
    if (pll.m_used.size() == 0) {
      availability |= (1 << index);
    }
    index++;
  }
  return availability;
}

void ModelConfig_IO_ROUTER::backup() {
  for (auto& pll : m_plls) {
    pll.backup();
  }
  for (auto& fclk : m_fclks) {
    fclk.backup();
  }
}

void ModelConfig_IO_ROUTER::restore() {
  for (auto& pll : m_plls) {
    pll.restore();
  }
  for (auto& fclk : m_fclks) {
    fclk.restore();
  }
}

ModelConfig_IO_62x44_ROUTER::ModelConfig_IO_62x44_ROUTER()
    : ModelConfig_IO_ROUTER(
          {ModelConfig_IO_PLL(
               "pll_0",
               {"hp_io_0_0", "hp_io_0_1", "hp_io_1_0", "hp_io_1_1",
                "hvl_io_0_0", "hvl_io_0_1", "hvl_io_1_0", "hvl_io_1_1", "osc"},
               "pll_clk_0"),
           ModelConfig_IO_PLL(
               "pll_1",
               {"hp_io_0_0", "hp_io_0_1", "hp_io_1_0", "hp_io_1_1",
                "hvr_io_0_0", "hvr_io_0_1", "hvr_io_1_0", "hvr_io_1_1", "osc"},
               "pll_clk_1")},
          {// HP
           ModelConfig_IO_FCLK("hp_fclk_0_A",
                               {"pll_clk_0", "hp_io_0_0", "hp_io_0_1"},
                               "hp_fclk_0_A"),
           ModelConfig_IO_FCLK("hp_fclk_0_B",
                               {"pll_clk_0", "hp_io_0_0", "hp_io_0_1"},
                               "hp_fclk_0_B"),
           ModelConfig_IO_FCLK("hp_fclk_1_A",
                               {"pll_clk_1", "hp_io_1_0", "hp_io_1_1"},
                               "hp_fclk_1_A"),
           ModelConfig_IO_FCLK("hp_fclk_1_B",
                               {"pll_clk_1", "hp_io_1_0", "hp_io_1_1"},
                               "hp_fclk_1_B"),
           // HVL
           ModelConfig_IO_FCLK("hvl_fclk_0_A",
                               {"pll_clk_0", "hvl_io_0_0", "hvl_io_0_1"},
                               "hvl_fclk_0_A"),
           ModelConfig_IO_FCLK("hvl_fclk_0_B",
                               {"pll_clk_0", "hvl_io_0_0", "hvl_io_0_1"},
                               "hvl_fclk_0_B"),
           ModelConfig_IO_FCLK("hvl_fclk_1_A",
                               {"pll_clk_0", "hvl_io_1_0", "hvl_io_1_1"},
                               "hvl_fclk_1_A"),
           ModelConfig_IO_FCLK("hvl_fclk_1_B",
                               {"pll_clk_0", "hvl_io_1_0", "hvl_io_1_1"},
                               "hvl_fclk_1_B"),
           // HVR
           ModelConfig_IO_FCLK("hvr_fclk_0_A",
                               {"pll_clk_1", "hvr_io_0_0", "hvr_io_0_1"},
                               "hvr_fclk_0_A"),
           ModelConfig_IO_FCLK("hvr_fclk_0_B",
                               {"pll_clk_1", "hvr_io_0_0", "hvr_io_0_1"},
                               "hvr_fclk_0_B"),
           ModelConfig_IO_FCLK("hvr_fclk_1_A",
                               {"pll_clk_1", "hvr_io_1_0", "hvr_io_1_1"},
                               "hvr_fclk_1_A"),
           ModelConfig_IO_FCLK("hvr_fclk_1_B",
                               {"pll_clk_1", "hvr_io_1_0", "hvr_io_1_1"},
                               "hvr_fclk_1_B")}) {}

}  // namespace FOEDAG
