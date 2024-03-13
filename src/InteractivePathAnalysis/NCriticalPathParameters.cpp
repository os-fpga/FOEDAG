/**
  * @file NCriticalPathParameters.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathParameters.h"

#include <fstream>
#include <iostream>

#include "NCriticalPathModuleInfo.h"
#include "SimpleLogger.h"
#include "client/ConvertUtils.h"

namespace FOEDAG {

NCriticalPathParameters::NCriticalPathParameters(
    const std::filesystem::path& settingsFilePath)
    : m_settingsFilePath(settingsFilePath) {
  if (!std::filesystem::exists(settingsFilePath)) {
    std::ofstream out(settingsFilePath);
    if (out.is_open()) {
      out << "{}";  // we need to write dummy json content
      out.close();
    }
  }

  loadFromFile();
}

const std::vector<std::string>&
NCriticalPathParameters::getHighLightAvailableOptions() const {
  static std::vector<std::string> options = {
      "crit path flylines", "crit path flylines delays", "crit path routing",
      "crit path routing delays"};
  return options;
}

const std::vector<std::string>&
NCriticalPathParameters::getPathDetailAvailableOptions() {
  if (m_pathDetailsAvailableOptions.empty()) {
    if (nlohmann::json json; tryLoadFromFile(json)) {
      std::vector<std::string> options =
          json[CATEGORY_VPR][SUBCATEGORY_ANALYSIS][PARAM_TIMING_REPORT_DETAIL]
              [SUBP_OPTIONS];
      m_pathDetailsAvailableOptions = options;
    }
  }
  return m_pathDetailsAvailableOptions;
}

const std::vector<std::string>&
NCriticalPathParameters::getCritPathTypeAvailableOptions() const {
  static std::vector<std::string> options = {comm::KEY_SETUP_PATH_LIST,
                                             comm::KEY_HOLD_PATH_LIST};
  return options;
}

void NCriticalPathParameters::validateDefaultValues(nlohmann::json& json) {
  bool requireSave = false;

  /* PARAM_HIGH_LIGHT_MODE */
  if (setDefaultString(
          json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_HIGH_LIGHT_MODE,
          SUBP_HELP,
          "set critical path high light mode for Place&Route Viewer")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_HIGH_LIGHT_MODE, SUBP_LABEL,
                       PARAM_HIGH_LIGHT_MODE)) {
    requireSave = true;
  }
  if (setDefaultVector(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_HIGH_LIGHT_MODE, SUBP_OPTIONS,
                       getHighLightAvailableOptions())) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                                PARAM_HIGH_LIGHT_MODE,
                                DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE)) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_HIGH_LIGHT_MODE, SUBP_WIDGET_TYPE,
                       WIDGET_COMBOBOX)) {
    requireSave = true;
  }

  /* PARAM_TYPE */
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                       SUBP_HELP, "set critical path type")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                       SUBP_LABEL, PARAM_TYPE)) {
    requireSave = true;
  }
  if (setDefaultVector(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                       SUBP_OPTIONS, getCritPathTypeAvailableOptions())) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                                PARAM_TYPE,
                                DEFAULT_VALUE_PATHLIST_PARAM_TYPE)) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                       SUBP_WIDGET_TYPE, WIDGET_COMBOBOX)) {
    requireSave = true;
  }

  /* PARAM_LOG_TO_FILE */
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_ENABLE_LOG_TO_FILE, SUBP_HELP,
                       "enable logging to the 'interactive_path_analysis.log' "
                       "file located in the project directory")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_ENABLE_LOG_TO_FILE, SUBP_LABEL,
                       PARAM_ENABLE_LOG_TO_FILE)) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_ENABLE_LOG_TO_FILE, SUBP_TEXT, "")) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(
          json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_ENABLE_LOG_TO_FILE,
          stringifyBool(DEFAULT_VALUE_PATHLIST_IS_LOG_TO_FILE_ENABLED))) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_ENABLE_LOG_TO_FILE, SUBP_WIDGET_TYPE,
                       WIDGET_CHECKBOX)) {
    requireSave = true;
  }

  /* PARAM_DRAW_CRIT_PATH_CONTOUR */
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_DRAW_CRITICAL_PATH_CONTOUR, SUBP_HELP,
                       "draw the critical path contour if at least one element "
                       "of critical path is selected")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_DRAW_CRITICAL_PATH_CONTOUR, SUBP_LABEL,
                       PARAM_DRAW_CRITICAL_PATH_CONTOUR)) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_DRAW_CRITICAL_PATH_CONTOUR, SUBP_TEXT, "")) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(
          json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
          PARAM_DRAW_CRITICAL_PATH_CONTOUR,
          stringifyBool(DEFAULT_VALUE_PATHLIST_DRAW_PATH_CONTOUR))) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_DRAW_CRITICAL_PATH_CONTOUR, SUBP_WIDGET_TYPE,
                       WIDGET_CHECKBOX)) {
    requireSave = true;
  }

