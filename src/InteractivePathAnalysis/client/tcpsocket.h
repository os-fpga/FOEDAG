#pragma once

#include "isocket.h"

#include <QByteArray>
#include <QTcpSocket>
#include <QTimer>

class TcpSocket final: public ISocket {
    static const int CONNECTION_WATCHER_INTERVAL_MS = 1000;
    static const QString LOCALHOST_IP_ADDR;
    static const int PORT_NUM = 61555;

public:
    TcpSocket();
    ~TcpSocket();

    bool connect() override final;
    bool isConnected() override final;
    bool write(const QByteArray&) override final;

private:
    QTcpSocket m_socket;
    QTimer m_connectionWatcher;
    QByteArray m_bytesBuf; // to aggregate chunk of telegrams

    void ensureConnected();
};

