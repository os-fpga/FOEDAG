#include "ncriticalpathparameters.h"
#include "simplelogger.h"

#include <fstream>
#include <iostream>

#ifndef STANDALONE_APP
#include "../Compiler/QLSettingsManager.h"
#endif

NCriticalPathParameters::NCriticalPathParameters()
{
    if (std::filesystem::exists(filePath())) {
        loadFromFile();
    }
    validateDefaultValues();
}

void NCriticalPathParameters::validateDefaultValues()
{
    bool requireSave = false;

    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE); value != DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_HIGH_LIGHT_MODE][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE;
        requireSave = true;
    }
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TYPE); value != DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_TYPE][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE;
        requireSave = true;
    }
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_DETAIL_LEVEL); value != DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_DETAIL_LEVEL][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL;
        requireSave = true;
    }
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_MAX_NUM); value != DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_MAX_NUM][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM;
        requireSave = true;
    }
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS); value != DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_SAVE_SETTINGS][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS;
        requireSave = true;
    }
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_FILTER, PARAMETER_SAVE_SETTINGS); value != DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS) {
        m_json[CATEGORY][SUBCATEGORY_FILTER][PARAMETER_SAVE_SETTINGS][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS;
        requireSave = true;
    }
#endif
#ifdef STANDALONE_APP
    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_IS_FLAT_ROUTING); value != DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING) {
        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_IS_FLAT_ROUTING][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING;
        requireSave = true;
    }
#endif

    if (requireSave) {
        saveToFile();
    }
}

std::string NCriticalPathParameters::getDefaultValueString(const std::string& category, const std::string& subcategory, const std::string& parameter)
{
    return getStringValue(category, subcategory, parameter, /*forceReturnDefaultValue*/true);
}

void NCriticalPathParameters::setHighLightMode(int value)
{
    setIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE, value);
}

void NCriticalPathParameters::setPathType(const std::string& value)
{
    setStringValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TYPE, value);
}

void NCriticalPathParameters::setPathDetailLevel(int value)
{
    setIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_DETAIL_LEVEL, value);
}

void NCriticalPathParameters::setCriticalPathNum(int value)
{
    setIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_MAX_NUM, value);
}

#ifdef STANDALONE_APP
void NCriticalPathParameters::setFlatRouting(bool value)
{
    setIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_IS_FLAT_ROUTING, value);
}
#endif

void NCriticalPathParameters::setSavePathListSettings(bool value)
{
    setIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS, value);
}

#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
void NCriticalPathParameters::setSaveFilterSettings(bool value)
{
    setIntValue(CATEGORY, SUBCATEGORY_FILTER, PARAMETER_SAVE_SETTINGS, value);
}
#endif

void NCriticalPathParameters::saveOptionSavePathListSettingsExplicitly(bool value)
{
    setSavePathListSettings(value);
    nlohmann::json json;
    if (loadFromFile(json)) {
        json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_SAVE_SETTINGS][KEY_USER_VALUE] = std::to_string(int(value));
        saveToFile(json);
    }
}

int NCriticalPathParameters::getHighLightMode() const
{
    return getIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE);
}

std::string NCriticalPathParameters::getPathType() const
{
    return getStringValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TYPE);
}

int NCriticalPathParameters::getPathDetailLevel() const
{
    return getIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_DETAIL_LEVEL);
}

int NCriticalPathParameters::getCriticalPathNum() const
{
    return getIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_MAX_NUM);
}

bool NCriticalPathParameters::getSavePathListSettings() const
{
    return getIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS);
}
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
bool NCriticalPathParameters::getSaveFilterSettings() const
{
    return m_parameters.saveFilterSettings;
}
#endif

bool NCriticalPathParameters::getIsFlatRouting() const
{
#ifdef STANDALONE_APP
    return getIntValue(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_IS_FLAT_ROUTING);
#else
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

void NCriticalPathParameters::setIntValue(const std::string& category, const std::string& subcategory, const std::string& parameter, int value)
{
    m_json[category][subcategory][parameter][KEY_USER_VALUE] = std::to_string(value);
}

void NCriticalPathParameters::setStringValue(const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value)
{
    m_json[category][subcategory][parameter][KEY_USER_VALUE] = value;
}

int NCriticalPathParameters::getIntValue(const std::string& category, const std::string& subcategory, const std::string& parameter) const
{
    int result = 0;

    std::string resultStr = getStringValue(category, subcategory, parameter);
    if (resultStr == "checked") {
        result = 1;
    } else if (resultStr == "unchecked") {
        result = 0;
    } else {
        try {
            result = std::atoi(resultStr.c_str());
        } catch(...) {
            SimpleLogger::instance().error("cannot convert", resultStr.c_str(), "value for", category.c_str(), subcategory.c_str(), parameter.c_str());
        }
    }

    return result;
}

std::string NCriticalPathParameters::getStringValue(const std::string& category, const std::string& subcategory, const std::string& parameter, bool forceReturnDefaultValue) const
{
    std::string result;

    const auto& json_ref = m_json;
    auto category_it = json_ref.find(category);
    if (category_it != json_ref.end()) {
        auto subcategory_it = category_it->find(subcategory);
        if (subcategory_it != category_it->end()) {
            auto parameter_it = subcategory_it->find(parameter);
            if (parameter_it != subcategory_it->end()) {
                auto value_it = parameter_it->find(KEY_USER_VALUE);
                if ((value_it != parameter_it->end()) && !forceReturnDefaultValue) {
                    result = value_it->get<std::string>();
                } else {
                    auto defaultValue_it = parameter_it->find(KEY_DEFAULT_VALUE);
                    if (defaultValue_it != parameter_it->end()) {
                        result = defaultValue_it->get<std::string>();
                    }
                }
            }
        }
    }

    return result;
}

void NCriticalPathParameters::saveToFile()
{
    saveToFile(m_json);
}

void NCriticalPathParameters::saveToFile(const nlohmann::json& json)
{
    std::ofstream file(filePath());
    file << std::setw(4) << json << std::endl;
}

bool NCriticalPathParameters::loadFromFile()
{
    return loadFromFile(m_json);
}

bool NCriticalPathParameters::loadFromFile(nlohmann::json& json)
{
    try {
        if (std::filesystem::exists(filePath())) {
            nlohmann::json candidate_json;
            std::ifstream file(filePath());
            file >> candidate_json;

            // if we succesfully read json
            json.clear();
            std::swap(json, candidate_json);
            return true;
        } else {
            SimpleLogger::instance().error("unable to find", filePath().c_str(), "to load");
            return false;
        }
    } catch(...) {
        SimpleLogger::instance().error("unable to load", filePath().c_str());
        return false;
    }
}

std::filesystem::path NCriticalPathParameters::filePath() const
{
#ifdef STANDALONE_APP
    return "ipa.json";
#else
    std::filesystem::path path = FOEDAG::QLSettingsManager::getInstance()->settings_json_filepath;
    path = path.remove_filename();
    return path / "ipa.json";
#endif
}
