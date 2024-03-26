/**
  * @file ServerFreePortDetector.h
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

#pragma once

#include <QTcpServer>

#include "../SimpleLogger.h"

namespace FOEDAG {

namespace client {

/**
 * @brief Utility class for finding an available free port number.
 *
 * This class serves as a simple utility to detect an unused port number
 * that can be utilized for running a server or other networking tasks.
 * It provides methods to identify and return an available port within
 * range 60555-65535.
 */
class ServerFreePortDetector {
  const int PORT_NUM_MAX = 65535;
  const int PORT_NUM_START = 60555;

 public:
  int detectAvailablePortNum() {
    while (isBusy(m_portNum) && (m_portNum < PORT_NUM_MAX)) {
      m_portNum++;
    }
    return m_portNum;
  }

 private:
  int m_portNum = PORT_NUM_START;

  bool isBusy(int portNum) {
    SimpleLogger::instance().log("test server portNum", portNum);
    QTcpServer server;
    if (server.listen(QHostAddress::LocalHost, portNum)) {
      server.close();
      SimpleLogger::instance().log("found free server portNum", portNum);
      return false;
    } else {
      SimpleLogger::instance().log("portNum", portNum, "is busy");
      return true;
    }
  }
};

}  // namespace client

}  // namespace FOEDAG