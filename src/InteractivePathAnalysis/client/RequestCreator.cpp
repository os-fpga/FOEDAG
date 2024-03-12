/**
  * @file RequestCreator.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RequestCreator.h"
#include "CommConstants.h"

#include <regex>

#include <QList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

namespace FOEDAG {

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
    options.append(QString("string:%1:%2;").arg(comm::OPTION_HIGHTLIGHT_MODE).arg(highLightMode));
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

} // namespace FOEDAG