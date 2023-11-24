#include "client.h"
#include "keys.h"

#include "tcpsocket.h"
#include "requestcreator.h"
#include "../simplelogger.h"

#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

Client::Client(const NCriticalPathParametersPtr& parameters)
    : m_parameters(parameters)
{
    connect(&m_socket, &TcpSocket::connectedChanged, this, &Client::connectedChanged);
    connect(&m_socket, &TcpSocket::dataRecieved, this, &Client::handleResponse);

#ifdef ENABLE_AUTOMATIC_REQUEST
    m_timer.setInterval(AUTOMATIC_CLIENT_REQUEST_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, this, &Client::runGetPathListScenario);
    m_timer.start();
#endif // ENABLE_AUTOMATIC_REQUEST
}

Client::~Client()
{
}

void Client::onHightLightModeChanged()
{
    if (!m_lastPathId.isEmpty()) {
        requestPathHighLight(m_lastPathId, "hight light mode change");
    }
}

bool Client::isConnected() const
{
    return m_socket.isConnected();
}

void Client::startConnectionWatcher()
{
    m_socket.startConnectionWatcher();
}

void Client::stopConnectionWatcher()
{
    m_socket.stopConnectionWatcher();
}

void Client::handleResponse(const QByteArray& bytes)
{
    SimpleLogger::instance().debug("from server:", bytes ,"size:", bytes.size()/1024, "Kb");

    // Convert the QByteArray to a QJsonDocument
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        SimpleLogger::instance().error("Error parsing JSON:", parseError.errorString());
        return;
    }

    // Extract the QJsonObject from the QJsonDocument
    QJsonObject jsonObject = jsonDoc.object();

    int cmd = jsonObject[KEY_CMD].toString().toInt();
    bool status = jsonObject[KEY_STATUS].toString().toInt();
    QString data = jsonObject[KEY_DATA].toString();

    SimpleLogger::instance().log(cmd, status, data);
    if (status) {
        switch(cmd) {
        case CMD_GET_PATH_LIST_ID: emit pathListDataReceived(data); break;
        case CMD_DRAW_PATH_ID: emit highLightModeReceived(); break;
        }
    } else {
        SimpleLogger::instance().error("unable to perform cmd on server, error", data);
    }
}

void Client::sendRequest(QByteArray& requestBytes, const QString& initiator)
{
    if (!m_socket.isConnected()) {
        m_socket.connect();
    }
    requestBytes.append(static_cast<unsigned char>(TELEGRAM_FRAME_DELIMETER));
    SimpleLogger::instance().debug("sending:", requestBytes, QString("requested by [%1]").arg(initiator));
    if (!m_socket.write(requestBytes)) {
        SimpleLogger::instance().error("fail to send");
     }
}

void Client::requestPathList(const QString& initiator)
{
    QByteArray bytes = RequestCreator::instance().getPathListRequestTelegram(m_parameters->getCriticalPathNum(),
                                                                             m_parameters->getPathType().c_str(),
                                                                             m_parameters->getPathDetailLevel(),
                                                                             m_parameters->getIsFlatRouting());
    sendRequest(bytes, initiator);
}

void Client::requestPathHighLight(const QString& pathId, const QString& initiator)
{
    m_lastPathId = pathId;
    int highLightMode = m_parameters->getHighLightMode() + 1; // +1 here is to shift item "None";
    QByteArray bytes = RequestCreator::instance().getDrawPathIdTelegram(pathId, highLightMode);
    sendRequest(bytes, initiator);
}
