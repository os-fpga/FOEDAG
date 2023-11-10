#include "tcpsocket.h"
#include "keys.h"

TcpSocket::TcpSocket()
{
    m_connectionWatcher.setInterval(1000);
    m_connectionWatcher.start();
    QObject::connect(&m_connectionWatcher, &QTimer::timeout, this, [this](){
        ensureConnected();
    });

    QObject::connect(&m_socket, &QAbstractSocket::stateChanged, this, [this](QAbstractSocket::SocketState state){
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
    });

    QObject::connect(&m_socket, &QIODevice::readyRead, this, [this](){
        QByteArray bytes = m_socket.readAll();
        m_bytesBuf.append(bytes);

        if (bytes.endsWith(END_TELEGRAM_SEQUENCE)) {
            m_bytesBuf.chop(strlen(END_TELEGRAM_SEQUENCE)); // remove end telegram sequence
            emit dataRecieved(m_bytesBuf);
            m_bytesBuf.clear();
        }
    });
}

TcpSocket::~TcpSocket()
{
    m_socket.close();
}

bool TcpSocket::connect()
{
    static QString localIpAddr = "127.0.0.1";
    m_socket.connectToHost(localIpAddr, PORT_NUM);
    if (m_socket.waitForConnected()) {
        qInfo() << "connected to host" << localIpAddr << PORT_NUM;
    } else {
        //qInfo() << "client connection failed!" << m_socket.errorString();
    }

    return isConnected();
}

bool TcpSocket::isConnected()
{
    return (m_socket.state() == QAbstractSocket::ConnectedState);
}

bool TcpSocket::write(const QByteArray& bytes)
{
    ensureConnected();
    m_socket.write(bytes);
    return m_socket.waitForBytesWritten();
}

void TcpSocket::ensureConnected()
{
    if (!isConnected()) {
        connect();
    }
}
