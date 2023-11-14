#pragma once

#include "ncriticalpathsettings.h"

#include <QString>
#include <QDebug>

#include <memory>

#ifndef STANDALONE_APP
#include "../Compiler/QLSettingsManager.h"
#endif

class NCriticalPathParameters {
public:
    NCriticalPathParameters() {
        load();
    }

    void saveToSettings() {
        auto& settings = NCriticalPathSettings::instance();
        settings.setHighLightMode(m_highLightMode);
        settings.setPathType(m_pathType);
        settings.setPathDetailLevel(m_detailLevel);
        settings.setCriticalPathNum(m_criticalPathNum);
    }

    void setCriticalPathNum(int criticalPathNum) { m_criticalPathNum = criticalPathNum; }
    void setPathType(const QString& pathType) { m_pathType = pathType; }
    void setDetailLevel(int detailLevel) { m_detailLevel = detailLevel; }
#ifdef STANDALONE_APP
    void setFlatRouting(bool isFlatRouting) { m_isFlatRouting = isFlatRouting; }
#endif
    void setHighLightMode(int highLightMode) { m_highLightMode = highLightMode; }

    int getCriticalPathNum() const { return m_criticalPathNum; }
    const QString& getPathType() { return m_pathType; }
    int getDetailLevel() { return m_detailLevel; }
    bool getIsFlatRouting() const
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
    int getHighLightMode() { return m_highLightMode; }

private:
    int m_criticalPathNum;
    QString m_pathType;
    int m_detailLevel;
#ifdef STANDALONE_APP
    bool m_isFlatRouting;
#endif
    int m_highLightMode;

    void load() {
        setHighLightMode(NCriticalPathSettings::instance().getHighLightMode());
        setPathType(NCriticalPathSettings::instance().getPathType());
        setCriticalPathNum(NCriticalPathSettings::instance().getCriticalPathNum());
        setDetailLevel(NCriticalPathSettings::instance().getPathDetailLevel());
    }
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
