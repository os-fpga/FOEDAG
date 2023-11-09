#include "requestcreator.h"
#include "keys.h"

#include <QJsonObject>
#include <QJsonDocument>

RequestCreator& RequestCreator::instance()
{
    static RequestCreator creator;
    return creator;
}

QByteArray RequestCreator::getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, int detailedLevel, bool isFlat)
{
    QJsonObject ob;
    ob[KEY_JOB_ID] = getNextRequestId();
    ob[KEY_CMD] = CMD_GET_PATH_LIST_ID;
    ob[KEY_OPTIONS] = QString("%1;%2;%3;%4").arg(nCriticalPathNum).arg(pathType).arg(detailedLevel).arg(isFlat);

    QJsonDocument jsonDoc(ob);
    return jsonDoc.toJson(QJsonDocument::Compact);
}

QByteArray RequestCreator::getDrawPathIdTelegram(const QString& pathId, int highLightMode)
{
    QJsonObject ob;
    ob[KEY_JOB_ID] = getNextRequestId();
    ob[KEY_CMD] = CMD_DRAW_PATH_ID;
    ob[KEY_OPTIONS] = pathId + ";" + QString::number(highLightMode);

    QJsonDocument jsonDoc(ob);
    return jsonDoc.toJson(QJsonDocument::Compact);
}

int RequestCreator::getNextRequestId()
{
    return ++m_lastRequestId;
}