#ifdef TODO_IPA_MIGRATION_SETTINGS
  /* PARAM_TIMING_REPORT_DETAIL */
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_DETAIL, SUBP_HELP,
                       "controls the level of detail included in generated "
                       "timing reports")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_DETAIL, SUBP_LABEL,
                       PARAM_TIMING_REPORT_DETAIL)) {
    requireSave = true;
  }
  static std::vector<std::string> reportDetailsOptions = {
      "netlist", "aggregated", "detailed", "debug"};
  if (setDefaultVector(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_DETAIL, SUBP_OPTIONS,
                       reportDetailsOptions)) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                                PARAM_TIMING_REPORT_DETAIL, "netlist")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_DETAIL, SUBP_WIDGET_TYPE,
                       WIDGET_COMBOBOX)) {
    requireSave = true;
  }

  /* PARAM_TIMING_REPORT_NPATHS */
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_NPATHS, SUBP_HELP,
                       "set how many timing paths are reported (maximum)")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_NPATHS, SUBP_LABEL,
                       PARAM_TIMING_REPORT_NPATHS)) {
    requireSave = true;
  }
  if (setDefaultStringUserValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                                PARAM_TIMING_REPORT_NPATHS, "100")) {
    requireSave = true;
  }
  if (setDefaultString(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                       PARAM_TIMING_REPORT_NPATHS, SUBP_WIDGET_TYPE, "input")) {
    requireSave = true;
  }
#endif

  if (requireSave) {
    saveToFile(json);
  }
}

void NCriticalPathParameters::readToolTips(nlohmann::json& json) {
  getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                 PARAM_HIGH_LIGHT_MODE, SUBP_HELP, m_highLightModeToolTip);
  getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                 SUBP_HELP, m_pathTypeToolTip);
  getStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                 PARAM_TIMING_REPORT_DETAIL, SUBP_HELP,
                 m_pathDetailLevelToolTip);
  getStringValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                 PARAM_TIMING_REPORT_NPATHS, SUBP_HELP,
                 m_criticalPathNumToolTip);
  getStringValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAM_FLAT_ROUTING,
                 SUBP_HELP, m_isFlatRoutingToolTip);
  getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                 PARAM_ENABLE_LOG_TO_FILE, SUBP_HELP,
                 m_isLogToFileEnabledToolTip);
  getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                 PARAM_DRAW_CRITICAL_PATH_CONTOUR, SUBP_HELP,
                 m_isDrawCriticalPathContourEnabledToolTip);
}

