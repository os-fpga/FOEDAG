/**
  * @file TcpSocket.cpp
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

#include "TcpSocket.h"

#include "../SimpleLogger.h"

namespace FOEDAG {

namespace client {

TcpSocket::TcpSocket() {
  m_connectionWatcher.setInterval(CONNECTION_WATCHER_INTERVAL_MS);
  QObject::connect(&m_connectionWatcher, &QTimer::timeout, this,
                   [this]() { ensureConnected(); });

  QObject::connect(&m_socket, &QAbstractSocket::stateChanged, this,
                   &TcpSocket::handleStateChanged);
  QObject::connect(&m_socket, &QIODevice::readyRead, this,
                   &TcpSocket::handleDataReady);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
  QObject::connect(
      &m_socket,
      qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this,
      &TcpSocket::handleError);
#else
  QObject::connect(&m_socket, &QAbstractSocket::errorOccurred, this,
                   &TcpSocket::handleError);
#endif
}

TcpSocket::~TcpSocket() {
  m_connectionWatcher.stop();
  m_socket.close();
}

bool TcpSocket::connect() {
  if (m_portNum == -1) {
    return false;
  }
  m_socket.connectToHost(m_addressRotator.address(), m_portNum);
  if (m_socket.waitForConnected(CONNECT_TO_HOST_TIMEOUT_MS)) {
    SimpleLogger::instance().log(
        "connected to host", m_addressRotator.address().toString(), m_portNum);
  } else {
    m_addressRotator.rotate();
  }
  return isConnected();
}

bool TcpSocket::isConnected() const {
  return (m_socket.state() == QAbstractSocket::ConnectedState);
}

bool TcpSocket::write(const QByteArray& bytes) {
  if (ensureConnected()) {
    m_socket.write(bytes);
    return m_socket.waitForBytesWritten();
  } else {
    return false;
  }
}

bool TcpSocket::ensureConnected() {
  if (!isConnected()) {
    return connect();
  }
  return true;
}

void TcpSocket::setServerIsRunning(bool serverIsRunning) {
  m_serverIsRunning = serverIsRunning;
  if (serverIsRunning) {
    startConnectionWatcher();
  } else {
    stopConnectionWatcher();
  }
}

void TcpSocket::handleStateChanged(QAbstractSocket::SocketState state) {
  if ((state == QAbstractSocket::ConnectedState) ||
      (state == QAbstractSocket::ConnectingState)) {
    stopConnectionWatcher();
  } else {
    if (m_serverIsRunning) {
      startConnectionWatcher();
    }
  }

  if (state == QAbstractSocket::ConnectedState) {
    emit connectedChanged(true);
  } else {
    emit connectedChanged(false);
  }
}

void TcpSocket::handleDataReady() {
  QByteArray bytes = m_socket.readAll();
  m_telegramBuff.append(comm::ByteArray{
      bytes.constData(), static_cast<std::size_t>(bytes.size())});

  std::vector<comm::TelegramFramePtr> telegramFrames =
      m_telegramBuff.takeTelegramFrames();
  for (const comm::TelegramFramePtr& telegramFrame : telegramFrames) {
    QByteArray bytes(reinterpret_cast<const char*>(telegramFrame->data.data()),
                     telegramFrame->data.size());
    SimpleLogger::instance().log("received",
                                 telegramFrame->header.info().c_str());
    emit dataRecieved(bytes, telegramFrame->header.isBodyCompressed());
  }

  std::vector<std::string> errors;
  m_telegramBuff.takeErrors(errors);
  for (const std::string& error : errors) {
    SimpleLogger::instance().error(error.c_str());
  }
}

void TcpSocket::handleError(QAbstractSocket::SocketError error) {
  m_telegramBuff.clear();
  SimpleLogger::instance().debug("socket error", m_socket.errorString(), error);
}

}  // namespace client

}  // namespace FOEDAG
