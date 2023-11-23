#pragma once

#ifdef STANDALONE_APP
#include <QSettings>
#endif

#include <QString>
#include <memory>

class NCriticalPathParameters {
    const char* OPT_HIGH_LIGHT_MODE = "critpaths.highLightMode";
    const char* OPT_PATH_TYPE = "critpaths.type";
    const char* OPT_N_CRITICAL_PATH_NUM = "critpaths.criticalPathNum";
    const char* OPT_PATH_DETAIL_LEVEL = "critpaths.detailLevel";
    const char* OPT_SAVE_PATH_LIST_SETTINGS = "savePathListSettings";
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const char* OPT_SAVE_FILTER_SETTINGS = "saveFilterSettings";
#endif

    struct Parameters {
        int hightLightMode = 0;
        QString pathType;
        int pathDetailLevel = 0;
        int criticalPathNum = 100;
        bool savePathListSettings = true;
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
        bool saveFilterSettings = false;
#endif
#ifdef STANDALONE_APP
        bool isFlatRouting = false;
#endif
    };

public:
    NCriticalPathParameters();
    ~NCriticalPathParameters()=default;

    void saveToFile();
    void loadFromFile();

    void setHighLightMode(int);
    void setPathType(const QString&);
    void setPathDetailLevel(int);
    void setCriticalPathNum(int);
    void setSavePathListSettings(bool);
    void saveOptionSavePathListSettings();
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    void setSaveFilterSettings(bool);
#endif

#ifdef STANDALONE_APP
    void setFlatRouting(bool isFlatRouting) { m_parameters.isFlatRouting = isFlatRouting; }
#endif

    int getHighLightMode() const { return m_parameters.hightLightMode; }
    QString getPathType() const { return m_parameters.pathType; }
    int getPathDetailLevel() const { return m_parameters.pathDetailLevel; }
    int getCriticalPathNum() const { return m_parameters.criticalPathNum; }
    bool getSavePathListSettings() const { return m_parameters.savePathListSettings; }
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    bool getSaveFilterSettings() const { return m_parameters.saveFilterSettings; }
#endif
    bool getIsFlatRouting() const;

private:
#ifdef STANDALONE_APP
    QSettings m_settings;
#endif

    Parameters m_parameters;
    Parameters m_defaultParameters;

    void saveIntValue(const QString& key, int value);
    void saveStringValue(const QString& key, const QString& value);

    int loadIntValue(const QString& key, int defaultValue) const;
    QString loadStringValue(const QString& key, const QString& defaultValue) const;
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
