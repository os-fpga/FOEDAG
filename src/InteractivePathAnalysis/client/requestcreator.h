#pragma once

#include <QByteArray>

namespace client {

class RequestCreator
{
public:
    static RequestCreator& instance();
    ~RequestCreator()=default;

    QByteArray getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, const QString& detailesLevel, bool isFlat);
    QByteArray getDrawPathIdTelegram(const QString& pathId, const QString& highLightMode);

private:
    RequestCreator()=default;

    int m_lastRequestId = 0;

    int getNextRequestId();

    QByteArray getTelegram(int cmd, const QString& options);
};

} // namespace client