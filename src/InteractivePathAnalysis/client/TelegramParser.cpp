/**
  * @file TelegramParser.cpp
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

#include "TelegramParser.h"

#include "CommConstants.h"
#include "ConvertUtils.h"

namespace FOEDAG {

namespace comm {

bool TelegramParser::tryExtractJsonValueStr(
    const std::string& jsonString, const std::string& key,
    std::optional<std::string>& result) {
  // Find the position of the key
  size_t keyPos = jsonString.find("\"" + key + "\":");

  if (keyPos == std::string::npos) {
    // Key not found
    return false;
  }

  // Find the position of the value after the key
  size_t valuePosStart = jsonString.find(
      "\"", keyPos + key.length() + std::string("\":\"").size());

  if (valuePosStart == std::string::npos) {
    // Value not found
    return false;
  }

  // Find the position of the closing quote for the value
  size_t valueEnd =
      jsonString.find("\"", valuePosStart + std::string("\"").size());

  if (valueEnd == std::string::npos) {
    // Closing quote not found
    return false;
  }

  // Extract the value substring
  result = std::move(
      jsonString.substr(valuePosStart + 1, (valueEnd - valuePosStart) - 1));
  return true;
}

std::optional<int> TelegramParser::tryExtractFieldJobId(
    const std::string& message) {
  std::optional<int> result;
  std::optional<std::string> strOpt;
  if (tryExtractJsonValueStr(message, comm::KEY_JOB_ID, strOpt)) {
    result = tryConvertToInt(strOpt.value());
  }
  return result;
}

std::optional<int> TelegramParser::tryExtractFieldCmd(
    const std::string& message) {
  std::optional<int> result;
  std::optional<std::string> strOpt;
  if (tryExtractJsonValueStr(message, comm::KEY_CMD, strOpt)) {
    result = tryConvertToInt(strOpt.value());
  }
  return result;
}

bool TelegramParser::tryExtractFieldOptions(
    const std::string& message, std::optional<std::string>& result) {
  return tryExtractJsonValueStr(message, comm::KEY_OPTIONS, result);
}

bool TelegramParser::tryExtractFieldData(const std::string& message,
                                         std::optional<std::string>& result) {
  return tryExtractJsonValueStr(message, comm::KEY_DATA, result);
}

std::optional<int> TelegramParser::tryExtractFieldStatus(
    const std::string& message) {
  std::optional<int> result;
  std::optional<std::string> strOpt;
  if (tryExtractJsonValueStr(message, comm::KEY_STATUS, strOpt)) {
    result = tryConvertToInt(strOpt.value());
  }
  return result;
}

}  // namespace comm

}  // namespace FOEDAG
