#include "ncriticalpathparameters.h"
#include "ncriticalpathmoduleinfo.h"
#include "simplelogger.h"

#include <fstream>
#include <iostream>

#include "../Compiler/QLSettingsManager.h"

NCriticalPathParameters::NCriticalPathParameters()
{
    loadFromFile();    
}

const std::vector<std::string>& NCriticalPathParameters::getHighLightAvailableOptions() const
{
    static std::vector<std::string> options = {"Crit Path Flylines",
                                               "Crit Path Flylines Delays",
                                               "Crit Path Routing",
                                               "Crit Path Routing Delays"};
    return options;
}

const std::vector<std::string>& NCriticalPathParameters::getPathDetailAvailableOptions()
{
    if (m_pathDetailsAvailableOptions.empty()) {
        nlohmann::json json;
        loadFromFile(json);
        std::vector<std::string> options = json[CATEGORY_VPR][SUBCATEGORY_ANALYSIS][PARAM_TIMING_REPORT_DETAIL][SUBP_OPTIONS];
        m_pathDetailsAvailableOptions = options;
    }
    return m_pathDetailsAvailableOptions;
}

const std::vector<std::string>& NCriticalPathParameters::getCritPathTypeAvailableOptions() const
{
    static std::vector<std::string> options = {KEY_SETUP_PATH_LIST, KEY_HOLD_PATH_LIST};
    return options;
}

void NCriticalPathParameters::validateDefaultValues(nlohmann::json& json)
{
    bool requireSave = false;

    /* PARAM_HIGH_LIGHT_MODE */
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, SUBP_HELP, "Crit path high light mode for Place and View Route")) { requireSave = true; }
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, SUBP_LABEL, PARAM_HIGH_LIGHT_MODE)) { requireSave = true; }
    if (setDefaultVector(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, SUBP_OPTIONS, getHighLightAvailableOptions())) { requireSave = true; }
    if (setDefaultStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE)) { requireSave = true; }
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, SUBP_WIDGET_TYPE, WIDGET_COMBOBOX)) { requireSave = true; }

    /* PARAM_TYPE */
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, SUBP_HELP, "Critical path type")) { requireSave = true; }
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, SUBP_LABEL, PARAM_TYPE)) { requireSave = true; }
    if (setDefaultVector(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, SUBP_OPTIONS, getCritPathTypeAvailableOptions())) { requireSave = true; }
    if (setDefaultStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, DEFAULT_VALUE_PATHLIST_PARAM_TYPE)) { requireSave = true; }
    if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, SUBP_WIDGET_TYPE, WIDGET_COMBOBOX)) { requireSave = true; }

    if (requireSave) {
        saveToFile(json);
    }
}

