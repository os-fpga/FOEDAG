#include "RequestCreator.h"
#include "CommConstants.h"

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

std::pair<QByteArray, uint8_t> RequestCreator::getPathListRequestTelegram(int nCriticalPathNum, const QString& pathType, const QString& detailsLevel, bool isFlat)
{
    QString options;
    if ((nCriticalPathNum < 0) || (nCriticalPathNum > comm::CRITICAL_PATH_NUM_THRESHOLD)) {
        qInfo() << "requested value" << nCriticalPathNum << "for n critical path max num is out of supported range, value limited to maximum possible" << comm::CRITICAL_PATH_NUM_THRESHOLD;
        nCriticalPathNum = comm::CRITICAL_PATH_NUM_THRESHOLD;
    }

    options.append(QString("int:%1:%2;").arg(comm::OPTION_PATH_NUM).arg(nCriticalPathNum));
    options.append(QString("string:%1:%2;").arg(comm::OPTION_PATH_TYPE).arg(pathType));
    options.append(QString("string:%1:%2;").arg(comm::OPTION_DETAILS_LEVEL).arg(detailsLevel));
    options.append(QString("bool:%1:%2").arg(comm::OPTION_IS_FLOAT_ROUTING).arg(isFlat));

    uint8_t compressorId{comm::NONE_COMPRESSOR_ID};
    QByteArray bytes = getTelegram(comm::CMD_GET_PATH_LIST_ID, options);
    return std::pair<QByteArray, uint8_t>{bytes, compressorId};
}

std::pair<QByteArray, uint8_t> RequestCreator::getDrawPathItemsTelegram(const QString& pathItems, const QString& highLightMode, bool drawPathContour)
{
    QString options;
    options.append(QString("string:%1:%2;").arg(comm::OPTION_PATH_ELEMENTS).arg(pathItems));
    options.append(QString("string:%1:%2;").arg(comm::OPTION_HIGHLIGHT_MODE).arg(highLightMode));
    options.append(QString("bool:%1:%2").arg(comm::OPTION_DRAW_PATH_CONTOUR).arg(drawPathContour));

    uint8_t compressorId{comm::NONE_COMPRESSOR_ID};
    QByteArray bytes = getTelegram(comm::CMD_DRAW_PATH_ID, options);
    return std::pair<QByteArray, uint8_t>{bytes, compressorId}; 
}

QByteArray RequestCreator::getTelegram(int cmd, const QString& options)
{
    QJsonObject ob;
    ob[comm::KEY_JOB_ID] = QString::number(getNextRequestId());
    ob[comm::KEY_CMD] = QString::number(cmd);
    ob[comm::KEY_OPTIONS] = options;

    QJsonDocument jsonDoc(ob);
    return jsonDoc.toJson(QJsonDocument::Compact);
}

int RequestCreator::getNextRequestId()
{
    return ++m_lastRequestId;
}

} // namespace client