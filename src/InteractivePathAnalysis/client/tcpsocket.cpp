#include "tcpsocket.h"
#include "keys.h"
#include "../simplelogger.h"

namespace client {

const QString TcpSocket::LOCALHOST_IP_ADDR = "127.0.0.1";

TcpSocket::TcpSocket()
{
    m_connectionWatcher.setInterval(CONNECTION_WATCHER_INTERVAL_MS);
    QObject::connect(&m_connectionWatcher, &QTimer::timeout, this, [this](){
        ensureConnected();
    });

    QObject::connect(&m_socket, &QAbstractSocket::stateChanged, this, &TcpSocket::handleStateChanged);
    QObject::connect(&m_socket, &QIODevice::readyRead, this, &TcpSocket::handleDataReady);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QObject::connect(&m_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &TcpSocket::handleError);
#else
    QObject::connect(&m_socket, &QAbstractSocket::errorOccurred, this, &TcpSocket::handleError);
#endif
}

TcpSocket::~TcpSocket()
{
    m_connectionWatcher.stop();
    m_socket.close();
}

bool TcpSocket::connect()
{
    if (m_portNum == -1) {
        return false;
    }
    m_socket.connectToHost(LOCALHOST_IP_ADDR, m_portNum);
    if (m_socket.waitForConnected(CONNECT_TO_HOST_TIMEOUT_MS)) {
        SimpleLogger::instance().log("connected to host", LOCALHOST_IP_ADDR, m_portNum);
    }
    return isConnected();
}

bool TcpSocket::isConnected() const
{
    return (m_socket.state() == QAbstractSocket::ConnectedState);
}

bool TcpSocket::write(const QByteArray& bytes)
{
    if (ensureConnected()) {
        m_socket.write(bytes);
        return m_socket.waitForBytesWritten();
    } else {
        return false;
    }
}

bool TcpSocket::ensureConnected()
{
    if (!isConnected()) {
        return connect();
    }
    return true;
}

void TcpSocket::setServerIsRunning(bool serverIsRunning)
{
    m_serverIsRunning = serverIsRunning;
    if (serverIsRunning) {
        startConnectionWatcher();
    } else {
        stopConnectionWatcher();
    }
}

void TcpSocket::handleStateChanged(QAbstractSocket::SocketState state)
{
    if ((state == QAbstractSocket::ConnectedState) || (state == QAbstractSocket::ConnectingState)) {
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

void TcpSocket::handleDataReady()
{
    QByteArray bytes = m_socket.readAll();
    m_telegramBuff.append(bytes.constData());

    auto frames = m_telegramBuff.takeFrames();
    for (const ByteArray& frame: frames) {
        QByteArray bytes(reinterpret_cast<const char*>(frame.data().data()), frame.data().size());
        emit dataRecieved(bytes);
    }
}

void TcpSocket::handleError(QAbstractSocket::SocketError error)
{
    m_telegramBuff.clear();
    SimpleLogger::instance().warn("socket error", m_socket.errorString(), error);
}

} // namespace client