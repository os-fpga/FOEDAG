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
    QString options = QString("%1;%2;%3;%4").arg(nCriticalPathNum).arg(pathType).arg(detailedLevel).arg(isFlat);
    return getTelegram(CMD_GET_PATH_LIST_ID, options);
}

QByteArray RequestCreator::getDrawPathIdTelegram(const QString& pathId, int highLightMode)
{
    QString options{pathId + ";" + QString::number(highLightMode)};
    return getTelegram(CMD_DRAW_PATH_ID, options);
}

QByteArray RequestCreator::getTelegram(int cmd, const QString& options)
{
    QJsonObject ob;
    ob[KEY_JOB_ID] = getNextRequestId();
    ob[KEY_CMD] = cmd;
    ob[KEY_OPTIONS] = options;

    QJsonDocument jsonDoc(ob);
    return jsonDoc.toJson(QJsonDocument::Compact);
}

int RequestCreator::getNextRequestId()
{
    return ++m_lastRequestId;
}

