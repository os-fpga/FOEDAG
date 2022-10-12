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

#include <vector>

namespace FOEDAG {

template <typename Key, typename Value>
class sequential_map {
 public:
  sequential_map() = default;

  Value &operator[](const Key &k) {
    for (auto &p : m_data) {
      if (p.first == k) return p.second;
    }
    m_data.push_back(std::make_pair(k, Value{}));
    return m_data.back().second;
  }

  const std::vector<std::pair<Key, Value>> &values() const { return m_data; }

  bool empty() const { return m_data.empty(); }

  Value value(const Key &key, const Value &defaultValue = Value{}) const {
    for (const auto &p : m_data) {
      if (p.first == key) return p.second;
    }
    return defaultValue;
  }

  void push_back(const std::pair<Key, Value> &p) { m_data.push_back(p); }

 private:
  std::vector<std::pair<Key, Value>> m_data;
};

}  // namespace FOEDAG
