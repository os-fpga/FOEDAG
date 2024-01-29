#define USE_CUSTOM_TELEGRAM_PARSER // to oveeride limitation of QJsonDocument maximum size ~100Mb

#include "GateIO.h"
#include "ClientConstants.h"
#include "TcpSocket.h"
#include "RequestCreator.h"
#include "../SimpleLogger.h"

#ifdef USE_CUSTOM_TELEGRAM_PARSER
#include "TelegramParser.h"
#else
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#endif

namespace {

std::string float_to_string(float value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;  // Set precision to 1 digit after the decimal point
    return ss.str();
}

std::string prettyDurationFromMs(int64_t durationMs) {
    std::string result;
    if (durationMs >= 1000) {
        result = float_to_string(durationMs/1000.0f) + " sec";
    } else {
        result = std::to_string(durationMs);
        result += " ms";
    }
    return result;
}

std::string prettySizeFromBytesNum(int64_t bytesNum) {
    std::string result;
    if (bytesNum >= 1024*1024*1024) {
        result = float_to_string(bytesNum/float(1024*1024*1024)) + " Gb";
    } else if (bytesNum >= 1024*1024) {
        result = float_to_string(bytesNum/float(1024*1024)) + " Mb";
    } else if (bytesNum >= 1024) {
        result = float_to_string(bytesNum/float(1024.0f)) + " Kb";
    } else {
        result = std::to_string(bytesNum) + " bytes";
    }
    return result;
}

} // namespace

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

#ifdef USE_CUSTOM_TELEGRAM_PARSER
    std::string telegram{bytes.constData()};

    std::optional<int> jobIdOpt = TelegramParser::tryExtractFieldJobId(telegram);
    std::optional<int> cmdOpt = TelegramParser::tryExtractFieldCmd(telegram);
    std::optional<int> statusOpt = TelegramParser::tryExtractFieldStatus(telegram);
    std::optional<std::string> dataOpt = TelegramParser::tryExtractFieldData(telegram);
    if (!jobIdOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", KEY_JOB_ID);
        return;
    }
    if (!cmdOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", KEY_CMD);
        return;
    }
    if (!statusOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", KEY_STATUS);
        return;
    }
    if (!dataOpt) {
        dataOpt = "";
    }

    int jobId = jobIdOpt.value();
    int cmd = cmdOpt.value();
    bool status = statusOpt.value();
    QString data{dataOpt.value().c_str()};

    std::optional<std::pair<int64_t, int64_t>> measurementOpt = m_commInspector.onJobFinished(jobId, data.size());
    if (measurementOpt) {
        const auto [sizeBytes, durationMs] = measurementOpt.value();
        SimpleLogger::instance().log("job", jobId, ", size", prettySizeFromBytesNum(sizeBytes).c_str(), ", took", prettyDurationFromMs(durationMs).c_str());
    }
#else
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
#endif

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
    m_commInspector.onJobStart(RequestCreator::instance().lastRequestId(), requestBytes.size());

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

void GateIO::requestPathItemsHighLight(const QString& pathItems, const QString& initiator)
{
    m_lastPathItems = pathItems;
    QByteArray bytes = RequestCreator::instance().getDrawPathItemsTelegram(pathItems, m_parameters->getHighLightMode().c_str(), m_parameters->getDrawPathContour());
    sendRequest(bytes, initiator);
}

} // namespace client