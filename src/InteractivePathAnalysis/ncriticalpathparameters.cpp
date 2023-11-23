#include "ncriticalpathparameters.h"
#include "simplelogger.h"

#ifndef STANDALONE_APP
#include "../Compiler/QLSettingsManager.h"
#endif

NCriticalPathParameters::NCriticalPathParameters()
{
    loadFromFile();
}

void NCriticalPathParameters::setHighLightMode(int value)
{
    m_parameters.hightLightMode = value;
}

void NCriticalPathParameters::setPathType(const QString& value)
{
    m_parameters.pathType = value;
}

void NCriticalPathParameters::setPathDetailLevel(int value)
{
    m_parameters.pathDetailLevel = value;
}

void NCriticalPathParameters::setCriticalPathNum(int value)
{
    m_parameters.criticalPathNum = value;
}

void NCriticalPathParameters::setSavePathListSettings(bool value)
{
    m_parameters.savePathListSettings = value;
}

void NCriticalPathParameters::saveOptionSavePathListSettings()
{
    saveIntValue(OPT_SAVE_PATH_LIST_SETTINGS, m_parameters.savePathListSettings);
}

#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
void NCriticalPathParameters::setSaveFilterSettings(bool value)
{
    m_parameters.saveFilterSettings = value;
}
#endif

bool NCriticalPathParameters::getIsFlatRouting() const
{
#ifdef STANDALONE_APP
    return m_parameters.isFlatRouting;
#else
    // reload QLSettingsManager() to ensure we account for dynamic changes in the settings/power json:
    FOEDAG::QLSettingsManager::reloadJSONSettings();

    // check if settings were loaded correctly before proceeding:
    if((FOEDAG::QLSettingsManager::getInstance()->settings_json).empty()) {
        SimpleLogger::instance().log("Project Settings JSON is missing, please check <project_name> and corresponding <project_name>.json exists");
        return false;
    }

    if( FOEDAG::QLSettingsManager::getStringValue("vpr", "route", "flat_routing") == "checked" ) {
        return true;
    }
    return false;
#endif
}

void NCriticalPathParameters::saveIntValue(const QString& key, int value)
{
#ifdef STANDALONE_APP
    m_settings.setValue(key, value);
#else
    assert(false);
#endif
}

void NCriticalPathParameters::saveStringValue(const QString& key, const QString& value)
{
#ifdef STANDALONE_APP
    m_settings.setValue(key, value);
#else
    assert(false);
#endif
}

int NCriticalPathParameters::loadIntValue(const QString& key, int defaultValue) const
{
#ifdef STANDALONE_APP
    if (QVariant value = m_settings.value(key); value.isValid()) {
        bool ok;
        int candidate = value.toInt(&ok);
        if (ok) {
            return candidate;
        }
    }
#else
    assert(false);
#endif
    return defaultValue;
}

QString NCriticalPathParameters::loadStringValue(const QString& key, const QString& defaultValue) const
{
#ifdef STANDALONE_APP
    if (QVariant value = m_settings.value(key); value.isValid()) {
        return value.toString();
    }
#else
    assert(false);
#endif
    return defaultValue;
}

void NCriticalPathParameters::saveToFile()
{
    saveIntValue(OPT_HIGH_LIGHT_MODE, m_parameters.hightLightMode);
    saveStringValue(OPT_PATH_TYPE, m_parameters.pathType);
    saveIntValue(OPT_PATH_DETAIL_LEVEL, m_parameters.pathDetailLevel);
    saveIntValue(OPT_N_CRITICAL_PATH_NUM, m_parameters.criticalPathNum);
    saveIntValue(OPT_SAVE_PATH_LIST_SETTINGS, m_parameters.savePathListSettings);
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    saveIntValue(OPT_SAVE_FILTER_SETTINGS, m_parameters.saveFilterSettings);
#endif
}

void NCriticalPathParameters::loadFromFile()
{
    m_parameters.hightLightMode = loadIntValue(OPT_HIGH_LIGHT_MODE, m_defaultParameters.hightLightMode);
    m_parameters.pathType = loadStringValue(OPT_PATH_TYPE, m_defaultParameters.pathType);
    m_parameters.pathDetailLevel = loadIntValue(OPT_PATH_DETAIL_LEVEL, m_defaultParameters.pathDetailLevel);
    m_parameters.criticalPathNum = loadIntValue(OPT_N_CRITICAL_PATH_NUM, m_defaultParameters.criticalPathNum);
    m_parameters.savePathListSettings = loadIntValue(OPT_SAVE_PATH_LIST_SETTINGS, m_defaultParameters.savePathListSettings);
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    m_parameters.saveFilterSettings = loadIntValue(OPT_SAVE_FILTER_SETTINGS, m_defaultParameters.saveFilterSettings);
#endif
}
