/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "Utils/StringUtils.h"

#include <algorithm>
#include <iostream>
#include <locale>
#include <regex>
#include <sstream>

namespace FOEDAG {

std::map<std::string, std::string> StringUtils::envVars;

void StringUtils::tokenize(std::string_view str, std::string_view separator,
                           std::vector<std::string>& result, bool skipEmpty) {
  std::string::size_type pos{0};
  const auto sepSize = separator.size();
  const auto stringSize = str.size();
  std::string tmp;
  std::string::size_type n = str.find(separator, pos);
  while (n != std::string::npos) {
    tmp = str.substr(pos, n - pos);
    if (!(tmp.empty() && skipEmpty)) result.push_back(tmp);
    pos = n + sepSize;
    n = str.find(separator, pos);
  }
  if (pos < stringSize) {  // put last part
    tmp = str.substr(pos, stringSize - pos);
    if (!(tmp.empty() && skipEmpty)) result.push_back(tmp);
  }
}

std::vector<std::string> StringUtils::tokenize(std::string_view str,
                                               std::string_view separator,
                                               bool skipEmpty) {
  std::vector<std::string> result;
  tokenize(str, separator, result, skipEmpty);
  return result;
}

bool StringUtils::contains(const StringVector& strings,
                           const std::string& str) {
  auto end = strings.cend();
  auto it = std::find(strings.cbegin(), end, str);
  return it != end;
}

std::string StringUtils::join(const std::vector<std::string>& strings,
                              const std::string& separator) {
  std::string result;
  for (const auto& s : strings) result += s + separator;
  if (!result.empty())
    for (size_t count = 0; count < separator.size(); count++) result.pop_back();
  return result;
}

std::string& StringUtils::trim(std::string& str) { return ltrim(rtrim(str)); }

std::string& StringUtils::ltrim(std::string& str) {
  auto it2 = std::find_if(str.begin(), str.end(), [](char ch) {
    return !std::isspace<char>(ch, std::locale::classic());
  });
  str.erase(str.begin(), it2);
  return str;
}

std::string& StringUtils::rtrim(std::string& str) {
  auto it1 = std::find_if(str.rbegin(), str.rend(), [](char ch) {
    return !std::isspace<char>(ch, std::locale::classic());
  });
  str.erase(it1.base(), str.end());
  return str;
}

std::string& StringUtils::rtrimEqual(std::string& str) {
  auto it1 = std::find_if(str.rbegin(), str.rend(),
                          [](char ch) { return (ch == '='); });
  if (it1 != str.rend()) str.erase(it1.base() - 1, str.end());
  return str;
}

std::string& StringUtils::rtrim(std::string& str, char c) {
  auto it1 = std::find_if(str.rbegin(), str.rend(),
                          [c](char ch) { return (ch == c); });
  if (it1 != str.rend()) str.erase(it1.base() - 1, str.end());
  return str;
}

std::string& StringUtils::ltrim(std::string& str, char c) {
  auto it1 =
      std::find_if(str.begin(), str.end(), [c](char ch) { return (ch == c); });
  if (it1 != str.end()) str.erase(str.begin(), it1 + 1);
  return str;
}

std::string_view StringUtils::leaf(std::string_view str) {
  const auto found_dot = str.find_last_of('.');
  return found_dot == std::string_view::npos ? str : str.substr(found_dot + 1);
}

std::string StringUtils::replaceAll(std::string_view str, std::string_view from,
                                    std::string_view to) {
  size_t start_pos = 0;
  std::string result(str);
  while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
    result.replace(start_pos, from.length(), to);
    start_pos += to.length();  // Handles case where 'to' is a substr of 'from'
  }
  return result;
}

// Split off the next view split with "separator" character.
// Modifies "src" to contain the remaining string.
// If "src" is exhausted, returned string-view will have data() == nullptr.
static std::string_view SplitNext(std::string_view* src, char separator) {
  if (src->empty()) return {nullptr, 0};  // Done.

  const auto pos = src->find_first_of(separator);
  const auto part_len = (pos != std::string_view::npos) ? pos + 1 : src->size();
  std::string_view result = src->substr(0, part_len);
  src->remove_prefix(part_len);
  return result;
}

