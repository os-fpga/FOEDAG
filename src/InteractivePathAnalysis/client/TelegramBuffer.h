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

#ifndef TELEGRAMBUFFER_H
#define TELEGRAMBUFFER_H

#include <cstring>
#include <memory>
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

  std::size_t findSequence(const char* sequence, std::size_t sequenceSize) {
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
    return std::size_t(-1);
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

class TelegramHeader {
 public:
  static constexpr const char SIGNATURE[] = "IPA";
  static constexpr size_t SIGNATURE_SIZE = sizeof(SIGNATURE);
  static constexpr size_t LENGTH_SIZE = sizeof(uint32_t);
  static constexpr size_t CHECKSUM_SIZE = LENGTH_SIZE;
  static constexpr size_t COMPRESSORID_SIZE = 1;

  static constexpr size_t LENGTH_OFFSET = SIGNATURE_SIZE;
  static constexpr size_t CHECKSUM_OFFSET = LENGTH_OFFSET + LENGTH_SIZE;
  static constexpr size_t COMPRESSORID_OFFSET = CHECKSUM_OFFSET + CHECKSUM_SIZE;

  TelegramHeader() = default;
  explicit TelegramHeader(uint32_t length, uint32_t checkSum,
                          uint8_t compressorId = 0);
  explicit TelegramHeader(const ByteArray& body);
  ~TelegramHeader() = default;

  template <typename T>
  static comm::TelegramHeader constructFromData(const T& body,
                                                uint8_t compressorId = 0) {
    uint32_t bodyCheckSum = comm::ByteArray::calcCheckSum(body);
    return comm::TelegramHeader{static_cast<uint32_t>(body.size()),
                                bodyCheckSum, compressorId};
  }

  static constexpr size_t size() {
    return SIGNATURE_SIZE + LENGTH_SIZE + CHECKSUM_SIZE + COMPRESSORID_SIZE;
  }

  bool isValid() const { return m_isValid; }

  const ByteArray& buffer() const { return m_buffer; }

  uint32_t bodyBytesNum() const { return m_bodyBytesNum; }
  uint32_t bodyCheckSum() const { return m_bodyCheckSum; }
  uint8_t compressorId() const { return m_compressorId; }

  bool isBodyCompressed() const { return m_compressorId != 0; }

  std::string info() const;

 private:
  bool m_isValid = false;
  ByteArray m_buffer;

  uint32_t m_bodyBytesNum = 0;
  uint32_t m_bodyCheckSum = 0;
  uint8_t m_compressorId = 0;
};

struct TelegramFrame {
  TelegramHeader header;
  ByteArray data;
};
using TelegramFramePtr = std::shared_ptr<TelegramFrame>;

/**
 * @brief Implements Telegram Buffer as a wrapper over BytesArray
 *
 * It aggregates received bytes and return only well filled frames, separated by
 * telegram delimerer byte.
 */
class TelegramBuffer {
  static const std::size_t DEFAULT_SIZE_HINT = 1024;

 public:
  TelegramBuffer(std::size_t sizeHint = DEFAULT_SIZE_HINT)
      : m_rawBuffer(sizeHint) {}
  ~TelegramBuffer() = default;

  bool empty() { return m_rawBuffer.empty(); }

  void clear() { m_rawBuffer.clear(); }

  void append(const ByteArray&);
  void takeTelegramFrames(std::vector<TelegramFramePtr>&);
  std::vector<TelegramFramePtr> takeTelegramFrames();
  void takeErrors(std::vector<std::string>&);

  const ByteArray& data() const { return m_rawBuffer; }

 private:
  ByteArray m_rawBuffer;
  std::vector<std::string> m_errors;
  std::optional<TelegramHeader> m_headerOpt;

  bool checkRawBuffer();
};

}  // namespace comm

}  // namespace FOEDAG

#endif  // TELEGRAMBUFFER_H
