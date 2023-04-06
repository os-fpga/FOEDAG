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

#ifndef FOEDAG_STRINGUTILS_H
#define FOEDAG_STRINGUTILS_H

#include <array>
#include <charconv>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace FOEDAG {

using StringVector = std::vector<std::string>;
class StringUtils final {
 public:
  // Splits the input string with respect to given separator.
  static void tokenize(std::string_view str, std::string_view separator,
                       std::vector<std::string>& result, bool skipEmpty = true);
  static std::vector<std::string> tokenize(std::string_view str,
                                           std::string_view separator,
                                           bool skipEmpty = true);

  // return true if 'strings' contains 'str' otherwise return false
  static bool contains(const StringVector& strings, const std::string& str);

  // join strings with separator
  static std::string join(const std::vector<std::string>& strings,
                          const std::string& separator);

  // Modify string string, remove whitespace at the beginning of the string.
  static std::string& ltrim(std::string& str);

  // Modify string string, remove whitespace at the end of the string.
  static std::string& rtrim(std::string& str);

  // Modify string, removing spaces on both ends.
  static std::string& trim(std::string& str);

  // Erase left of the string until given character is reached. If this
  // is not reached, the string is unchanged. Modifies string.
  // TODO: this name is confusing, as it does not do the same as the other
  // trim functions (which trim characters until there is none)
  static std::string& ltrim(std::string& str, char c);

  // Erase right of the string until given character is reached. If this
  // is not reached, the string is unchanged. Modifies string.
  // TODO: this name is confusing, as it does not do the same as the other
  // trim functions (which trim characters until there is none)
  static std::string& rtrim(std::string& str, char c);

  // Trim and modify string at assignment character.
  static std::string& rtrimEqual(std::string& str);

  // Return the last element of a dot-separated path foo.bar.baz -> baz
  static std::string_view leaf(std::string_view str);

  // In given string "str", replace all occurences of "from" with "to"
  static std::string replaceAll(std::string_view str, std::string_view from,
                                std::string_view to);

  // Given a large input, return the content of line number "line".
  // Lines are 1 indexed.
  static std::string_view getLineInString(std::string_view text, int line);

  // Split input text into lines at '\n'. This separator is included in the
  // returned lines; the last line in text might not have a newline so might
  // not be included.
  static std::vector<std::string_view> splitLines(std::string_view text);

  // Remove '//' and '#'-style end-of-line comments
  static std::string removeComments(std::string_view text);

  // Expand environment variables in the form of ${FOO} or $FOO/
  // (variable followed by slash) in string. Modifies the string.
  static void autoExpandEnvironmentVariables(std::string* text);

  // Like autoExpandEnvironmentVariables(), but returns modified string.
  static std::string evaluateEnvVars(std::string_view text);

  static void registerEnvVar(std::string_view var, std::string_view value) {
    envVars.insert(std::make_pair(var, value));
  }

  static std::string unquoted(const std::string& text);

  // Returns bool indicating if `text` ends with `ending`
  static bool endsWith(const std::string& text, const std::string& ending);

  // Returns bool indicating if `text` starts with `start`
  static bool startsWith(const std::string& text, const std::string& start);

  // Converts the input text to lower case
  static std::string toLower(const std::string& text);
  // Converts the input text to upper case
  static std::string toUpper(const std::string& text);

  // append or modify argument 'arg' with value 'value' inside 'stringVector'
  static void setArgumentValue(StringVector& stringVector,
                               const std::string& arg,
                               const std::string& value);

  // convert string into any numeric value
  template <typename NumberType>
  static std::pair<NumberType, bool> to_number(const std::string& str) {
    static_assert(!std::is_floating_point<NumberType>::value, "Not supported");
    NumberType number;
    [[maybe_unused]] auto [ptr, ec]{
        std::from_chars(str.data(), str.data() + str.size(), number)};
    return std::make_pair(number, ec == std::errc());
  }

  // Convert any kind of numbers
  template <typename NumberType, typename... Args>
  static std::string to_string(NumberType number, Args... format_args) {
    std::ostringstream out;
    if constexpr (std::is_floating_point<NumberType>::value) {
      out.precision(format_args...);
      out << std::fixed << number;
    } else {
      out << number;
    }
    return out.str();
  }

  static StringVector FromArgs(int argc, const char* argv[]);

 private:
  StringUtils() = delete;
  StringUtils(const StringUtils& orig) = delete;
  ~StringUtils() = delete;

  static std::map<std::string, std::string> envVars;
};

}  // namespace FOEDAG

#endif /* FOEDAG_STRINGUTILS_H */
