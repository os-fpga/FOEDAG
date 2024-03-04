#pragma once

#include <QByteArray>

namespace client {

/**
 * @brief Utility class facilitating client request variants to the server for the Interactive Path Analysis tool.
 * 
 * This utility class serves as an interface to manage and generate various client requests 
 * sent to the server within the Interactive Path Analysis tool. It encapsulates the logic 
 * for creating and managing different types of requests that the client sends to the server, 
 * providing a streamlined and organized approach to interact with the server-side functionalities.
 */
class RequestCreator
{
public:
    static RequestCreator& instance();
    ~RequestCreator()=default;

    std::pair<QByteArray, uint8_t> getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, const QString& detailesLevel, bool isFlat);
    std::pair<QByteArray, uint8_t> getDrawPathItemsTelegram(const QString& pathItems, const QString& highLightMode, bool drawPathContour);

    int lastRequestId() const { return m_lastRequestId; }

private:
    RequestCreator()=default;

    int m_lastRequestId = 0;

    int getNextRequestId();

    QByteArray getTelegram(int cmd, const QString& options);
};

} // namespace client