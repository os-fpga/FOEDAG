#pragma once

#include <QWidget>
#include <QObject>
#include <QTimer>

#include <memory>

class ClientToolsWidget;
class ISocket;

#ifndef STANDALONE_APP
#include "../../Compiler/Compiler.h"
#endif

class Client : public QObject
{
    Q_OBJECT
public:
    Client(
#ifndef STANDALONE_APP
        FOEDAG::Compiler*
#endif
        );
    ~Client();

    ClientToolsWidget* toolsWidget() const { return m_toolsWidget; }

    bool isConnected() const;

public slots:
    void onPathSelected(const QString&, const QString&);

signals:
    void critPathsDataReady(const QString&);

private:
    ClientToolsWidget* m_toolsWidget = nullptr;
    std::unique_ptr<ISocket> m_socket;
#ifdef ENABLE_AUTOMATIC_REQUEST
    QTimer m_timer;
#endif // ENABLE_AUTOMATIC_REQUEST

    void sendRequest(const QByteArray&, const QString&);
    void handleResponse(const QByteArray&);

private slots:
    void runGetPathListScenario(const QString&);
};

