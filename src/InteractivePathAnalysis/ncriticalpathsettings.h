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

    void setHighLightMode(int);
    void setPathType(const QString&);
    void setPathDetailLevel(int);
    void setCriticalPathNum(int);
    void setAutoRefreshPathList(bool);

    int getHighLightMode() const { return m_hightLightMode; }
    QString getPathType() const { return m_pathType; }
    int getPathDetailLevel() const { return m_pathDetailLevel; }
    int getCriticalPathNum() const { return m_criticalPathNum; }
    bool getAutoRefreshPathList() const { return m_autoRefreshPathList; }

private:
    QSettings m_settings;

    int m_hightLightMode;
    QString m_pathType;
    int m_pathDetailLevel;
    int m_criticalPathNum;
    bool m_autoRefreshPathList;

    void load();
};
