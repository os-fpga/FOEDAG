/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Settings.h"

#include <QFile>
#include <iostream>

#include "Foedag.h"
#include "Main/WidgetFactory.h"
#include "NewProject/ProjectManager/config.h"

using namespace FOEDAG;

#define SETTINGS_DEBUG false

auto SETTINGS_DBG_PRINT = [](std::string printStr) {
  if (SETTINGS_DEBUG) {
    std::cout << printStr << std::flush;
  }
};

Settings::Settings() { FOEDAG::initTclArgFns(); }

void Settings::clear() {
  m_json.clear();
  SETTINGS_DBG_PRINT("Settings: Cleared\n");
}

void Settings::loadSettings(const QStringList& jsonFiles) {
  for (const QString& filepath : jsonFiles) {
    loadJsonFile(filepath);
  }
  applyTclVars();
}

QString Settings::getJsonStr(const json& object) {
  return QString::fromStdString(object.dump());
}

QString Settings::getJsonStr() { return getJsonStr(m_json); }

// This will look up 2 keys in the given object and treat them as string arrays.
// If option is found in the options array, its index will be used to retrieve a
// value from lookup. "" is returned if a value can't be found.
//
// Note this assumes json arrays are ordered, see Main/Settings.h for details
// about making sure your json interface is using nlohmann::ordered_json
QString Settings::getLookupValue(
    const json& object, const QString& option,
    const QString& optionsArrayKey /* "options" */,
    const QString& lookupArrayKey /* "optionsLookup" */) {
  // Convert json array stored under key to QStringList
  auto GetStrList = [object](const QString& key) {
    QStringList strings;
    // If the key exists
    if (object.contains(key.toStdString())) {
      json array = object[key.toStdString()];
      std::transform(array.begin(), array.end(), std::back_inserter(strings),
                     [](json val) -> QString {
                       return QString::fromStdString(val.get<std::string>());
                     });
    }
    return strings;
  };

  QStringList options = GetStrList(optionsArrayKey);
  QStringList lookups = GetStrList(lookupArrayKey);

  QString value;
  // Find the given option in the option array
  int idx = options.indexOf(option);
  if (idx > -1 && idx < lookups.count()) {
    // use the option index for a lookup if there are enough values
    value = lookups.at(idx);
  }

  return value;
}

QString Settings::getSystemDefaultSettingsDir() {
  const std::string separator(1, std::filesystem::path::preferred_separator);
  std::string settingsPath = Config::Instance()->dataPath().string() +
                             separator + std::string("etc") + separator +
                             std::string("settings") + separator;
  return QString::fromStdString(settingsPath);
}

void Settings::loadJsonFile(const QString& filePath) {
  loadJsonFile(&m_json, filePath);
}

// This will recursively traverse a json tree, calling visitFn on each node
// while storing the current path in path. The path can be used to create a
// nlohmann::json_pointer which can be used to access the given node after this
// traversal has finished
void Settings::traverseJson(json& obj,
                            std::function<void(json&, QString)> visitFn,
                            QString path /* QString() */) {
  visitFn(obj, path);
  if (obj.type() == nlohmann::detail::value_t::array) {
    for (int i = 0; i < obj.size(); i++) {
      QString childPath = path + "/" + QString::number(i);
      traverseJson(obj.at(i), visitFn, childPath);
    }
  } else if (obj.type() == nlohmann::detail::value_t::object) {
    for (auto& item : obj.items()) {
      QString childPath = path + "/" + QString::fromStdString(item.key());
      traverseJson(obj[item.key()], visitFn, childPath);
    }
  }
}

QString Settings::getUserSettingsPath() {
  QString path;
  QString projPath =
      GlobalSession->GetCompiler()->ProjManager()->getProjectPath();
  QString projName =
      GlobalSession->GetCompiler()->ProjManager()->getProjectName();
  QString separator = QString::fromStdString(
      std::string(1, std::filesystem::path::preferred_separator));

  if (!projPath.isEmpty() && !projName.isEmpty()) {
    path = projPath + separator + projName + ".settings/";
  }

  return path;
}

