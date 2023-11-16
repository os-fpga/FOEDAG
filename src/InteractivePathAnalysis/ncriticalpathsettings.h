#pragma once

#include <QSettings>

class NCriticalPathSettings {
    const char* OPT_HIGH_LIGHT_MODE = "critpaths.highLightMode";
    const char* OPT_PATH_TYPE = "critpaths.type";
    const char* OPT_N_CRITICAL_PATH_NUM = "critpaths.criticalPathNum";
    const char* OPT_PATH_DETAIL_LEVEL = "critpaths.detailLevel";

    NCriticalPathSettings();

public:
    static NCriticalPathSettings& instance();
    ~NCriticalPathSettings()=default;

    void setHighLightMode(int);
    void setPathType(const QString&);
    void setPathDetailLevel(int);
    void setCriticalPathNum(int);

    int getHighLightMode() const { return m_hightLightMode; }
    QString getPathType() const { return m_pathType; }
    int getPathDetailLevel() const { return m_pathDetailLevel; }
    int getCriticalPathNum() const { return m_criticalPathNum; }

private:
    QSettings m_settings;

    int m_hightLightMode;
    QString m_pathType;
    int m_pathDetailLevel;
    int m_criticalPathNum;

    void load();
};
