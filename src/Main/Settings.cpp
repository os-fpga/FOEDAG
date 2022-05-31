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

#include "NewProject/ProjectManager/config.h"

using namespace FOEDAG;

#define SETTINGS_DEBUG false
auto DBG_PRINT = [](std::string printStr) {
  if (SETTINGS_DEBUG) {
    std::cout << printStr << std::flush;
  }
};

Settings::Settings() {}

void Settings::clear() {
  m_json.clear();
  DBG_PRINT("Settings: Cleared\n");
}

void Settings::loadSettings(const QStringList& jsonFiles) {
  for (const QString& filepath : jsonFiles) {
    loadJsonFile(filepath);
  }
}

QString Settings::getJsonStr(const json& object) {
  return QString::fromStdString(object.dump());
}

QString Settings::getJsonStr() { return getJsonStr(m_json); }

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

void Settings::loadJsonFile(json* jsonObject, const QString& filePath) {
  QFile jsonFile;
  jsonFile.setFileName(filePath);
  if (jsonFile.exists() &&
      jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read/parse json from file and update the passed jsonObject w/ new vals
    QString jsonStr = jsonFile.readAll();

    DBG_PRINT("Settings: Loading " + filePath.toStdString() + "\n");
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