bool NCriticalPathParameters::setDefaultString(nlohmann::json& json,
                                               const std::string& category,
                                               const std::string& subcategory,
                                               const std::string& parameter,
                                               const std::string& subparameter,
                                               const std::string& value) const {
  if (json[category][subcategory][parameter][subparameter] != value) {
    json[category][subcategory][parameter][subparameter] = value;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setDefaultStringUserValue(
    nlohmann::json& json, const std::string& category,
    const std::string& subcategory, const std::string& parameter,
    const std::string& value) const {
  // we set user value only if it's absent
  if (!hasValue(json, category, subcategory, parameter, SUBP_USER_VALUE)) {
    json[category][subcategory][parameter][SUBP_USER_VALUE] = value;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setDefaultVector(
    nlohmann::json& json, const std::string& category,
    const std::string& subcategory, const std::string& parameter,
    const std::string& subparameter,
    const std::vector<std::string>& value) const {
  if (json[category][subcategory][parameter][subparameter] != value) {
    json[category][subcategory][parameter][subparameter] = value;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setHighLightMode(const std::string& value) {
  if (m_highLightMode != value) {
    m_highLightMode = value;
    m_isHightLightModeChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setPathType(const std::string& value) {
  if (m_pathType != value) {
    m_pathType = value;
    m_isPathListConfigChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setPathDetailLevel(const std::string& value) {
  if (m_pathDetailLevel != value) {
    m_pathDetailLevel = value;
    m_isPathListConfigChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setCriticalPathNum(int value) {
  if (m_criticalPathNum != value) {
    m_criticalPathNum = value;
    m_isPathListConfigChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setIsFlatRouting(bool value) {
  if (m_isFlatRouting != value) {
    m_isFlatRouting = value;
    m_isFlatRoutingChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setIsLogToFileEnabled(bool value) {
  if (m_isLogToFileEnabled != value) {
    m_isLogToFileEnabled = value;
    m_isLogToFileChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setIsDrawCriticalPathContourEnabled(bool value) {
  if (m_isDrawCriticalPathContourEnabled != value) {
    m_isDrawCriticalPathContourEnabled = value;
    m_isDrawCriticalPathContourChanged = true;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::setBoolUserValue(nlohmann::json& json,
                                               const std::string& category,
                                               const std::string& subcategory,
                                               const std::string& parameter,
                                               bool value) {
  return setStringUserValue(json, category, subcategory, parameter,
                            value ? "checked" : "unchecked");
}

bool NCriticalPathParameters::setIntUserValue(nlohmann::json& json,
                                              const std::string& category,
                                              const std::string& subcategory,
                                              const std::string& parameter,
                                              int value) {
  std::string strValue{std::to_string(value)};
  return setStringUserValue(json, category, subcategory, parameter, strValue);
}

bool NCriticalPathParameters::setStringUserValue(nlohmann::json& json,
                                                 const std::string& category,
                                                 const std::string& subcategory,
                                                 const std::string& parameter,
                                                 const std::string& value) {
  if (json[category][subcategory][parameter][SUBP_USER_VALUE] != value) {
    json[category][subcategory][parameter][SUBP_USER_VALUE] = value;
    return true;
  }
  return false;
}

bool NCriticalPathParameters::getBoolValue(const nlohmann::json& json,
                                           const std::string& category,
                                           const std::string& subcategory,
                                           const std::string& parameter,
                                           const std::string& subparameter,
                                           bool& result) const {
  std::string resultStr;
  if (getStringValue(json, category, subcategory, parameter, subparameter,
                     resultStr)) {
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

bool NCriticalPathParameters::getIntValue(const nlohmann::json& json,
                                          const std::string& category,
                                          const std::string& subcategory,
                                          const std::string& parameter,
                                          const std::string& subparameter,
                                          int& result) const {
  std::string resultStr;
  if (getStringValue(json, category, subcategory, parameter, subparameter,
                     resultStr)) {
    if (std::optional<int> resultOpt = tryConvertToInt(resultStr)) {
      result = resultOpt.value();
      return true;
    } else {
      SimpleLogger::instance().error(
          "cannot convert", resultStr.c_str(), "value for", category.c_str(),
          subcategory.c_str(), parameter.c_str(), subparameter.c_str());
    }
  }
  return false;
}

bool NCriticalPathParameters::hasValue(const nlohmann::json& json,
                                       const std::string& category,
                                       const std::string& subcategory,
                                       const std::string& parameter,
                                       const std::string& subparameter) const {
  std::string result_unused;
  return getStringValue(json, category, subcategory, parameter, subparameter,
                        result_unused);
}

bool NCriticalPathParameters::getStringValue(const nlohmann::json& json,
                                             const std::string& category,
                                             const std::string& subcategory,
                                             const std::string& parameter,
                                             const std::string& subparameter,
                                             std::string& result) const {
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

bool NCriticalPathParameters::saveToFile() {
  if (nlohmann::json json; tryLoadFromFile(json)) {
    bool hasChanges = false;

    if (setStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                           PARAM_HIGH_LIGHT_MODE, m_highLightMode)) {
      hasChanges = true;
    }
    if (setStringUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                           m_pathType)) {
      hasChanges = true;
    }
    if (setStringUserValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                           PARAM_TIMING_REPORT_DETAIL, m_pathDetailLevel)) {
      hasChanges = true;
    }
    if (setIntUserValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                        PARAM_TIMING_REPORT_NPATHS, m_criticalPathNum)) {
      hasChanges = true;
    }
    if (setBoolUserValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE,
                         PARAM_FLAT_ROUTING, m_isFlatRouting)) {
      hasChanges = true;
    }
    if (setBoolUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                         PARAM_ENABLE_LOG_TO_FILE, m_isLogToFileEnabled)) {
      hasChanges = true;
    }
    if (setBoolUserValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                         PARAM_DRAW_CRITICAL_PATH_CONTOUR,
                         m_isDrawCriticalPathContourEnabled)) {
      hasChanges = true;
    }

    if (hasChanges) {
      saveToFile(json);
      return true;
    }
  }

  return false;
}

void NCriticalPathParameters::saveToFile(const nlohmann::json& json) {
  if (std::filesystem::exists(getFilePath())) {
    std::ofstream file(getFilePath());
    file << std::setw(4) << json << std::endl;
  }
}

bool NCriticalPathParameters::loadFromFile() {
  bool hasChanges = false;

  if (nlohmann::json json; tryLoadFromFile(json)) {
    if (!m_isDefaultValuesChecked) {
      validateDefaultValues(json);
      readToolTips(json);
      m_isDefaultValuesChecked = true;
    }

    if (std::string candidate;
        getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                       PARAM_HIGH_LIGHT_MODE, SUBP_USER_VALUE, candidate)) {
      if (setHighLightMode(candidate)) {
        hasChanges = true;
      }
    }
    if (std::string candidate;
        getStringValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST, PARAM_TYPE,
                       SUBP_USER_VALUE, candidate)) {
      if (setPathType(candidate)) {
        hasChanges = true;
      }
    }
    if (std::string candidate; getStringValue(
            json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
            PARAM_TIMING_REPORT_DETAIL, SUBP_USER_VALUE, candidate)) {
      if (setPathDetailLevel(candidate)) {
        hasChanges = true;
      }
    }
    if (int candidate;
        getIntValue(json, CATEGORY_VPR, SUBCATEGORY_ANALYSIS,
                    PARAM_TIMING_REPORT_NPATHS, SUBP_USER_VALUE, candidate)) {
      if (setCriticalPathNum(candidate)) {
        hasChanges = true;
      }
    }
    if (bool candidate;
        getBoolValue(json, CATEGORY_VPR, SUBCATEGORY_ROUTE, PARAM_FLAT_ROUTING,
                     SUBP_USER_VALUE, candidate)) {
      if (setIsFlatRouting(candidate)) {
        hasChanges = true;
      }
    }
    if (bool candidate;
        getBoolValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                     PARAM_ENABLE_LOG_TO_FILE, SUBP_USER_VALUE, candidate)) {
      if (setIsLogToFileEnabled(candidate)) {
        hasChanges = true;
      }
    }
    if (bool candidate; getBoolValue(json, CATEGORY_IPA, SUBCATEGORY_PATHLIST,
                                     PARAM_DRAW_CRITICAL_PATH_CONTOUR,
                                     SUBP_USER_VALUE, candidate)) {
      if (setIsDrawCriticalPathContourEnabled(candidate)) {
        hasChanges = true;
      }
    }
  }
  return hasChanges;
}

bool NCriticalPathParameters::tryLoadFromFile(nlohmann::json& json) const {
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
      SimpleLogger::instance().error("unable to open", getFilePath().c_str(),
                                     "to load");
      return false;
    }
  } catch (...) {
    SimpleLogger::instance().error("unable to load", getFilePath().c_str());
    return false;
  }
}

void NCriticalPathParameters::resetChangedFlags() {
  m_isPathListConfigChanged = false;
  m_isHightLightModeChanged = false;
  m_isFlatRoutingChanged = false;
  m_isLogToFileChanged = false;
  m_isDrawCriticalPathContourChanged = false;
}

}  // namespace FOEDAG