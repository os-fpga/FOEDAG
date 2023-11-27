#pragma once

#include "../ncriticalpathparameters.h"
#include "tcpsocket.h"

#include <QObject>

class Client : public QObject
{
    Q_OBJECT

public:
    Client(const NCriticalPathParametersPtr&);
    ~Client();

    bool isConnected() const;
    void startConnectionWatcher();
    void stopConnectionWatcher();

public slots:
    void requestPathHighLight(const QString&, const QString&);
    void requestPathList(const QString&);
    void onHightLightModeChanged();

signals:
    void pathListDataReceived(const QString&);
    void highLightModeReceived();
    void connectedChanged(bool);

private:
    NCriticalPathParametersPtr m_parameters;

    QString m_lastPathId;
    TcpSocket m_socket;

    void sendRequest(QByteArray&, const QString&);
    void handleResponse(const QByteArray&);
};

