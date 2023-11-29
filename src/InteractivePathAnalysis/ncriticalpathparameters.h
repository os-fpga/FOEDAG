#pragma once

#include "client/keys.h"
#include "ncriticalpathmoduleinfo.h"

#include "nlohmann_json/json.hpp"

#include <filesystem>
#include <string>
#include <memory>

class NCriticalPathParameters {
    const char* WIDGET_COMBOBOX = "dropdown";
    const char* WIDGET_CHECKBOX = "checkbox";

    const char* SUBP_HELP = "help";
    const char* SUBP_OPTIONS = "options";
    const char* SUBP_LABEL = "label";
    const char* SUBP_USER_VALUE = "userValue";
    const char* SUBP_WIDGET_TYPE = "widgetType";

    const char* CATEGORY_IPA = NCRITICALPATH_INNER_NAME;
    const char* CATEGORY_VPR = "vpr";

    const char* SUBCATEGORY_ROUTE = "route";
    const char* SUBCATEGORY_PATHLIST = "pathlist";
    const char* SUBCATEGORY_ANALYSIS = "analysis";

    const char* PARAM_HIGH_LIGHT_MODE = "high_light_mode";
    const char* PARAM_TYPE = "type";
    const char* PARAM_TIMING_REPORT_NPATHS = "timing_report_npaths";
    const char* PARAM_TIMING_REPORT_DETAIL = "timing_report_detail";
    const char* PARAM_FLAT_ROUTING = "flat_routing";

    const char* DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE = "Crit Path Flylines";
    const char* DEFAULT_VALUE_PATHLIST_PARAM_TYPE = KEY_SETUP_PATH_LIST;
    const char* DEFAULT_VALUE_PATHLIST_PARAM_DETAIL_LEVEL = "netlist";
    const int DEFAULT_VALUE_PATHLIST_PARAM_MAX_NUM = 100;
    const bool DEFAULT_VALUE_PATHLIST_PARAM_IS_FLAT_ROUTING = false;

public:
    NCriticalPathParameters();
    ~NCriticalPathParameters()=default;

    const std::vector<std::string>& getHighLightAvailableOptions() const;
    const std::vector<std::string>& getPathDetailAvailableOptions();
    const std::vector<std::string>& getCritPathTypeAvailableOptions() const;

    bool isPathListConfigChanged() const { return m_isPathListConfigChanged; }
    bool isHightLightModeChanged() const { return m_isHightLightModeChanged; }
    bool isFlatRoutingChanged() const { return m_isFlatRoutingChanged; }

    static std::filesystem::path getFilePath();

    bool saveToFile();
    bool loadFromFile();
    void resetChangedFlags();

    void setHighLightMode(const std::string& value);
    void setPathType(const std::string& value);
    void setPathDetailLevel(const std::string& value);
    void setCriticalPathNum(int value);
    void setFlatRouting(bool value);

    const std::string& getHighLightMode() const { return m_highLightMode; }
    const std::string& getPathType() const { return m_pathType; }
    const std::string& getPathDetailLevel() const { return m_pathDetailLevel; }
    int getCriticalPathNum() const { return m_criticalPathNum; }
    bool getIsFlatRouting() const { return m_isFlatRouting; }

private:
    bool m_isPathListConfigChanged = false;
    bool m_isHightLightModeChanged = false;
    bool m_isFlatRoutingChanged = false;

    std::string m_highLightMode = DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE;
    std::string m_pathType = DEFAULT_VALUE_PATHLIST_PARAM_TYPE;
    std::string m_pathDetailLevel = DEFAULT_VALUE_PATHLIST_PARAM_DETAIL_LEVEL;
    int m_criticalPathNum = DEFAULT_VALUE_PATHLIST_PARAM_MAX_NUM;
    bool m_isFlatRouting = DEFAULT_VALUE_PATHLIST_PARAM_IS_FLAT_ROUTING;

    std::vector<std::string> m_pathDetailsAvailableOptions;

    void validateDefaultValues(nlohmann::json&);
    std::string stringifyBool(bool flag) { return flag? "checked": "unchecked"; }

    bool setBoolValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, bool value);
    bool setIntValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, int value);
    bool setStringValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value);

    bool getBoolValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const;
    int getIntValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const;
    std::string getStringValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const;

    bool loadFromFile(nlohmann::json& json) const;
    void saveToFile(const nlohmann::json& json);

    bool setDefaultString(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, const std::string& value) const;
    bool setDefaultVector(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, const std::vector<std::string>& value) const;
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
