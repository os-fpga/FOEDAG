#pragma once

#include "isocket.h"

#include <QByteArray>
#include <QTcpSocket>
#include <QTimer>

#ifndef PORT_NUM
#define PORT_NUM 61555
#endif

class TcpSocket final: public ISocket {
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

