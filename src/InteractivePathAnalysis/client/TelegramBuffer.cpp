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

#include "TelegramBuffer.h"

// #include "ConvertUtils.h"

namespace FOEDAG {

namespace comm {

void TelegramBuffer::append(const ByteArray& bytes) {
  m_rawBuffer.append(bytes);
}

bool TelegramBuffer::checkRawBuffer() {
  std::size_t signatureStartIndex = m_rawBuffer.findSequence(
      TelegramHeader::SIGNATURE, TelegramHeader::SIGNATURE_SIZE);
  if (signatureStartIndex != std::size_t(-1)) {
    if (signatureStartIndex != 0) {
      m_rawBuffer.erase(m_rawBuffer.begin(),
                        m_rawBuffer.begin() + signatureStartIndex);
    }
    return true;
  }
  return false;
}

void TelegramBuffer::takeTelegramFrames(
    std::vector<comm::TelegramFramePtr>& result) {
  if (m_rawBuffer.size() <= TelegramHeader::size()) {
    return;
  }

  bool mayContainFullTelegram = true;
  while (mayContainFullTelegram) {
    mayContainFullTelegram = false;
    if (!m_headerOpt) {
      if (checkRawBuffer()) {
        TelegramHeader header(m_rawBuffer);
        if (header.isValid()) {
          m_headerOpt = std::move(header);
        }
      }
    }

    if (m_headerOpt) {
      const TelegramHeader& header = m_headerOpt.value();
      std::size_t wholeTelegramSize =
          TelegramHeader::size() + header.bodyBytesNum();
      if (m_rawBuffer.size() >= wholeTelegramSize) {
        ByteArray data(m_rawBuffer.begin() + TelegramHeader::size(),
                       m_rawBuffer.begin() + wholeTelegramSize);
        uint32_t actualCheckSum = data.calcCheckSum();
        if (actualCheckSum == header.bodyCheckSum()) {
          TelegramFramePtr telegramFramePtr = std::make_shared<TelegramFrame>(
              TelegramFrame{header, std::move(data)});
          data.clear();
          result.push_back(telegramFramePtr);
        } else {
          m_errors.push_back("wrong checkSums " +
                             std::to_string(actualCheckSum) + " for " +
                             header.info() + " , drop this chunk");
        }
        m_rawBuffer.erase(m_rawBuffer.begin(),
                          m_rawBuffer.begin() + wholeTelegramSize);
        m_headerOpt.reset();
        mayContainFullTelegram = true;
      }
    }
  }
}

std::vector<comm::TelegramFramePtr> TelegramBuffer::takeTelegramFrames() {
  std::vector<comm::TelegramFramePtr> result;
  takeTelegramFrames(result);
  return result;
}

void TelegramBuffer::takeErrors(std::vector<std::string>& errors) {
  errors.clear();
  std::swap(errors, m_errors);
}

}  // namespace comm

}  // namespace FOEDAG
