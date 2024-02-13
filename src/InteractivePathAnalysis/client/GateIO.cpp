#define USE_CUSTOM_TELEGRAM_PARSER // to override limitation of QJsonDocument maximum size ~100Mb

#include "GateIO.h"
#include "CommConstants.h"
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
}

GateIO::~GateIO()
{
}

void GateIO::onHightLightModeChanged()
{
    requestPathItemsHighLight(m_lastPathItems, "hight light mode changed");
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
    SimpleLogger::instance().debug("from server:", bytes ,"size:", bytes.size(), "bytes");

#ifdef USE_CUSTOM_TELEGRAM_PARSER
    std::string telegram{bytes.constData()};

    std::optional<int> jobIdOpt = comm::TelegramParser::tryExtractFieldJobId(telegram);
    std::optional<int> cmdOpt = comm::TelegramParser::tryExtractFieldCmd(telegram);
    std::optional<int> statusOpt = comm::TelegramParser::tryExtractFieldStatus(telegram);
    std::optional<std::string> dataOpt = comm::TelegramParser::tryExtractFieldData(telegram);
    if (!jobIdOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_JOB_ID);
        return;
    }
    if (!cmdOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_CMD);
        return;
    }
    if (!statusOpt) {
        SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_STATUS);
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

    SimpleLogger::instance().log("cmd:", cmd, "status:", status, "data:", data);
    if (status) {
        switch(cmd) {
        case comm::CMD_GET_PATH_LIST_ID: emit pathListDataReceived(data); break;
        case comm::CMD_DRAW_PATH_ID: emit highLightModeReceived(); break;
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
    requestBytes.append(static_cast<unsigned char>(comm::TELEGRAM_FRAME_DELIMETER));
    SimpleLogger::instance().debug("sending:", requestBytes, QString("requested by [%1]").arg(initiator));
    if (!m_socket.write(requestBytes)) {
        SimpleLogger::instance().error("fail to send");
    }
}

void GateIO::requestPathList(const QString& initiator)
{
    m_lastPathItems = comm::CRITICAL_PATH_ITEMS_SELECTION_NONE; // reset previous selection on new path list request
    QByteArray bytes = RequestCreator::instance().getPathListRequestTelegram(m_parameters->getCriticalPathNum(),
                                                                             m_parameters->getPathType().c_str(),
                                                                             m_parameters->getPathDetailLevel().c_str(),
                                                                             m_parameters->getIsFlatRouting());
    sendRequest(bytes, initiator);
}

void GateIO::requestPathItemsHighLight(const QString& pathItems, const QString& initiator)
{
    m_lastPathItems = pathItems;
    QByteArray bytes = RequestCreator::instance().getDrawPathItemsTelegram(pathItems, m_parameters->getHighLightMode().c_str(), m_parameters->getIsDrawCriticalPathContourEnabled());
    sendRequest(bytes, initiator);
}

} // namespace client