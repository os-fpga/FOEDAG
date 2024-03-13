/**
  * @file NCriticalPathParameters.h
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

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "NCriticalPathModuleInfo.h"
#include "client/CommConstants.h"
#include "nlohmann_json/json.hpp"

namespace FOEDAG {

/**
 * @brief Central data structure to keep parameters updated for the Interactive
 * Analysis Tool.
 *
 * It manages loading parameters from a file, saving to a file, and initializes
 * a settings file with default values in a JSON structure. The purpose of this
 * class is to maintain a single shared data copy among instances that require
 * it, such as creation requests for the server, displaying in the UI, etc.
 */
class NCriticalPathParameters {
  const char* UNCHECKED = "unchecked";

  const char* WIDGET_COMBOBOX = "dropdown";
  const char* WIDGET_CHECKBOX = "checkbox";

  const char* SUBP_HELP = "help";
  const char* SUBP_OPTIONS = "options";
  const char* SUBP_LABEL = "label";
  const char* SUBP_TEXT = "text";
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
  const char* PARAM_ENABLE_LOG_TO_FILE = "enable_log_to_file";
  const char* PARAM_DRAW_CRITICAL_PATH_CONTOUR = "draw_critical_path_contour";

  const char* DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE =
      "crit path flylines";
  const char* DEFAULT_VALUE_PATHLIST_PARAM_TYPE = comm::KEY_SETUP_PATH_LIST;
  const char* DEFAULT_VALUE_PATHLIST_PARAM_DETAIL_LEVEL = "netlist";
  const int DEFAULT_VALUE_PATHLIST_PARAM_MAX_NUM = 100;
  const bool DEFAULT_VALUE_PATHLIST_PARAM_IS_FLAT_ROUTING = false;
  const bool DEFAULT_VALUE_PATHLIST_IS_LOG_TO_FILE_ENABLED = false;
  const bool DEFAULT_VALUE_PATHLIST_DRAW_PATH_CONTOUR = true;

 public:
  NCriticalPathParameters(const std::filesystem::path& settingsFilePath);
  ~NCriticalPathParameters() = default;

  void setDrawCriticalPathContourEnabled(bool status) {
    m_isDrawCriticalPathContourEnabled = status;
  }
  bool isDrawCriticalPathContourEnabled() const {
    return m_isDrawCriticalPathContourEnabled;
  }

  const std::vector<std::string>& getHighLightAvailableOptions() const;
  const std::vector<std::string>& getPathDetailAvailableOptions();
  const std::vector<std::string>& getCritPathTypeAvailableOptions() const;

  bool isPathListConfigChanged() const { return m_isPathListConfigChanged; }
  bool isHightLightModeChanged() const { return m_isHightLightModeChanged; }
  bool isFlatRoutingChanged() const { return m_isFlatRoutingChanged; }
  bool isLogToFileChanged() const { return m_isLogToFileChanged; }
  bool isDrawCriticalPathContourChanged() const {
    return m_isDrawCriticalPathContourChanged;
  }

  const std::filesystem::path& getFilePath() const {
    return m_settingsFilePath;
  }

  bool hasChanges() const {
    return m_isHightLightModeChanged || m_isPathListConfigChanged ||
           m_isFlatRoutingChanged || m_isLogToFileChanged ||
           m_isDrawCriticalPathContourChanged;
  }

  bool saveToFile();
  bool loadFromFile();
  void resetChangedFlags();

  bool setHighLightMode(const std::string& value);
  bool setPathType(const std::string& value);
  bool setPathDetailLevel(const std::string& value);
  bool setCriticalPathNum(int value);
  bool setIsFlatRouting(bool value);
  bool setIsLogToFileEnabled(bool value);
  bool setIsDrawCriticalPathContourEnabled(bool value);

  const std::string& getHighLightMode() const { return m_highLightMode; }
  const std::string& getPathType() const { return m_pathType; }
  const std::string& getPathDetailLevel() const { return m_pathDetailLevel; }
  int getCriticalPathNum() const { return m_criticalPathNum; }
  bool getIsFlatRouting() const { return m_isFlatRouting; }
  bool getIsLogToFileEnabled() const { return m_isLogToFileEnabled; }
  bool getIsDrawCriticalPathContourEnabled() const {
    return m_isDrawCriticalPathContourEnabled;
  }