std::string_view StringUtils::getLineInString(std::string_view text, int line) {
  if (line < 1) return "";

  std::string_view s;
  while (line && (s = SplitNext(&text, '\n'), s.data()) != nullptr) {
    --line;
  }
  return s;
}

std::vector<std::string_view> StringUtils::splitLines(std::string_view text) {
  std::vector<std::string_view> result;
  std::string_view s;
  while ((s = SplitNext(&text, '\n'), s.data()) != nullptr) {
    result.push_back(s);
  }
  return result;
}

std::string StringUtils::removeComments(std::string_view text) {
  std::string result;
  char c1 = '\0';
  bool inComment = 0;
  for (char c2 : text) {
    if ((c2 == '/') && (c1 == '/')) {
      inComment = true;
      result.erase(result.end() - 1);
    }
    if ((c1 == ' ' || c1 == '\0' || c1 == '\t') && c2 == '#') inComment = true;
    if (c2 == '\n') inComment = false;
    if (!inComment) result += c2;
    c1 = c2;
  }

  return result;
}

// Update the input string.

void StringUtils::autoExpandEnvironmentVariables(std::string* text) {
  static std::regex env(R"(\$\{([^}]+)\})");
  std::smatch match;
  while (std::regex_search(*text, match, env)) {
    std::string var;
    const char* s = getenv(match[1].str().c_str());
    if (s == nullptr) {
      auto itr = envVars.find(match[1].str());
      if (itr != envVars.end()) var = (*itr).second;
    }
    if (var.empty() && s) var = s;
    text->replace(match.position(0), match.length(0), var);
  }
  static std::regex env2("\\$([a-zA-Z0-9_]+)/");
  while (std::regex_search(*text, match, env2)) {
    std::string var;
    const char* s = getenv(match[1].str().c_str());
    if (s == nullptr) {
      auto itr = envVars.find(match[1].str());
      if (itr != envVars.end()) var = (*itr).second;
    }
    if (var.empty() && s) var = s;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
    if (!var.empty() && (var[var.size() - 1] != '\\')) var += "\\";
#else
    if (!var.empty() && (var[var.size() - 1] != '/')) var += "/";
#endif
    text->replace(match.position(0), match.length(0), var);
  }
  static std::regex env3(R"(\$\(([^}]+)\))");
  while (std::regex_search(*text, match, env3)) {
    std::string var;
    const char* s = getenv(match[1].str().c_str());
    if (s == nullptr) {
      auto itr = envVars.find(match[1].str());
      if (itr != envVars.end()) var = (*itr).second;
    }
    if (var.empty() && s) var = s;
    text->replace(match.position(0), match.length(0), var);
  }
}

// Leave input alone and return new string.
std::string StringUtils::evaluateEnvVars(std::string_view text) {
  std::string input(text.begin(), text.end());
  autoExpandEnvironmentVariables(&input);
  return input;
}

std::string StringUtils::unquoted(const std::string& text) {
  if ((text.size() >= 2) && (text.front() == '\"') && (text.back() == '\"')) {
    return text.substr(1, text.length() - 2);
  }
  return text;
}

bool StringUtils::endsWith(const std::string& fullString,
                           const std::string& ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}

bool StringUtils::startsWith(const std::string& text,
                             const std::string& start) {
  if (text.length() >= start.length()) {
    return (0 == text.compare(0, start.length(), start));
  }
  return false;
}

std::string StringUtils::toLower(const std::string& text) {
  auto result = text;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](auto c) { return std::tolower(c); });
  return result;
}

StringVector StringUtils::FromArgs(int argc, const char* argv[]) {
  StringVector res{};
  for (int i = 0; i < argc; i++) res.push_back(std::string{argv[i]});
  return res;
}

std::string StringUtils::toUpper(const std::string& text) {
  auto result = text;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](auto c) { return std::toupper(c); });
  return result;
}

void StringUtils::setArgumentValue(StringVector& stringVector,
                                   const std::string& arg,
                                   const std::string& value) {
  if (!contains(stringVector, arg)) {
    stringVector.push_back(arg);
    stringVector.push_back(value);
    return;
  }
  for (size_t i = 0; (i + 1) < stringVector.size(); i++) {
    if (stringVector.at(i) == arg) {
      stringVector[i + 1] = value;
      return;
    }
  }
}

}  // namespace FOEDAG
