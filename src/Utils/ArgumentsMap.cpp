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
#include "ArgumentsMap.h"

#include "StringUtils.h"

namespace FOEDAG {

void ArgumentsMap::addArgument(const std::string& key,
                               const std::string& value) {
  m_args.push_back(std::make_pair(key, value));
}

bool ArgumentsMap::hasKey(const std::string& key) const {
  return std::find_if(m_args.values().begin(), m_args.values().end(),
                      [&key](const std::pair<std::string, std::string>& pair) {
                        return pair.first == key;
                      }) != m_args.values().end();
}

std::vector<std::string> ArgumentsMap::keys() const {
  std::vector<std::string> keys;
  for (auto& [key, val] : m_args.values()) keys.emplace_back(key);
  return keys;
}

std::string ArgumentsMap::toString() const {
  StringVector pairs{};
  for (const auto& [key, val] : m_args.values()) {
    if (!val.empty())
      pairs.emplace_back(StringUtils::format("-% %", key, val));
    else
      pairs.emplace_back(StringUtils::format("-%", key));
  }
  return StringUtils::join(pairs, " ");
}

ArgValue ArgumentsMap::value(const std::string& key) const {
  return {hasKey(key), m_args.value(key, std::string{})};
}

ArgValue ArgumentsMap::takeValue(const std::string& key) {
  auto keyFounded = hasKey(key);
  return {keyFounded, m_args.take(key)};
}

ArgumentsMap parseArguments(const std::string& args) {
  ArgumentsMap arguments;
  StringVector words = StringUtils::tokenize(args, " ", true);
  for (size_t i = 0; i < words.size(); i++) {
    if (StringUtils::startsWith(words.at(i), "-")) {
      std::string key = words.at(i).substr(1, words.at(i).size() - 1);
      std::string value{};
      if ((i + 1) < words.size()) {
        if (!StringUtils::startsWith(words.at(i + 1), "-")) {
          value = words.at(i + 1);
          i++;
        }
      }
      arguments.addArgument(key, value);
    }
  }
  return arguments;
}
}  // namespace FOEDAG