void Settings::loadJsonFile(json* jsonObject, const QString& filePath) {
  QFile jsonFile;
  jsonFile.setFileName(filePath);
  if (jsonFile.exists() &&
      jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read/parse json from file and update the passed jsonObject w/ new vals
    QString jsonStr = jsonFile.readAll();

    SETTINGS_DBG_PRINT("Settings: Loading " + filePath.toStdString() + "\n\t" +
                       jsonStr.toStdString() + "\n");

    try {
      // Merge the json
      jsonObject->update(json::parse(jsonStr.toStdString()), true);
    } catch (json::parse_error& e) {
      // output exception information
      std::cerr << "Json Error: " << e.what() << '\n'
                << "filePath: " << filePath.toStdString() << "\n"
                << "byte position of error: " << e.byte << std::endl;
    }
  } else {
    std::cerr << "ERROR - Settings::loadJsonFile - Failed to read \""
              << filePath.toStdString() << "\"\n";
  }
}

// This will find any settings categories that have tclArgKey defined in their
// _META_ object, collect any default or user set values, and apply those values
// using the tcl setter associated with tclArgKey
void Settings::applyTclVars() {
  // Get json ptr paths to all settings objects
  QStringList paths = getSettingsJsonPtrPaths();

  json& topJson = getJson();

  // Step through each settings path and load tcl vars if possible
  for (QString path : paths) {
    // Get the json stored at this path
    json::json_pointer jsonPtr(path.toStdString());
    json& settingsJson = topJson.at(jsonPtr);

    // Look for a tclArgeKey
    QString tclArgKey;
    if (settingsJson.contains("_META_")) {
      tclArgKey =
          QString::fromStdString(settingsJson["_META_"].value("tclArgKey", ""));
    }

    // If this setting has a tclArgKey use it to lookup the related setter and
    // apply and
    if (!tclArgKey.isEmpty()) {
      auto [setter, getter] = FOEDAG::getTclArgFns(tclArgKey);
      if (setter != nullptr) {
        setter(getTclArgString(settingsJson).toStdString());
      } else {
        SETTINGS_DBG_PRINT("Settings: getTclArgFns for key \"" +
                           tclArgKey.toStdString() +
                           "\" returned a null setter function pointer. Back "
                           "end tcl values will not be set.\n");
      }
    }
  }
}

// This will step through the setting json finding any settings categories (an
// object w/ a "_META_" tag and "isSetting" set to true inside that _META_
// object) and return a QStringList of nlohman json ptrs to those objects
QStringList Settings::getSettingsJsonPtrPaths(
    bool includeHiddenSettings /*true*/) {
  return Settings::getSettingsJsonPtrPaths(getJson(), includeHiddenSettings);
}

// This will step through the given json finding any settings categories (an
// object w/ a "_META_" tag and "isSetting" set to true inside that _META_
// object) and return a QStringList of nlohman json ptrs to those objects
QStringList Settings::getSettingsJsonPtrPaths(
    json& jsonData, bool includeHiddenSettings /*true*/) {
  QStringList jsonPaths;

  // Callback to find and store paths to json objects that define settings
  auto findCb = [&jsonPaths, includeHiddenSettings](json& obj,
                                                    const QString& path) {
    // Check if the object has meta data and is a setting category
    if (obj.contains("_META_")) {
      bool isSetting = obj["_META_"].value("isSetting", false);
      bool hidden = obj["_META_"].value("hidden", true);

      // store the path if it's a setting we are interested in
      if (isSetting && (includeHiddenSettings || !hidden)) {
        jsonPaths << path;
      }
    }
  };

  // Find and return the setting paths
  traverseJson(jsonData, findCb);
  return jsonPaths;
}

