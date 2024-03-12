/**
  * @file RequestCreator.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
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

#pragma once

#include <QByteArray>

namespace FOEDAG {

namespace client {

/**
 * @brief Utility class facilitating client request variants to the server for the Interactive Path Analysis tool.
 * 
 * This utility class serves as an interface to manage and generate various client requests 
 * sent to the server within the Interactive Path Analysis tool. It encapsulates the logic 
 * for creating and managing different types of requests that the client sends to the server, 
 * providing a streamlined and organized approach to interact with the server-side functionalities.
 */
class RequestCreator
{
public:
    static RequestCreator& instance();
    ~RequestCreator()=default;

    std::pair<QByteArray, uint8_t> getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, const QString& detailesLevel, bool isFlat);
    std::pair<QByteArray, uint8_t> getDrawPathItemsTelegram(const QString& pathItems, const QString& highLightMode, bool drawPathContour);

    int lastRequestId() const { return m_lastRequestId; }

private:
    RequestCreator()=default;

    int m_lastRequestId = 0;

    int getNextRequestId();

    QByteArray getTelegram(int cmd, const QString& options);
};

} // namespace client

} // namespace FOEDAG