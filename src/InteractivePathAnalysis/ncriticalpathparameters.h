#pragma once

#include <client/keys.h>

#ifdef STANDALONE_APP
#include <QSettings>
#endif

#include <QString>
#include <memory>

class NCriticalPathParameters {
    const char* CATEGORY = "ipa";
    const char* PATHLIST_SUBCATEGORY = "pathlist";
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const char* FILTER_SUBCATEGORY = "filter";
#endif
    const char* HIGH_LIGHT_MODE_PARAMETER = "highLightMode";
    const char* TYPE_PARAMETER = "type";
    const char* MAX_NUM_PARAMETER = "maxNum";
    const char* DETAIL_LEVEL_PARAMETER = "detailLevel";
    const char* SAVE_SETTINGS_PARAMETER = "saveSettings";

    struct Parameters {
        int hightLightMode = 0;
        QString pathType = KEY_SETUP_PATH_LIST;
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

    void saveIntValue(const QString& subcategory, const QString& parameter, int value);
    void saveStringValue(const QString& subcategory, const QString& parameter, const QString& value);

    int loadIntValue(const QString& subcategory, const QString& parameter, int defaultValue) const;
    QString loadStringValue(const QString& subcategory, const QString& parameter, const QString& defaultValue) const;
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
