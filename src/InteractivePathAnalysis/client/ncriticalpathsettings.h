#pragma once

#include <QSettings>

class NCriticalPathSettings {
    const char* OPT_HIGH_LIGHT_MODE = "critpaths.highLightMode";
    const char* OPT_PATH_TYPE = "critpaths.type";
    const char* OPT_N_CRITICAL_PATH_NUM = "critpaths.criticalPathNum";
    const char* OPT_PATH_DETAIL_LEVEL = "critpaths.detailLevel";
    const char* OPT_AUTO_REFRESH_PATH_LIST = "autoRefreshPathList";

    NCriticalPathSettings();

public:
    static NCriticalPathSettings& instance();
    ~NCriticalPathSettings()=default;

    void setHighLightMode(const QString&);
    void setPathType(const QString&);
    void setPathDetailLevel(const QString&);
    void setCriticalPathNum(const QString&);
    void setAutoRefreshPathList(bool);

    QString getHighLightMode() const { return m_hightLightMode; }
    QString getPathType() const { return m_pathType; }
    QString getPathDetailLevel() const { return m_pathDetailLevel; }
    QString getCriticalPathNum() const { return m_criticalPathNum; }
    bool getAutoRefreshPathList() const { return m_autoRefreshPathList; }

private:
    QSettings m_settings;

    QString m_hightLightMode;
    QString m_pathType;
    QString m_pathDetailLevel;
    QString m_criticalPathNum;
    bool m_autoRefreshPathList;

    void load();
};
