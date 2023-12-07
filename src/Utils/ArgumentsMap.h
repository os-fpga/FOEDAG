/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#pragma once

#include <string>

#include "Utils/sequential_map.h"

namespace FOEDAG {

struct ArgValue {
  bool exist;
  std::string value;
  explicit operator bool() const { return exist; }
  operator std::string() const { return value; }
  ArgValue(const std::string &str) : exist(true), value(str) {}
  ArgValue(const char *str) : exist(true), value(std::string{str}) {}
  ArgValue(bool ex, const std::string &str) : exist(ex), value(str) {}

  bool operator==(const ArgValue &other) const {
    return (exist == other.exist) && (value == other.value);
  }
};

class ArgumentsMap {
 public:
  void addArgument(const std::string &key, const std::string &value = {});
  ArgValue value(const std::string &key) const;
  ArgValue takeValue(const std::string &key);
  bool hasKey(const std::string &key) const;
  std::vector<std::string> keys() const;

  std::string toString() const;

 private:
  sequential_map<std::string, std::string> m_args{};
};

ArgumentsMap parseArguments(const std::string &args);

};  // namespace FOEDAG
