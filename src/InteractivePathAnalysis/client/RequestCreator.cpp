#include "RequestCreator.h"
#include "ClientConstants.h"

#include <regex>

#include <QList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

namespace client {

RequestCreator& RequestCreator::instance()
{
    static RequestCreator creator;
    return creator;
}

QByteArray RequestCreator::getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, const QString& detailsLevel, bool isFlat)
{
    QString options;
    options.append(QString("int:%1:%2;").arg(OPTION_PATH_NUM).arg(nCriticalPathNum));
    options.append(QString("string:%1:%2;").arg(OPTION_PATH_TYPE).arg(pathType));
    options.append(QString("string:%1:%2;").arg(OPTION_DETAILS_LEVEL).arg(detailsLevel));
    options.append(QString("bool:%1:%2").arg(OPTION_IS_FLOAT_ROUTING).arg(isFlat));

    return getTelegram(CMD_GET_PATH_LIST_ID, options);
}

QByteArray RequestCreator::getDrawPathItemsTelegram(const QString& pathItems, const QString& highLightMode, bool drawPathContour)
{
    QString options;
    options.append(QString("string:%1:%2;").arg(OPTION_PATH_ELEMENTS).arg(pathItems));
    options.append(QString("string:%1:%2;").arg(OPTION_HIGHTLIGHT_MODE).arg(highLightMode));
    options.append(QString("bool:%1:%2").arg(OPTION_DRAW_PATH_CONTOUR).arg(drawPathContour));

    return getTelegram(CMD_DRAW_PATH_ID, options);
}

QByteArray RequestCreator::getTelegram(int cmd, const QString& options)
{
    QJsonObject ob;
    ob[KEY_JOB_ID] = QString::number(getNextRequestId());
    ob[KEY_CMD] = QString::number(cmd);
    ob[KEY_OPTIONS] = options;

    QJsonDocument jsonDoc(ob);
    return jsonDoc.toJson(QJsonDocument::Compact);
}

int RequestCreator::getNextRequestId()
{
    return ++m_lastRequestId;
}

} // namespace client