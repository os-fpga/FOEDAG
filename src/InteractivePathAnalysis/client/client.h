#pragma once

#include "../ncriticalpathparameters.h"
#include "tcpsocket.h"

#include <QObject>
#ifdef ENABLE_AUTOMATIC_REQUEST
#include <QTimer>
#endif


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
    void critPathsDataReady(const QString&);
    void connectedChanged(bool);

private:
    NCriticalPathParametersPtr m_parameters;

    QString m_lastPathId;
    TcpSocket m_socket;
#ifdef ENABLE_AUTOMATIC_REQUEST
    QTimer m_timer;
#endif // ENABLE_AUTOMATIC_REQUEST

    void sendRequest(const QByteArray&, const QString&);
    void handleResponse(const QByteArray&);
};

