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

#include <optional>
#include <string>
#include <vector>

#include "ByteArray.h"
#include "TelegramFrame.h"

namespace FOEDAG {

namespace comm {

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
