#include "ncriticalpathsettings.h"

NCriticalPathSettings::NCriticalPathSettings()
{
    load();
}

NCriticalPathSettings& NCriticalPathSettings::instance()
{
    static NCriticalPathSettings settings;
    return settings;
}

void NCriticalPathSettings::setHighLightMode(int value)
{
    m_settings.setValue(OPT_HIGH_LIGHT_MODE, value);
}

void NCriticalPathSettings::setPathType(const QString& value)
{
    m_settings.setValue(OPT_PATH_TYPE, value);
}

void NCriticalPathSettings::setPathDetailLevel(int value)
{
    m_settings.setValue(OPT_PATH_DETAIL_LEVEL, value);
}

void NCriticalPathSettings::setCriticalPathNum(int value)
{
    m_settings.setValue(OPT_N_CRITICAL_PATH_NUM, value);
}

void NCriticalPathSettings::setAutoRefreshPathList(bool value)
{
    m_settings.setValue(OPT_AUTO_REFRESH_PATH_LIST, value);
}

void NCriticalPathSettings::load()
{
    if (QVariant value = m_settings.value(OPT_HIGH_LIGHT_MODE); value.isValid()) {
        m_hightLightMode = value.toInt();
    }

    if (QVariant value = m_settings.value(OPT_PATH_TYPE); value.isValid()) {
        m_pathType = value.toString();
    }

    if (QVariant value = m_settings.value(OPT_PATH_DETAIL_LEVEL); value.isValid()) {
        m_pathDetailLevel = value.toInt();
    }

    if (QVariant value = m_settings.value(OPT_N_CRITICAL_PATH_NUM); value.isValid()) {
        m_criticalPathNum = value.toInt();
    } else {
        m_criticalPathNum = 100;
    }

    if (QVariant value = m_settings.value(OPT_AUTO_REFRESH_PATH_LIST); value.isValid()) {
        m_autoRefreshPathList = value.toBool();
    } else {
        m_autoRefreshPathList = true;
    }
}
