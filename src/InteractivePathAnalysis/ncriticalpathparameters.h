#pragma once

#include "client/keys.h"

#include "nlohmann_json/json.hpp"

#include <filesystem>
#include <string>
#include <memory>

class NCriticalPathParameters {
    const char* KEY_USER_VALUE = "userValue";
    const char* KEY_DEFAULT_VALUE = "default";

    const char* CATEGORY = "ipa";
    const char* SUBCATEGORY_PATHLIST = "pathlist";
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const char* SUBCATEGORY_FILTER = "filter";
#endif
    const char* PARAMETER_HIGH_LIGHT_MODE = "highLightMode";
    const char* PARAMETER_TYPE = "type";
    const char* PARAMETER_MAX_NUM = "maxNum";
    const char* PARAMETER_DETAIL_LEVEL = "detailLevel";
    const char* PARAMETER_SAVE_SETTINGS = "saveSettings";
#ifdef STANDALONE_APP
    const char* PARAMETER_IS_FLAT_ROUTING = "isFlatRouting";
#endif

    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE = "0";
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE = KEY_SETUP_PATH_LIST;
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL = "0";
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM = "100";
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS = "checked";
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    const char* DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS = "unchecked";
#endif
#ifdef STANDALONE_APP
    const char* DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING = "unchecked";
#endif

public:
    NCriticalPathParameters();
    ~NCriticalPathParameters()=default;

    void saveToFile();
    bool loadFromFile();

    void setHighLightMode(int);
    void setPathType(const std::string&);
    void setPathDetailLevel(int);
    void setCriticalPathNum(int);
    void setSavePathListSettings(bool);

    void saveOptionSavePathListSettingsExplicitly(bool);

#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    void setSaveFilterSettings(bool);
#endif

#ifdef STANDALONE_APP
    void setFlatRouting(bool);
#endif

    int getHighLightMode() const;
    std::string getPathType() const;
    int getPathDetailLevel() const;
    int getCriticalPathNum() const;
    bool getSavePathListSettings() const;
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    bool getSaveFilterSettings() const;
#endif
    bool getIsFlatRouting() const;

private:
    nlohmann::json m_json;

    void validateDefaultValues();

    std::string getDefaultValueString(const std::string& category, const std::string& subcategory, const std::string& parameter);

    void setIntValue(const std::string& category, const std::string& subcategory, const std::string& parameter, int value);
    void setStringValue(const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value);

    int getIntValue(const std::string& category, const std::string& subcategory, const std::string& parameter) const;
    std::string getStringValue(const std::string& category, const std::string& subcategory, const std::string& parameter, bool forceReturnDefaultValue=false) const;

    std::filesystem::path filePath() const;

    bool loadFromFile(nlohmann::json& json);
    void saveToFile(const nlohmann::json& json);
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