// This will traverse a json object looking for an "arg" field, if it finds one
// it will then check the current json object for "userValue" and "default"
// fields which it will use to create a tcl arg list string from
QString Settings::getTclArgString(json& jsonData) {
  // Create a callback to collect and return tcl arg strings
  QString argStr;
  auto findCb = [&argStr](json& obj, const QString& path) {
    if (obj.type() == nlohmann::detail::value_t::object) {
      QString widgetType = QString::fromStdString(obj.value("widgetType", ""));

      // Check if the object has an "arg" field
      QString tclArg = QString::fromStdString(obj.value("arg", ""));
      if (!tclArg.isEmpty()) {
        // Create a safe json look up that will read the expected value type
        // based off the widgetType
        auto getValStr = [obj, widgetType](const std::string& key) -> QString {
          QString val;
          if (obj.contains(key)) {
            // The json library can cause some issues if you try to read a
            // number into a string so we add special handling for widget types
            // that can take numbers
            if (widgetType.toLower() == "doublespinbox") {
              val = QString::number(obj.value(key, 0.0));
            } else if (widgetType.toLower() == "spinbox") {
              val = QString::number(obj.value(key, 0));
            } else {
              val = QString::fromStdString(obj.value(key, ""));
            }
          }

          return val;
        };

        // Attempt to get a userValue and then a default if userValue isn't
        // provided
        QString val = getValStr("userValue");
        if (val.isEmpty()) {
          val = getValStr("default");
        }

        // If a userValue or default value was found
        if (val.isEmpty() == false) {
          QString widgetType =
              QString::fromStdString(obj.value("widgetType", ""));

          // Special handling for checkboxes which don't provide a value with
          // their arg, but instead use their arg as a flag (-arg)
          if (widgetType.toLower() == "checkbox") {
            // only a checked state matters, no arg passed implies unchecked
            if (val.toLower() == "checked") {
              argStr += " -" + tclArg;
            }
          } else {
            // for other types, just append like a normal arguement (-arg value)
            QString tclVal = Settings::getLookupValue(obj, val);
            if (tclVal.isEmpty()) {
              tclVal = val;
            }

            // we expect tclVal might be multi-space value like "-a val0 -b
            // val1" or "--some-arg=val" so need to convert spaces/dashes/new
            // lines.
            tclVal = convertAll(tclVal);

            argStr += " -" + tclArg + " " + tclVal;
          }
        }
      }
    }
  };

  // Build up and return arg string
  traverseJson(jsonData, findCb);
  return argStr;
}

void Settings::syncWith(const QString& task) {
  // use m_syncTasks vector to avoid cyclic dependencies, when two tasks try to
  // sync each other
  if (!m_syncTasks.contains(task)) {
    m_syncTasks.push_back(task);
    emit sync(task);
    if (int index = m_syncTasks.indexOf(task); index != -1)
      m_syncTasks.remove(index);
  }
}

QString Settings::Config(const std::filesystem::path& path,
                         const QString& group, const QString& key) {
  QFile jsonFile;
  jsonFile.setFileName(QString::fromStdString(path.string()));
  if (jsonFile.exists() &&
      jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read/parse json from file and update the passed jsonObject w/ new vals
    QString jsonStr = jsonFile.readAll();

    json jsonObject{};
    try {
      // Merge the json
      jsonObject.update(json::parse(jsonStr.toStdString()), true);
      return QString::fromStdString(
          jsonObject[group.toStdString()][key.toStdString()]);
    } catch (json::parse_error& e) {
      // output exception information
      std::cerr << "Json Error: " << e.what() << '\n'
                << "filePath: " << path.string() << "\n"
                << "byte position of error: " << e.byte << std::endl;
    } catch (std::exception& e) {
      std::cerr << "Json Error: " << e.what() << std::endl;
    }
  } else {
    std::cerr << "ERROR - Settings::loadJsonFile - Failed to read \""
              << path.string() << "\"\n";
  }
  return QString{};
}
