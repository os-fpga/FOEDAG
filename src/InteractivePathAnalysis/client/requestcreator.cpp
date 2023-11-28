#include "requestcreator.h"
#include "keys.h"
#include <regex>

#include <QJsonObject>
#include <QJsonDocument>

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

QByteArray RequestCreator::getDrawPathIdTelegram(const QString& pathId, const QString& highLightMode)
{
    auto extractPathIndex = [](const std::string& message) {
        static std::regex pattern("^#Path (\\d+)");
        std::smatch match;
        if (std::regex_search(message, match, pattern)) {
            if (match.size() > 1) {
                return std::atoi(match[1].str().c_str());
            }
        }
        return -1;
    };

    int pathIndex = extractPathIndex(pathId.toStdString());
    if (pathIndex > 0) {
        pathIndex--;
    }

    QString options;
    options.append(QString("int:%1:%2;").arg(OPTION_PATH_INDEX).arg(pathIndex));
    options.append(QString("string:%1:%2;").arg(OPTION_HIGHTLIGHT_MODE).arg(highLightMode));

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

