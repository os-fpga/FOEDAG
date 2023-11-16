#pragma once

#include <QSettings>

class NCriticalPathSettings {
    const char* OPT_HIGH_LIGHT_MODE = "critpaths.highLightMode";
    const char* OPT_PATH_TYPE = "critpaths.type";
    const char* OPT_N_CRITICAL_PATH_NUM = "critpaths.criticalPathNum";
    const char* OPT_PATH_DETAIL_LEVEL = "critpaths.detailLevel";
    const char* OPT_SAVE_PATH_LIST_SETTINGS = "savePathListSettings";
    const char* OPT_SAVE_FILTER_SETTINGS = "saveFilterSettings";

    NCriticalPathSettings();

public:
    static NCriticalPathSettings& instance();
    ~NCriticalPathSettings()=default;

    void load();

    void setHighLightMode(int);
    void setPathType(const QString&);
    void setPathDetailLevel(int);
    void setCriticalPathNum(int);
    void setSavePathListSettings(bool);
    void setSaveFilterSettings(bool);

    int getHighLightMode() const { return m_hightLightMode; }
    QString getPathType() const { return m_pathType; }
    int getPathDetailLevel() const { return m_pathDetailLevel; }
    int getCriticalPathNum() const { return m_criticalPathNum; }
    bool getSavePathListSettings() const { return m_savePathListSettings; }
    bool getSaveFilterSettings() const { return m_saveFilterSettings; }

private:
    QSettings m_settings;

    int m_hightLightMode;
    QString m_pathType;
    int m_pathDetailLevel;
    int m_criticalPathNum;
    bool m_savePathListSettings;
    bool m_saveFilterSettings;
};
