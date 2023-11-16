#include "tcpsocket.h"
#include "keys.h"

const QString TcpSocket::LOCALHOST_IP_ADDR = "127.0.0.1";

TcpSocket::TcpSocket()
{
    m_connectionWatcher.setInterval(CONNECTION_WATCHER_INTERVAL_MS);
    QObject::connect(&m_connectionWatcher, &QTimer::timeout, this, [this](){
        ensureConnected();
    });

    QObject::connect(&m_socket, &QAbstractSocket::stateChanged, this, &TcpSocket::handleStateChanged);
    QObject::connect(&m_socket, &QIODevice::readyRead, this, &TcpSocket::handleDataReady);
    QObject::connect(&m_socket, &QTcpSocket::errorOccurred, this, &TcpSocket::handleSocketError);
}

TcpSocket::~TcpSocket()
{
    m_socket.close();
}

bool TcpSocket::connect()
{
    m_socket.connectToHost(LOCALHOST_IP_ADDR, PORT_NUM);
    if (m_socket.waitForConnected(CONNECT_TO_HOST_TIMEOUT_MS)) {
        qDebug() << "connected to host" << LOCALHOST_IP_ADDR << PORT_NUM;
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

void TcpSocket::handleStateChanged(QAbstractSocket::SocketState state)
{
    if ((state == QAbstractSocket::ConnectedState) || (state == QAbstractSocket::ConnectingState)) {
        m_connectionWatcher.stop();
    } else {
        m_connectionWatcher.start();
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
    m_bytesBuf.append(bytes);

    if (bytes.endsWith(END_TELEGRAM_SEQUENCE)) {
        m_bytesBuf.chop(strlen(END_TELEGRAM_SEQUENCE)); // remove end telegram sequence
        emit dataRecieved(m_bytesBuf);
        m_bytesBuf.clear();
    }
}

void TcpSocket::handleSocketError(QAbstractSocket::SocketError)
{
    m_bytesBuf.clear();
    qDebug() << m_socket.errorString();
}


