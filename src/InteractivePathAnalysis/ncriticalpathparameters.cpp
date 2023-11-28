#include "ncriticalpathparameters.h"
#include "ncriticalpathmoduleinfo.h"
#include "simplelogger.h"

#include <fstream>
#include <iostream>

#ifndef STANDALONE_APP
#include "../Compiler/QLSettingsManager.h"
#endif

NCriticalPathParameters::NCriticalPathParameters()
{
    loadFromFile();
    //validateDefaultValues();
}

//void NCriticalPathParameters::validateDefaultValues()
//{
//    bool requireSave = false;

//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE); value != DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_HIGH_LIGHT_MODE][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_HIGH_LIGHT_MODE;
//        requireSave = true;
//    }
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TYPE); value != DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_TYPE][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_TYPE;
//        requireSave = true;
//    }
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TIMING_REPORT_DETAIL); value != DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_DETAIL_LEVEL][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_DETAIL_LEVEL;
//        requireSave = true;
//    }
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_TIMING_REPORT_NPATHS); value != DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_MAX_NUM][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_MAX_NUM;
//        requireSave = true;
//    }
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS); value != DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_SAVE_SETTINGS][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_SAVE_SETTINGS;
//        requireSave = true;
//    }
//#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_FILTER, PARAMETER_SAVE_SETTINGS); value != DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS) {
//        m_json[CATEGORY][SUBCATEGORY_FILTER][PARAMETER_SAVE_SETTINGS][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_FILTER_PARAMETER_SAVE_SETTINGS;
//        requireSave = true;
//    }
//#endif
//#ifdef STANDALONE_APP
//    if (std::string value = getDefaultValueString(CATEGORY, SUBCATEGORY_PATHLIST, PARAMETER_IS_FLAT_ROUTING); value != DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING) {
//        m_json[CATEGORY][SUBCATEGORY_PATHLIST][PARAMETER_IS_FLAT_ROUTING][KEY_DEFAULT_VALUE] = DEFAULT_VALUE_PATHLIST_PARAMETER_IS_FLAT_ROUTING;
//        requireSave = true;
//    }
//#endif

//    if (requireSave) {
//        saveToFile();
//    }
//}

//std::string NCriticalPathParameters::getDefaultValueString(const std::string& category, const std::string& subcategory, const std::string& parameter)
//{
//    return getStringValue(category, subcategory, parameter, /*forceReturnDefaultValue*/true);
//}

void NCriticalPathParameters::setHighLightMode(const std::string& value)
{
    if (m_highLightMode != value) {
        m_highLightMode = value;
        m_isHightLightModeChanged = true;
    }
}

void NCriticalPathParameters::setPathType(const std::string& value)
{
    if (m_pathType != value) {
        m_pathType = value;
        m_isPathListConfigChanged = true;
    }
}

void NCriticalPathParameters::setPathDetailLevel(const std::string& value)
{
    if (m_pathDetailLevel != value) {
        m_pathDetailLevel = value;
        m_isPathListConfigChanged = true;
    }
}

void NCriticalPathParameters::setCriticalPathNum(int value)
{
    if (m_criticalPathNum != value) {
        m_criticalPathNum = value;
        m_isPathListConfigChanged = true;
    }
}

void NCriticalPathParameters::setFlatRouting(bool value)
{
    if (m_isFlatRouting != value) {
        m_isFlatRouting = value;
        m_isFlatRoutingChanged = true;
    }
}

void NCriticalPathParameters::saveOptionSavePathListSettingsExplicitly(bool value)
{
    if (m_isSavePathListSettingsEnabled) {
        m_isSavePathListSettingsEnabled = value;
        nlohmann::json json;
        if (loadFromFile(json)) {
            if (setBoolValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS, value)) {
                saveToFile(json);
            }
        }
    }
}

bool NCriticalPathParameters::setBoolValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, bool value)
{
    return setStringValue(json, category, subcategory, parameter, value ? "checked": "unchecked");
}

bool NCriticalPathParameters::setIntValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, int value)
{
    std::string strValue{std::to_string(value)};
    return setStringValue(json, category, subcategory, parameter, strValue);
}

bool NCriticalPathParameters::setStringValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value)
{
    if (json[category][subcategory][parameter][KEY_USER_VALUE] != value) {
        json[category][subcategory][parameter][KEY_USER_VALUE] = value;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::getBoolValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const
{
    std::string resultStr = getStringValue(json, category, subcategory, parameter);
    if (resultStr == "checked") {
        return true;
    } else if (resultStr == "unchecked") {
        return false;
    }

    return false;
}

int NCriticalPathParameters::getIntValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter) const
{
    int result = 0;

    std::string resultStr = getStringValue(json, category, subcategory, parameter);
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

std::string NCriticalPathParameters::getStringValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, bool forceReturnDefaultValue) const
{
    std::string result;

    auto category_it = json.find(category);
    if (category_it != json.end()) {
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
    nlohmann::json json;
    loadFromFile(json);

    setStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE, m_highLightMode);
    setStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_TYPE, m_pathType);
    setStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAMETER_TIMING_REPORT_DETAIL, m_pathDetailLevel);
    setIntValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAMETER_TIMING_REPORT_NPATHS, m_criticalPathNum);
    setBoolValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS, m_isSavePathListSettingsEnabled);
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
    setBoolValue(json, CATEGORY_IPA, SUBCATEGORY_FILTER, PARAMETER_SAVE_SETTINGS, m_isSaveFilterSettingsEnabled);
#endif
    setBoolValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAMETER_FLAT_ROUTING, m_isFlatRouting);

    saveToFile(json);

    resetChangedFlags();
}

void NCriticalPathParameters::saveToFile(const nlohmann::json& json)
{
    std::ofstream file(filePath());
    file << std::setw(4) << json << std::endl;
}

bool NCriticalPathParameters::loadFromFile()
{
    nlohmann::json json;
    if (loadFromFile(json)) {
        m_highLightMode = getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_HIGH_LIGHT_MODE);
        m_pathType = getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_TYPE);
        m_pathDetailLevel = getStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAMETER_TIMING_REPORT_DETAIL);
        m_criticalPathNum = getIntValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAMETER_TIMING_REPORT_NPATHS);
        m_isSavePathListSettingsEnabled = getBoolValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAMETER_SAVE_SETTINGS);
#ifdef ENABLE_FILTER_SAVE_SETTINGS_FEATURE
        m_isSaveFilterSettingsEnabled = getBoolValue(json, CATEGORY_IPA, SUBCATEGORY_FILTER, PARAMETER_SAVE_SETTINGS);
#endif
        m_isFlatRouting = getBoolValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAMETER_FLAT_ROUTING);
        return true;
    }
    return false;
}

bool NCriticalPathParameters::loadFromFile(nlohmann::json& json)
{
    try {
        if (std::filesystem::exists(filePath())) {
            nlohmann::json candidate_json;
            std::ifstream file(filePath());
            file >> candidate_json;

            resetChangedFlags();

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

void NCriticalPathParameters::resetChangedFlags()
{
    m_isPathListConfigChanged = false;
    m_isHightLightModeChanged = false;
    m_isFlatRoutingChanged = false;
}

std::filesystem::path NCriticalPathParameters::filePath()
{
    std::string fileName{NCRITICALPATH_INNER_NAME};
    fileName += ".json";
#ifdef STANDALONE_APP
    return fileName;
#else
    std::filesystem::path path = FOEDAG::QLSettingsManager::getInstance()->settings_json_filepath;
    path = path.remove_filename();
    return path / fileName;
#endif
}
