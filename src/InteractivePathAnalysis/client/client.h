#pragma once

#include "../ncriticalpathparameters.h"

#include <QObject>
#ifdef ENABLE_AUTOMATIC_REQUEST
#include <QTimer>
#endif

#include <memory>

class ISocket;

class Client : public QObject
{
    Q_OBJECT

public:
    Client(const NCriticalPathParametersPtr&);
    ~Client();

    bool isConnected() const;

public slots:
    void onPathSelectionChanged(const QString&, const QString&);
    void runGetPathListScenario(const QString&);
    void onHightLightModeChanged();

signals:
    void critPathsDataReady(const QString&);
    void connectedChanged(bool);

private:
    NCriticalPathParametersPtr m_parameters;

    QString m_lastPathId;
    std::unique_ptr<ISocket> m_socket;
#ifdef ENABLE_AUTOMATIC_REQUEST
    QTimer m_timer;
#endif // ENABLE_AUTOMATIC_REQUEST

    void sendRequest(const QByteArray&, const QString&);
    void handleResponse(const QByteArray&);
};

