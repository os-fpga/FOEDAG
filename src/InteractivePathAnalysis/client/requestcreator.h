#pragma once

#include <QByteArray>

class RequestCreator
{
public:
    static RequestCreator& instance();
    ~RequestCreator()=default;

    QByteArray getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, int detailesLevel, bool isFlat);
    QByteArray getDrawPathIdTelegram(const QString& pathId, int highLightMode);

private:
    RequestCreator()=default;

    int m_lastRequestId = 0;

    int getNextRequestId();
};

