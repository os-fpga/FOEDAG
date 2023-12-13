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

#include <algorithm>
#include <vector>

namespace FOEDAG {

template <typename Key, typename Value>
class sequential_map {
 public:
  using pair = std::pair<Key, Value>;
  using map = std::vector<pair>;

  sequential_map() = default;

  Value &operator[](const Key &k) {
    for (auto &p : m_data) {
      if (p.first == k) return p.second;
    }
    m_data.push_back(std::make_pair(k, Value{}));
    return m_data.back().second;
  }

  const map &values() const { return m_data; }

  bool empty() const { return m_data.empty(); }

  Value value(const Key &key, const Value &defaultValue = Value{}) const {
    for (const auto &p : m_data) {
      if (p.first == key) return p.second;
    }
    return defaultValue;
  }

  void push_back(const pair &p) {
    // remove previosly added pair with same key, to escape sitation of having
    // multiple keys
    for (auto it = m_data.begin(); it != m_data.end();) {
      if (it->first == p.first) {
        it = m_data.erase(it);
      } else {
        ++it;
      }
    }
    //

    m_data.push_back(p);
  }

  size_t count() const { return m_data.size(); }
  Value take(const Key &key) {
    auto iter = std::find_if(m_data.begin(), m_data.end(),
                             [&key](const pair &p) { return p.first == key; });
    if (iter != m_data.end()) {
      auto returnValue = *iter;
      m_data.erase(iter);
      return returnValue.second;
    }
    return Value{};
  }

 private:
  map m_data;
};

template <typename Key, typename Value>
class sequential_multi_map {
 public:
  using pair = std::pair<Key, Value>;
  using multi_map = std::vector<pair>;
  sequential_multi_map() = default;

  Value &operator[](const Key &k) {
    for (auto &p : m_data) {
      if (p.first == k) return p.second;
    }
    m_data.push_back(std::make_pair(k, Value{}));
    return m_data.back().second;
  }

  const multi_map &values() const { return m_data; }

  bool empty() const { return m_data.empty(); }

  Value value(const Key &key, const Value &defaultValue = Value{}) const {
    for (const auto &p : m_data) {
      if (p.first == key) return p.second;
    }
    return defaultValue;
  }

  void push_back(const pair &p) { m_data.push_back(p); }

  size_t count() const { return m_data.size(); }
  Value take(const Key &key) {
    pair returnValue{};
    auto iter = std::find_if(m_data.begin(), m_data.end(),
                             [&key](const pair &p) { return p.first == key; });
    while (iter != m_data.end()) {
      returnValue = *iter;
      m_data.erase(iter);
      iter = std::find_if(m_data.begin(), m_data.end(),
                          [&key](const pair &p) { return p.first == key; });
    }
    return returnValue.second;
  }

 private:
  multi_map m_data;
};

}  // namespace FOEDAG