bool NCriticalPathParameters::setDefaultString(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, const std::string& value) const
{
    if (json[category][subcategory][parameter][subparameter] != value) {
        json[category][subcategory][parameter][subparameter] = value;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setDefaultStringUserValue(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& value) const
{
    // we set user value only if it's absent
    if (!hasValue(json, category, subcategory, parameter, SUBP_USER_VALUE)) {
        json[category][subcategory][parameter][SUBP_USER_VALUE] = value;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setDefaultVector(nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, const std::vector<std::string>& value) const
{
    if (json[category][subcategory][parameter][subparameter] != value) {
        json[category][subcategory][parameter][subparameter] = value;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setHighLightMode(const std::string& value)
{
    if (m_highLightMode != value) {
        m_highLightMode = value;
        m_isHightLightModeChanged = true;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setPathType(const std::string& value)
{
    if (m_pathType != value) {
        m_pathType = value;
        m_isPathListConfigChanged = true;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setPathDetailLevel(const std::string& value)
{
    if (m_pathDetailLevel != value) {
        m_pathDetailLevel = value;
        m_isPathListConfigChanged = true;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setCriticalPathNum(int value)
{
    if (m_criticalPathNum != value) {
        m_criticalPathNum = value;
        m_isPathListConfigChanged = true;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::setFlatRouting(bool value)
{
    if (m_isFlatRouting != value) {
        m_isFlatRouting = value;
        m_isFlatRoutingChanged = true;
        return true;
    }
    return false;
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
    if (json[category][subcategory][parameter][SUBP_USER_VALUE] != value) {
        json[category][subcategory][parameter][SUBP_USER_VALUE] = value;
        return true;
    }
    return false;
}

bool NCriticalPathParameters::getBoolValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, bool& result) const
{
    std::string resultStr;
    if (getStringValue(json, category, subcategory, parameter, subparameter, resultStr)) {
        if (resultStr == "checked") {
            result = true;
            return true;
        } else if (resultStr == "unchecked") {
            result = false;
            return true;
        }
    }

    return false;
}

bool NCriticalPathParameters::getIntValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, int& result) const
{
    std::string resultStr;
    if (getStringValue(json, category, subcategory, parameter, subparameter, resultStr)) {
        try {
            result = std::atoi(resultStr.c_str());
            return true;
        } catch(...) {
            SimpleLogger::instance().error("cannot convert", resultStr.c_str(), "value for", category.c_str(), subcategory.c_str(), parameter.c_str(), subparameter.c_str());
        }
    }
    return false;
}

bool NCriticalPathParameters::hasValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter) const
{
    std::string result_unused;
    return getStringValue(json, category, subcategory, parameter, subparameter, result_unused);
}

bool NCriticalPathParameters::getStringValue(const nlohmann::json& json, const std::string& category, const std::string& subcategory, const std::string& parameter, const std::string& subparameter, std::string& result) const
{
    auto category_it = json.find(category);
    if (category_it != json.end()) {
        auto subcategory_it = category_it->find(subcategory);
        if (subcategory_it != category_it->end()) {
            auto parameter_it = subcategory_it->find(parameter);
            if (parameter_it != subcategory_it->end()) {
                auto value_it = parameter_it->find(subparameter);
                if ((value_it != parameter_it->end())) {
                    result = value_it->get<std::string>();
                    return true;
                }
            }
        }
    }

    return false;
}

bool NCriticalPathParameters::saveToFile()
{
    nlohmann::json json;
    loadFromFile(json);

    bool hasChanges = false;

    if (setStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, m_highLightMode)) { hasChanges = true; }
    if (setStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, m_pathType)) { hasChanges = true; }
    if (setStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAM_TIMING_REPORT_DETAIL, m_pathDetailLevel)) { hasChanges = true; }
    if (setIntValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAM_TIMING_REPORT_NPATHS, m_criticalPathNum)) { hasChanges = true; }
    if (setBoolValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAM_FLAT_ROUTING, m_isFlatRouting)) { hasChanges = true; }

    if (hasChanges) {
        saveToFile(json);
        return true;
    }

    return false;
}

void NCriticalPathParameters::saveToFile(const nlohmann::json& json)
{
    std::ofstream file(getFilePath());
    file << std::setw(4) << json << std::endl;
}

bool NCriticalPathParameters::loadFromFile()
{
    nlohmann::json json;
    if (loadFromFile(json)) {
        if (!m_isDefaultValuesChecked) {
            validateDefaultValues(json);
            m_isDefaultValuesChecked = true;
        }

        bool hasChanges = false;

        if (std::string candidate; getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE, SUBP_USER_VALUE, candidate)) { 
            if (setHighLightMode(candidate)) {
                hasChanges = true;
            }
        }
        if (std::string candidate; getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE, SUBP_USER_VALUE, candidate)) { 
            if (setPathType(candidate)) {
                hasChanges = true;
            }
        }
        if (std::string candidate; getStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAM_TIMING_REPORT_DETAIL, SUBP_USER_VALUE, candidate)) {
            if (setPathDetailLevel(candidate)) {
                hasChanges = true;
            }
        }
        if (int candidate; getIntValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS, PARAM_TIMING_REPORT_NPATHS, SUBP_USER_VALUE, candidate)) { 
            if (setCriticalPathNum(candidate)) {
                hasChanges = true;
            }
        }
        if (bool candidate; getBoolValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAM_FLAT_ROUTING, SUBP_USER_VALUE, candidate)) { 
            if (setFlatRouting(candidate)) {
                hasChanges = true;
            }
        }

        return hasChanges;
    }
    return false;
}

bool NCriticalPathParameters::loadFromFile(nlohmann::json& json) const
{
    try {
        if (std::filesystem::exists(getFilePath())) {
            nlohmann::json candidate_json;
            std::ifstream file(getFilePath());
            file >> candidate_json;

            // if we succesfully read json
            json.clear();
            std::swap(json, candidate_json);
            return true;
        } else {
            SimpleLogger::instance().error("unable to find", getFilePath().c_str(), "to load");
            return false;
        }
    } catch(...) {
        SimpleLogger::instance().error("unable to load", getFilePath().c_str());
        return false;
    }
}

void NCriticalPathParameters::resetChangedFlags()
{
    m_isPathListConfigChanged = false;
    m_isHightLightModeChanged = false;
    m_isFlatRoutingChanged = false;
}

std::filesystem::path NCriticalPathParameters::getFilePath()
{
    return FOEDAG::QLSettingsManager::getInstance()->settings_json_filepath;
}