  const std::string& getHighLightModeToolTip() const {
    return m_highLightModeToolTip;
  }
  const std::string& getPathTypeToolTip() const { return m_pathTypeToolTip; }
  const std::string& getPathDetailLevelToolTip() const {
    return m_pathDetailLevelToolTip;
  }
  const std::string& getCriticalPathNumToolTip() const {
    return m_criticalPathNumToolTip;
  }
  const std::string& getIsFlatRoutingToolTip() const {
    return m_isFlatRoutingToolTip;
  }
  const std::string& getIsLogToFileEnabledToolTip() const {
    return m_isLogToFileEnabledToolTip;
  }
  const std::string& getIsDrawCriticalPathContourEnabledToolTip() const {
    return m_isDrawCriticalPathContourEnabledToolTip;
  }

 private:
  std::filesystem::path m_settingsFilePath;
  bool m_isDefaultValuesChecked = false;

  bool m_isPathListConfigChanged = false;
  bool m_isHightLightModeChanged = false;
  bool m_isFlatRoutingChanged = false;
  bool m_isLogToFileChanged = false;
  bool m_isDrawCriticalPathContourChanged = false;

  std::string m_highLightMode = DEFAULT_VALUE_PATHLIST_PARAM_HIGH_LIGHT_MODE;
  std::string m_pathType = DEFAULT_VALUE_PATHLIST_PARAM_TYPE;
  std::string m_pathDetailLevel = DEFAULT_VALUE_PATHLIST_PARAM_DETAIL_LEVEL;
  int m_criticalPathNum = DEFAULT_VALUE_PATHLIST_PARAM_MAX_NUM;
  bool m_isFlatRouting = DEFAULT_VALUE_PATHLIST_PARAM_IS_FLAT_ROUTING;
  bool m_isLogToFileEnabled = DEFAULT_VALUE_PATHLIST_IS_LOG_TO_FILE_ENABLED;
  bool m_isDrawCriticalPathContourEnabled =
      DEFAULT_VALUE_PATHLIST_DRAW_PATH_CONTOUR;

  std::string m_highLightModeToolTip;
  std::string m_pathTypeToolTip;
  std::string m_pathDetailLevelToolTip;
  std::string m_criticalPathNumToolTip;
  std::string m_isFlatRoutingToolTip;
  std::string m_isLogToFileEnabledToolTip;
  std::string m_isDrawCriticalPathContourEnabledToolTip;

  std::vector<std::string> m_pathDetailsAvailableOptions;

  void validateDefaultValues(nlohmann::json&);
  std::string stringifyBool(bool flag) {
    return flag ? "checked" : "unchecked";
  }

  bool setBoolUserValue(nlohmann::json& json, const std::string& category,
                        const std::string& subcategory,
                        const std::string& parameter, bool value);
  bool setIntUserValue(nlohmann::json& json, const std::string& category,
                       const std::string& subcategory,
                       const std::string& parameter, int value);
  bool setStringUserValue(nlohmann::json& json, const std::string& category,
                          const std::string& subcategory,
                          const std::string& parameter,
                          const std::string& value);

  bool getBoolValue(const nlohmann::json& json, const std::string& category,
                    const std::string& subcategory,
                    const std::string& parameter,
                    const std::string& subparameter, bool& result) const;
  bool getIntValue(const nlohmann::json& json, const std::string& category,
                   const std::string& subcategory, const std::string& parameter,
                   const std::string& subparameter, int& result) const;
  bool getStringValue(const nlohmann::json& json, const std::string& category,
                      const std::string& subcategory,
                      const std::string& parameter,
                      const std::string& subparameter,
                      std::string& result) const;

  bool hasValue(const nlohmann::json& json, const std::string& category,
                const std::string& subcategory, const std::string& parameter,
                const std::string& subparameter) const;

  bool tryLoadFromFile(nlohmann::json& json) const;
  void saveToFile(const nlohmann::json& json);

  bool setDefaultString(nlohmann::json& json, const std::string& category,
                        const std::string& subcategory,
                        const std::string& parameter,
                        const std::string& subparameter,
                        const std::string& value) const;
  bool setDefaultStringUserValue(nlohmann::json& json,
                                 const std::string& category,
                                 const std::string& subcategory,
                                 const std::string& parameter,
                                 const std::string& value) const;
  bool setDefaultVector(nlohmann::json& json, const std::string& category,
                        const std::string& subcategory,
                        const std::string& parameter,
                        const std::string& subparameter,
                        const std::vector<std::string>& value) const;

  void readToolTips(nlohmann::json& json);
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;

}  // namespace FOEDAG