#pragma once

#include "telegrambuffer.h"

#include <QObject>
#include <QByteArray>
#include <QTcpSocket>
#include <QTimer>

class TcpSocket : public QObject {
    Q_OBJECT

    static const int CONNECTION_WATCHER_INTERVAL_MS = 1000;
    static const int CONNECT_TO_HOST_TIMEOUT_MS = 5000;
    static const QString LOCALHOST_IP_ADDR;
    static const int PORT_NUM = 61555;
    static const int CRITICAL_ERRORS_NUM = 5;

public:
    TcpSocket();
    ~TcpSocket();

    void startConnectionWatcher() { m_connectionWatcher.start(); }
    void stopConnectionWatcher() { m_connectionWatcher.stop(); }

    bool connect();
    bool isConnected() const;
    bool write(const QByteArray&);

signals:
    void connectedChanged(bool);
    void dataRecieved(QByteArray);

private:
    QTcpSocket m_socket;
    QTimer m_connectionWatcher;
    TelegramBuffer m_telegramBuff;

    bool ensureConnected();

private slots:
    void handleStateChanged(QAbstractSocket::SocketState);
    void handleDataReady();
    void handleError(QAbstractSocket::SocketError);
};

