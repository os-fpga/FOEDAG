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

void NCriticalPathSettings::setLastSelectedPathId(const QString& value)
{
    m_settings.setValue(OPT_LAST_SELECTED_PATH_ID, value);
}

void NCriticalPathSettings::setHighLightMode(const QString& value)
{
    m_settings.setValue(OPT_HIGH_LIGHT_MODE, value);
}

void NCriticalPathSettings::setPathType(const QString& value)
{
    m_settings.setValue(OPT_PATH_TYPE, value);
}

void NCriticalPathSettings::setPathDetailLevel(const QString& value)
{
    m_settings.setValue(OPT_PATH_DETAIL_LEVEL, value);
}

void NCriticalPathSettings::setCriticalPathNum(const QString& value)
{
    m_settings.setValue(OPT_N_CRITICAL_PATH_NUM, value);
}

void NCriticalPathSettings::setAutoRefreshPathList(bool value)
{
    m_settings.setValue(OPT_AUTO_REFRESH_PATH_LIST, value);
}

void NCriticalPathSettings::load()
{
    //
    {
        QVariant value = m_settings.value(OPT_LAST_SELECTED_PATH_ID);
        if (value.isValid()) {
            m_lastSelectedId = value.toString();
        }
    }

    //
    {
        QVariant value = m_settings.value(OPT_HIGH_LIGHT_MODE);
        if (value.isValid()) {
            m_hightLightMode = value.toString();
        }
    }

    //
    {
        QVariant value = m_settings.value(OPT_PATH_TYPE);
        if (value.isValid()) {
            m_pathType = value.toString();
        }
    }

    //
    {
        QVariant value = m_settings.value(OPT_PATH_DETAIL_LEVEL);
        if (value.isValid()) {
            m_pathDetailLevel = value.toString();
        }
    }

    //
    {
        QVariant value = m_settings.value(OPT_N_CRITICAL_PATH_NUM);
        if (value.isValid()) {
            m_criticalPathNum = value.toString();
        } else {
            m_criticalPathNum = QString::number(100);
        }
    }

    {
        QVariant value = m_settings.value(OPT_AUTO_REFRESH_PATH_LIST);
        if (value.isValid()) {
            m_autoRefreshPathList = value.toBool();
        } else {
            m_autoRefreshPathList = true;
        }
    }
}
