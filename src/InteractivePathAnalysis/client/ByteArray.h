/**
  * @file TelegramBuffer.h
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

#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace FOEDAG {

namespace comm {

/**
 * @brief ByteArray as a simple wrapper over std::vector<uint8_t>
 */
class ByteArray : public std::vector<uint8_t> {
 public:
  static const std::size_t DEFAULT_SIZE_HINT = 1024;

  ByteArray(const char* data)
      : std::vector<uint8_t>(
            reinterpret_cast<const uint8_t*>(data),
            reinterpret_cast<const uint8_t*>(data + std::strlen(data))) {}

  ByteArray(const char* data, std::size_t size)
      : std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                             reinterpret_cast<const uint8_t*>(data + size)) {}

  ByteArray(std::size_t sizeHint = DEFAULT_SIZE_HINT) { reserve(sizeHint); }

  template <typename Iterator>
  ByteArray(Iterator first, Iterator last)
      : std::vector<uint8_t>(first, last) {}

  void append(const ByteArray& appendix) {
    insert(end(), appendix.begin(), appendix.end());
  }

  void append(uint8_t b) { push_back(b); }

  std::optional<std::size_t> findSequence(const char* sequence,
                                          std::size_t sequenceSize) {
    const std::size_t mSize = size();
    if (mSize >= sequenceSize) {
      for (std::size_t i = 0; i <= mSize - sequenceSize; ++i) {
        bool found = true;
        for (std::size_t j = 0; j < sequenceSize; ++j) {
          if (at(i + j) != sequence[j]) {
            found = false;
            break;
          }
        }
        if (found) {
          return i;
        }
      }
    }
    return std::nullopt;
  }

  std::string to_string() const {
    return std::string(reinterpret_cast<const char*>(this->data()),
                       this->size());
  }

  uint32_t calcCheckSum() { return calcCheckSum<ByteArray>(*this); }

  template <typename T>
  static uint32_t calcCheckSum(const T& iterable) {
    uint32_t sum = 0;
    for (uint8_t c : iterable) {
      sum += static_cast<unsigned int>(c);
    }
    return sum;
  }
};

}  // namespace comm

}  // namespace FOEDAG

#endif  // BYTEARRAY_H
