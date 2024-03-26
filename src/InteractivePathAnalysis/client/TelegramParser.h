/**
  * @file TelegramParser.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#include <optional>
#include <string>

namespace FOEDAG {

namespace comm {

/**
 * @brief Dummy JSON parser using regular expressions.
 *
 * This module provides helper methods to extract values for a keys as "JOB_ID",
 * "CMD", or "OPTIONS" from a JSON schema structured as follows: {JOB_ID:num,
 * CMD:enum, OPTIONS:string}.
 */
class TelegramParser {
 public:
  static std::optional<int> tryExtractFieldJobId(const std::string& message);
  static std::optional<int> tryExtractFieldCmd(const std::string& message);
  static bool tryExtractFieldOptions(const std::string& message,
                                     std::optional<std::string>& result);
  static bool tryExtractFieldData(const std::string& message,
                                  std::optional<std::string>& result);
  static std::optional<int> tryExtractFieldStatus(const std::string& message);

 private:
  static bool tryExtractJsonValueStr(const std::string& jsonString,
                                     const std::string& key,
                                     std::optional<std::string>& result);
};

}  // namespace comm

}  // namespace FOEDAG

#endif  // TELEGRAMPARSER_H
