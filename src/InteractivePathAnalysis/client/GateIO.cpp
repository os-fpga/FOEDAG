#include "GateIO.h"
#include "CommConstants.h"
#include "TcpSocket.h"
#include "RequestCreator.h"
#include "ZlibUtils.h"
#include "ConvertUtils.h"
#include "TelegramParser.h"

namespace client {

GateIO::GateIO(const NCriticalPathParametersPtr& parameters)
    : m_parameters(parameters)
{
    connect(&m_socket, &TcpSocket::connectedChanged, this, &GateIO::connectedChanged);
    connect(&m_socket, &TcpSocket::dataRecieved, this, &GateIO::handleResponse);

    m_statShowTimer.setInterval(STAT_LOG_INTERVAL_MS);
    connect(&m_statShowTimer, &QTimer::timeout, this, [this](){
        m_jobStatusStat.show(true);
    });    
    m_statShowTimer.start();
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

void GateIO::handleResponse(const QByteArray& bytes, bool isCompressed)
{
    static const std::string echoData{comm::ECHO_DATA};

    std::string rawData{bytes.begin(), bytes.end()};

    bool isEchoTelegram = false;
    if (rawData.size() == echoData.size()) {
        if (rawData == echoData) {
            sendRequest(comm::TelegramHeader::constructFromData(echoData), QByteArray(echoData.c_str()), comm::ECHO_DATA);
            isEchoTelegram = true;
        }
    }
    if (!isEchoTelegram) {                
        std::optional<std::string> decompressedTelegramOpt;
#ifndef FORCE_DISABLE_ZLIB_TELEGRAM_COMPRESSION
        if (isCompressed) {
            decompressedTelegramOpt = tryDecompress(rawData);
        }
#endif
        if (!decompressedTelegramOpt) {
            decompressedTelegramOpt = std::move(rawData);
            rawData.clear();
        }

        const std::string& telegram  = decompressedTelegramOpt.value();

        std::optional<int> jobIdOpt = comm::TelegramParser::tryExtractFieldJobId(telegram);
        std::optional<int> cmdOpt = comm::TelegramParser::tryExtractFieldCmd(telegram);
        std::optional<int> statusOpt = comm::TelegramParser::tryExtractFieldStatus(telegram);
        std::optional<std::string> dataOpt;
        comm::TelegramParser::tryExtractFieldData(telegram, dataOpt);

        bool isResponseConsistent = true;
        if (!jobIdOpt) {
            SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_JOB_ID);
            isResponseConsistent = false;
        }
        if (!cmdOpt) {
            SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_CMD);
            isResponseConsistent = false;
        }
        if (!statusOpt) {
            SimpleLogger::instance().error("bad response telegram, missing required field", comm::KEY_STATUS);
            isResponseConsistent = false;
        }
        if (!dataOpt) {
            dataOpt = "";
        }

        if (isResponseConsistent) {
            int jobId = jobIdOpt.value();

            int cmd = cmdOpt.value();
            bool status = statusOpt.value();
            QString data{dataOpt.value().c_str()};

            std::optional<std::pair<int64_t, int64_t>> measurementOpt = m_jobStatusStat.trackJobFinish(jobId, status, data.size());
            if (measurementOpt) {
                const auto [sizeBytes, durationMs] = measurementOpt.value();
                SimpleLogger::instance().log("job", jobId, "size", getPrettySizeStrFromBytesNum(sizeBytes).c_str(), "took", getPrettyDurationStrFromMs(durationMs).c_str());
            }

            //SimpleLogger::instance().debug("cmd:", cmd, "status:", status, "data:", getTruncatedMiddleStr(data.toStdString()).c_str());
            if (status) {
                switch(cmd) {
                case comm::CMD_GET_PATH_LIST_ID: emit pathListDataReceived(data); break;
                case comm::CMD_DRAW_PATH_ID: emit highLightModeReceived(); break;
                }
            } else {
                SimpleLogger::instance().error("unable to perform cmd on server, error", getTruncatedMiddleStr(data.toStdString()).c_str());
            }
        } else {
            m_jobStatusStat.trackResponseBroken();
        }
    }
}

void GateIO::sendRequest(const comm::TelegramHeader& header, const QByteArray& body, const QString& initiator)
{
    if (!m_socket.isConnected()) {
        m_socket.connect();
    }
    QByteArray telegram(reinterpret_cast<const char*>(header.buffer().data()), header.size());
    telegram.append(body);
    if (m_socket.write(telegram)) {
        if (initiator != comm::ECHO_DATA) {
            // we don't want that ECHO telegram will take a part in statistic
            m_jobStatusStat.trackRequestCreation(RequestCreator::instance().lastRequestId(), header.bodyBytesNum());
        }
        SimpleLogger::instance().debug("sent", header.info().c_str(), " data[", getTruncatedMiddleStr(body.toStdString()).c_str(), "]", QString("requested by [%1]").arg(initiator));
    } else {
        SimpleLogger::instance().error("unable to send", header.info().c_str(), " data[", getTruncatedMiddleStr(body.toStdString()).c_str(), "]", QString("requested by [%1]").arg(initiator));
    }    
}

void GateIO::requestPathList(const QString& initiator)
{
    m_lastPathItems = comm::CRITICAL_PATH_ITEMS_SELECTION_NONE; // reset previous selection on new path list request
    auto [bytes, compressorId] = RequestCreator::instance().getPathListRequestTelegram(m_parameters->getCriticalPathNum(),
                                                                             m_parameters->getPathType().c_str(),
                                                                             m_parameters->getPathDetailLevel().c_str(),
                                                                             m_parameters->getIsFlatRouting());
    comm::TelegramHeader header(bytes.size(), comm::ByteArray::calcCheckSum(bytes), compressorId);
    sendRequest(header, bytes, initiator);
}

void GateIO::requestPathItemsHighLight(const QString& pathItems, const QString& initiator)
{
    m_lastPathItems = pathItems;
    auto [bytes, compressorId] = RequestCreator::instance().getDrawPathItemsTelegram(pathItems, m_parameters->getHighLightMode().c_str(), m_parameters->getIsDrawCriticalPathContourEnabled());
    comm::TelegramHeader header(bytes.size(), comm::ByteArray::calcCheckSum(bytes), compressorId);
    sendRequest(header, bytes, initiator);
}

} // namespace client