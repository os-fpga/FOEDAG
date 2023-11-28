#pragma once

#include "client/keys.h"
#include "ncriticalpathmoduleinfo.h"

#include "nlohmann_json/json.hpp"

#include <filesystem>
#include <string>
#include <memory>

class NCriticalPathParameters {
    const char* KEY_USER_VALUE = "userValue";
    const char* KEY_DEFAULT_VALUE = "default";

    const char* CATEGORY_IPA = NCRITICALPATH_INNER_NAME;
    const char* CATEGORY_VPR = "vpr";
    const char* SUBCATEGORY_ROUTE = "route";
    const char* SUBCATEGORY_PATHLIST = "pathlist";
    const char* SUBCATEGORY_ANALYSIS = "analysis";
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const char* SUBCATEGORY_FILTER = "filter";
#endif
    const char* PARAMETER_HIGH_LIGHT_MODE = "high_light_mode";
    const char* PARAMETER_TYPE = "type";
    const char* PARAMETER_TIMING_REPORT_NPATHS = "timing_report_npaths";
    const char* PARAMETER_TIMING_REPORT_DETAIL = "timing_report_detail";
    const char* PARAMETER_SAVE_SETTINGS = "save_settings";
    const char* PARAMETER_FLAT_ROUTING = "flat_routing";

    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE = "flylines";
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE = KEY_SETUP_PATH_LIST;
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL = "netlist";
    const int DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM = 100;
    const bool DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS = true;
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const bool DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS = false;
#endif
    const bool DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING = false;

public:
    NCriticalPathParameters();
    ~NCriticalPathParameters()=default;

    bool isPathListConfigChanged() const { return m_isPathListConfigChanged; }
    bool isHightLightModeChanged() const { return m_isHightLightModeChanged; }
    bool isFlatRoutingChanged() const { return m_isFlatRoutingChanged; }

    static std::filesystem::path filePath();

    void saveToFile();
    bool loadFromFile();

    void setHighLightMode(const std::string& value);
    void setPathType(const std::string& value);
    void setPathDetailLevel(const std::string& value);
    void setCriticalPathNum(int value);
    void setFlatRouting(bool value);

    void saveOptionSavePathListSettingsExplicitly(bool);

#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    void setSaveFilterSettings(bool);
#endif

    const std::string& getHighLightMode() const { return m_highLightMode; }
    const std::string& getPathType() const { return m_pathType; }
    const std::string& getPathDetailLevel() const { return m_pathDetailLevel; }
    int getCriticalPathNum() const { return m_criticalPathNum; }
    bool getSavePathListSettings() const { return m_isSavePathListSettingsEnabled; }
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    bool getSaveFilterSettings() const { return m_isSaveFilterSettingsEnabled; }
#endif
    bool getIsFlatRouting() const { return m_isFlatRouting; }

private:
    bool m_isPathListConfigChanged = false;
    bool m_isHightLightModeChanged = false;
    bool m_isFlatRoutingChanged = false;

    std::string m_highLightMode = DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE;
    std::string m_pathType = DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE;
    std::string m_pathDetailLevel = DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL;
    int m_criticalPathNum = DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM;
    bool m_isSavePathListSettingsEnabled = DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS;
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    bool m_isSaveFilterSettingsEnabled = DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS;
#endif
    bool m_isFlatRouting = DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING;

//    void validateDefaultValues();

    std::string getDefaultValueString(const std::string& category, const std::string& subcategory, const std::string& parameter);

    bool setBoolValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, bool value);
    bool setIntValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, int value);
    bool setStringValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value);

    bool getBoolValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const;
    int getIntValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const;
    std::string getStringValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, bool forceReturnDefaultValue=false) const;

    bool loadFromFile(nlohmann::json& json);
    void saveToFile(const nlohmann::json& json);

    void createSettingsTemplateFile();

    void resetChangedFlags();
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
