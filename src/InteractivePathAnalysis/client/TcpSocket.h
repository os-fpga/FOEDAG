/**
  * @file TcpSocket.h
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

#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include "TelegramBuffer.h"

namespace FOEDAG {

namespace client {

/**
 * @brief Low-level communication layer implementing client functionality.
 *
 * This class acts as a wrapper around QTcpSocket, providing a foundational
 * interface for handling client-side communication. It manages the underlying
 * TCP socket functionality and facilitates communication with the server.
 * It serves as a fundamental building block for establishing and managing
 * connections between the client and the server, enabling bidirectional
 * data transmission and interaction.
 */
class TcpSocket : public QObject {
  Q_OBJECT

  static const int CONNECTION_WATCHER_INTERVAL_MS = 1000;
  static const int CONNECT_TO_HOST_TIMEOUT_MS = 5000;
  static const int CRITICAL_ERRORS_NUM = 5;

  class AddressRotator {
   public:
    AddressRotator()
        : m_variants{QHostAddress{QHostAddress::LocalHostIPv6},
                     QHostAddress{QHostAddress::LocalHost}} {}
    void rotate() {
      if (m_currentIndex < m_variants.size() - 1) {
        m_currentIndex++;
      } else {
        m_currentIndex = 0;
      }
    }

    const QHostAddress& address() { return m_variants.at(m_currentIndex); }

   private:
    std::size_t m_currentIndex = 0;
    std::vector<QHostAddress> m_variants;
  };

 public:
  TcpSocket();
  ~TcpSocket();

  void setPortNum(int portNum) { m_portNum = portNum; }
  void setServerIsRunning(bool serverIsRunning);

  void startConnectionWatcher() { m_connectionWatcher.start(); }
  void stopConnectionWatcher() { m_connectionWatcher.stop(); }

  bool connect();
  bool isConnected() const;
  bool write(const QByteArray&);

 signals:
  void connectedChanged(bool);
  void dataRecieved(QByteArray, bool isCompressed);

 private:
  int m_portNum = -1;
  AddressRotator m_addressRotator;
  QTcpSocket m_socket;
  bool m_serverIsRunning = false;
  QTimer m_connectionWatcher;
  comm::TelegramBuffer m_telegramBuff;

  bool ensureConnected();

 private slots:
  void handleStateChanged(QAbstractSocket::SocketState);
  void handleDataReady();
  void handleError(QAbstractSocket::SocketError);
};

}  // namespace client

}  // namespace FOEDAG
