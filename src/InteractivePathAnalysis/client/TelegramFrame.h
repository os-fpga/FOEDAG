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

#ifndef TELEGRAMFRAME_H
#define TELEGRAMFRAME_H

#include <memory>

#include "ByteArray.h"
#include "TelegramHeader.h"

namespace FOEDAG {

namespace comm {

struct TelegramFrame {
  TelegramHeader header;
  ByteArray body;
};
using TelegramFramePtr = std::shared_ptr<TelegramFrame>;

}  // namespace comm

}  // namespace FOEDAG

#endif  // TELEGRAMBUFFER_H
