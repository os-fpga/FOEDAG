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

#ifndef MODEL_CONFIG_IO_ROUTER_H
#define MODEL_CONFIG_IO_ROUTER_H

#include <Configuration/CFGCommon/CFGCommon.h>

namespace FOEDAG {

struct ModelConfig_IO_MODEL {
  ModelConfig_IO_MODEL(const std::string& name, std::vector<std::string> inputs,
                       const std::string& output);
  void assign(const std::string* const_ptr, const std::string& value) const;
  void set_used(const std::string& used) const;
  void backup() const;
  void restore() const;
  const std::string m_name = "";
  const std::vector<std::string> m_inputs;
  const std::string m_output;
  std::string m_used = "";
  std::string m_backup_used = "";
};

struct ModelConfig_IO_PLL : ModelConfig_IO_MODEL {
  ModelConfig_IO_PLL(const std::string& name, std::vector<std::string> inputs,
                     const std::string& output)
      : ModelConfig_IO_MODEL(name, inputs, output) {}
};

struct ModelConfig_IO_FCLK : ModelConfig_IO_MODEL {
  ModelConfig_IO_FCLK(const std::string& name, std::vector<std::string> inputs,
                      const std::string& output)
      : ModelConfig_IO_MODEL(name, inputs, output) {}
};

struct ModelConfig_IO_ROUTER {
  ModelConfig_IO_ROUTER(std::vector<ModelConfig_IO_PLL> plls,
                        std::vector<ModelConfig_IO_FCLK> fclks);
  bool use_fclk(const std::string& src, const std::string& output);
  std::vector<std::string> get_fclk_resource(const std::string& assigned);
  uint32_t get_pll_availability();
  void backup();
  void restore();
  const std::vector<ModelConfig_IO_PLL> m_plls;
  const std::vector<ModelConfig_IO_FCLK> m_fclks;
  std::string m_msg = "";
};

struct ModelConfig_IO_62x44_ROUTER : ModelConfig_IO_ROUTER {
  ModelConfig_IO_62x44_ROUTER();
};

}  // namespace FOEDAG

#endif
