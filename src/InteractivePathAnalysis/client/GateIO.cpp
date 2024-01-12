#include "GateIO.h"
#include "ClientConstants.h"
#include "TcpSocket.h"
#include "RequestCreator.h"
#include "../SimpleLogger.h"

#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

namespace client {

GateIO::GateIO(const NCriticalPathParametersPtr& parameters)
    : m_parameters(parameters)
{
    connect(&m_socket, &TcpSocket::connectedChanged, this, &GateIO::connectedChanged);
    connect(&m_socket, &TcpSocket::dataRecieved, this, &GateIO::handleResponse);

#ifdef ENABLE_AUTOMATIC_REQUEST
    m_timer.setInterval(AUTOMATIC_CLIENT_REQUEST_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, this, &GateIO::runGetPathListScenario);
    m_timer.start();
#endif // ENABLE_AUTOMATIC_REQUEST
}

GateIO::~GateIO()
{
}

void GateIO::onHightLightModeChanged()
{
    if (!m_lastPathItems.isEmpty()) {
        requestPathItemsHighLight(m_lastPathItems, "hight light mode changed");
    }
}

void GateIO::onServerPortDetected(int serverPortNum)
{
    m_socket.setPortNum(serverPortNum);
}

bool GateIO::isConnected() const
{
    return m_socket.isConnected();
}

void GateIO::startConnectionWatcher()
{
    m_socket.startConnectionWatcher();
}

void GateIO::stopConnectionWatcher()
{
    m_socket.stopConnectionWatcher();
}

void GateIO::handleResponse(const QByteArray& bytes)
{
    SimpleLogger::instance().debug("from server:", bytes ,"size:", bytes.size(), "Bytes");

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

void GateIO::sendRequest(QByteArray& requestBytes, const QString& initiator)
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

void GateIO::requestPathList(const QString& initiator)
{
    QByteArray bytes = RequestCreator::instance().getPathListRequestTelegram(m_parameters->getCriticalPathNum(),
                                                                             m_parameters->getPathType().c_str(),
                                                                             m_parameters->getPathDetailLevel().c_str(),
                                                                             m_parameters->getIsFlatRouting());
    sendRequest(bytes, initiator);
}

void GateIO::requestPathItemsHighLight(const QList<QString>& pathItems, const QString& initiator)
{
    m_lastPathItems = pathItems;
    QByteArray bytes = RequestCreator::instance().getDrawPathItemsTelegram(pathItems, m_parameters->getHighLightMode().c_str());
    sendRequest(bytes, initiator);
}

} // namespace client