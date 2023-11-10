#include "client.h"
#include "clienttoolswidget.h"
#include "keys.h"

#include "tcpsocket.h"
#include "requestcreator.h"
#include "ncriticalpathsettings.h"

#include <QCoreApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

Client::Client(
#ifndef STANDALONE_APP
    FOEDAG::Compiler* compiler
#endif
    )
{
    m_socket = std::make_unique<TcpSocket>();

    /// UI
    m_toolsWidget = new ClientToolsWidget(
#ifndef STANDALONE_APP
        compiler
#endif
        );

    connect(m_toolsWidget, &ClientToolsWidget::getPathListRequested, this, &Client::runGetPathListScenario);

    connect(m_toolsWidget, &ClientToolsWidget::highLightModeChanged, this, [this](){
        if (!m_lastSelectedPathId.isEmpty()) {
            onPathSelected(m_lastSelectedPathId, "hight light mode change");
        }
    });
    //

    connect(m_socket.get(), &ISocket::connectedChanged, m_toolsWidget, &ClientToolsWidget::onConnectionStatusChanged);
    connect(m_socket.get(), &ISocket::dataRecieved, this, &Client::handleResponse);
    ///

#ifdef ENABLE_AUTOMATIC_REQUEST
    m_timer.setInterval(AUTOMATIC_CLIENT_REQUEST_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, this, &Client::runGetPathListScenario);
    m_timer.start();
#endif // ENABLE_AUTOMATIC_REQUEST
}

Client::~Client()
{
    if (!m_toolsWidget->parent()) {
        delete m_toolsWidget;
    }
}

bool Client::isConnected() const
{
    return m_socket->isConnected();
}

void Client::handleResponse(const QByteArray& bytes)
{
    qDebug() << "from server:" << bytes;

    // Convert the QByteArray to a QJsonDocument
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Error parsing JSON:" << parseError.errorString();
        return;
    }

    // Extract the QJsonObject from the QJsonDocument
    QJsonObject jsonObject = jsonDoc.object();

    int cmd = jsonObject[KEY_CMD].toString().toInt();
    bool status = jsonObject[KEY_STATUS].toString().toInt();
    QString data = jsonObject[KEY_DATA].toString();

    qInfo() << cmd << status << data;
    if (status) {
        switch(cmd) {
        case CMD_GET_PATH_LIST_ID: emit critPathsDataReady(data); break;
        }
    } else {
        qInfo() << "unable to perform cmd on server, error" << data;
    }
}

void Client::sendRequest(const QByteArray& requestBytes, const QString& initiator)
{
    if (!m_socket->isConnected()) {
        m_socket->connect();
    }
    qDebug() << "sending:" << requestBytes << QString("requested by [%1]").arg(initiator);
    if (!m_socket->write(requestBytes)) {
        qCritical() << "unable to send" << requestBytes;
    }
}

void Client::runGetPathListScenario(const QString& initiator)
{
    QByteArray bytes = RequestCreator::instance().getPathListRequestTelegram(m_toolsWidget->nCriticalPathNum(),
                                                                             m_toolsWidget->pathType(),
                                                                             m_toolsWidget->detailesLevel(),
                                                                             m_toolsWidget->isFlatRouting());
    sendRequest(bytes, initiator);
}

void Client::onPathSelected(const QString& pathId, const QString& initiator)
{
    m_lastSelectedPathId = pathId;
    QByteArray bytes = RequestCreator::instance().getDrawPathIdTelegram(pathId, m_toolsWidget->highlightMode());
    sendRequest(bytes, initiator);
}
