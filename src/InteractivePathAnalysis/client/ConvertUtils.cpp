/**
  * @file ConvertUtils.cpp
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

#include "ConvertUtils.h"

#include <iomanip>
#include <sstream>

namespace FOEDAG {

std::optional<int> tryConvertToInt(const std::string& str) {
  std::optional<int> result;

  std::istringstream iss(str);
  int intValue;
  if (iss >> intValue) {
    if (iss.eof()) {
      result = intValue;
    }
  }
  return result;
}

namespace {
std::string getPrettyStrFromFloat(float value) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(2)
     << value;  // Set precision to 2 digit after the decimal point
  return ss.str();
}
}  // namespace

std::string getPrettyDurationStrFromMs(int64_t durationMs) {
  std::string result;
  if (durationMs >= 1000) {
    result = getPrettyStrFromFloat(durationMs / 1000.0f) + " sec";
  } else {
    result = std::to_string(durationMs);
    result += " ms";
  }
  return result;
}

std::string getPrettySizeStrFromBytesNum(int64_t bytesNum) {
  std::string result;
  if (bytesNum >= 1024 * 1024 * 1024) {
    result = getPrettyStrFromFloat(bytesNum / float(1024 * 1024 * 1024)) + "Gb";
  } else if (bytesNum >= 1024 * 1024) {
    result = getPrettyStrFromFloat(bytesNum / float(1024 * 1024)) + "Mb";
  } else if (bytesNum >= 1024) {
    result = getPrettyStrFromFloat(bytesNum / float(1024)) + "Kb";
  } else {
    result = std::to_string(bytesNum) + "bytes";
  }
  return result;
}

std::string getTruncatedMiddleStr(const std::string& src, std::size_t num) {
  std::string result;
  static std::size_t minimalStringSizeToTruncate = 20;
  if (num < minimalStringSizeToTruncate) {
    num = minimalStringSizeToTruncate;
  }
  static std::string middlePlaceHolder("...");
  const std::size_t srcSize = src.size();
  if (srcSize > num) {
    int prefixNum = num / 2;
    int suffixNum = num / 2 - middlePlaceHolder.size();
    result.append(std::move(src.substr(0, prefixNum)));
    result.append(middlePlaceHolder);
    result.append(std::move(src.substr(srcSize - suffixNum)));
  } else {
    result = src;
  }

  return result;
}

}  // namespace FOEDAG
