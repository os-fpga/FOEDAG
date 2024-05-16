/**
  * @file TelegramBuffer.cpp
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

#include "TelegramHeader.h"

#include <sstream>

#include "ConvertUtils.h"

namespace FOEDAG {

namespace comm {

TelegramHeader::TelegramHeader(uint32_t length, uint32_t checkSum,
                               uint8_t compressorId)
    : m_bodyBytesNum(length),
      m_bodyCheckSum(checkSum),
      m_compressorId(compressorId) {
  m_buffer.resize(TelegramHeader::size());

  // Write signature into a buffer
  std::memcpy(m_buffer.data(), TelegramHeader::SIGNATURE,
              TelegramHeader::SIGNATURE_SIZE);

  // Write the length into the buffer in big-endian byte order
  std::memcpy(m_buffer.data() + TelegramHeader::LENGTH_OFFSET, &length,
              TelegramHeader::LENGTH_SIZE);

  // Write the checksum into the buffer in big-endian byte order
  std::memcpy(m_buffer.data() + TelegramHeader::CHECKSUM_OFFSET, &checkSum,
              TelegramHeader::CHECKSUM_SIZE);

  // Write compressor id
  std::memcpy(m_buffer.data() + TelegramHeader::COMPRESSORID_OFFSET,
              &compressorId, TelegramHeader::COMPRESSORID_SIZE);

  m_isValid = true;
}

TelegramHeader::TelegramHeader(const ByteArray& buffer) {
  m_buffer.resize(TelegramHeader::size());

  bool hasError = false;

  if (buffer.size() >= TelegramHeader::size()) {
    // Check the signature to ensure that this is a valid header
    if (std::memcmp(buffer.data(), TelegramHeader::SIGNATURE,
                    TelegramHeader::SIGNATURE_SIZE)) {
      hasError = true;
    }

    // Read the length from the buffer in big-endian byte order
    std::memcpy(&m_bodyBytesNum, buffer.data() + TelegramHeader::LENGTH_OFFSET,
                TelegramHeader::LENGTH_SIZE);

    // Read the checksum from the buffer in big-endian byte order
    std::memcpy(&m_bodyCheckSum,
                buffer.data() + TelegramHeader::CHECKSUM_OFFSET,
                TelegramHeader::CHECKSUM_SIZE);

    // Read the checksum from the buffer in big-endian byte order
    std::memcpy(&m_compressorId,
                buffer.data() + TelegramHeader::COMPRESSORID_OFFSET,
                TelegramHeader::COMPRESSORID_SIZE);

    if (m_bodyBytesNum == 0) {
      hasError = false;
    }
    if (m_bodyCheckSum == 0) {
      hasError = false;
    }
  }

  if (!hasError) {
    m_isValid = true;
  }
}

std::string TelegramHeader::info() const {
  std::stringstream ss;
  ss << "header" << (m_isValid ? "" : "(INVALID)") << "["
     << "l=" << getPrettySizeStrFromBytesNum(m_bodyBytesNum)
     << "/s=" << m_bodyCheckSum;
  if (m_compressorId) {
    ss << "/c=" << m_compressorId;
  }
  ss << "]";
  return ss.str();
}

}  // namespace comm

}  // namespace FOEDAG
