#include "ncriticalpathparameters.h"

#include "ncriticalpathsettings.h"

#include <QString>
#include <QDebug>

#ifndef STANDALONE_APP
#include "../Compiler/QLSettingsManager.h"
#endif

NCriticalPathParameters::NCriticalPathParameters()
{
    load();
}

void NCriticalPathParameters::saveToSettings()
{
    auto& settings = NCriticalPathSettings::instance();
    settings.setHighLightMode(m_highLightMode);
    settings.setPathType(m_pathType);
    settings.setPathDetailLevel(m_detailLevel);
    settings.setCriticalPathNum(m_criticalPathNum);
}

bool NCriticalPathParameters::getIsFlatRouting() const
{
#ifdef STANDALONE_APP
    return m_isFlatRouting;
#else
    // reload QLSettingsManager() to ensure we account for dynamic changes in the settings/power json:
    FOEDAG::QLSettingsManager::reloadJSONSettings();

    // check if settings were loaded correctly before proceeding:
    if((FOEDAG::QLSettingsManager::getInstance()->settings_json).empty()) {
        qCritical() << "Project Settings JSON is missing, please check <project_name> and corresponding <project_name>.json exists";
        return false;
    }

    if( FOEDAG::QLSettingsManager::getStringValue("vpr", "route", "flat_routing") == "checked" ) {
        return true;
    }
    return false;
#endif
}

void NCriticalPathParameters::load()
{
    auto& settings = NCriticalPathSettings::instance();
    setHighLightMode(settings.getHighLightMode());
    setPathType(settings.getPathType());
    setCriticalPathNum(settings.getCriticalPathNum());
    setDetailLevel(settings.getPathDetailLevel());
}

