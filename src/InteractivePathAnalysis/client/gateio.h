#pragma once

#include "../ncriticalpathparameters.h"
#include "tcpsocket.h"

#include <QObject>

namespace client {

class GateIO : public QObject
{
    Q_OBJECT

public:
    GateIO(const NCriticalPathParametersPtr&);
    ~GateIO();

    bool isConnected() const;
    void startConnectionWatcher();
    void stopConnectionWatcher();
    void setServerIsRunning(bool flag) { m_socket.setServerIsRunning(flag); }

public slots:
    void requestPathHighLight(const QString&, const QString&);
    void requestPathList(const QString&);
    void onHightLightModeChanged();
    void onServerPortDetected(int);

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

} // namespace client