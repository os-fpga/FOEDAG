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

#ifndef MODEL_CONFIG_BITSTREAM_SETTING_XML_H
#define MODEL_CONFIG_BITSTREAM_SETTING_XML_H

#include <Configuration/CFGCommon/CFGCommon.h>

#include <map>
#include <string>
#include <vector>

namespace FOEDAG {

class ModelConfig_BITSREAM_SETTINGS_XML {
 public:
  static void gen(const std::vector<std::string>& flag_options,
                  const std::map<std::string, std::string>& options,
                  const std::string& input, const std::string& output);

 private:
};

}  // namespace FOEDAG

#endif
