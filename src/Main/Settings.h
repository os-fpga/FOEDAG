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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QVector>
#include <filesystem>

#include "nlohmann_json/json.hpp"
// Per https://json.nlohmann.me/features/object_order/
// Json order is undefined in the JSON standard. As such, the developer is given
// the option to use alphabetically sorted json w/ nlohmann::json or insertion
// order preserved json with nlohmann::ordered_json.
// Since alphabetical order doesn't help a backend system and insertion order
// can have benefits when reading user defined json (for example the order of
// widgets when reading tasks json for creating settings dialogs)
// we will use nlohmann::ordered_json to preserve insertion order.
using json = nlohmann::ordered_json;

namespace FOEDAG {

class Settings : public QObject {
  Q_OBJECT
 private:
  json m_json;

 public:
  Settings();
  void clear();
  void loadSettings(const QStringList& jsonFiles);
  QString getJsonStr(const json& object);
  QString getJsonStr();
  static QString getLookupValue(
      const json& object, const QString& option,
      const QString& optionsArrayKey = "options",
      const QString& lookupArrayKey = "optionsLookup");
  QString getSystemDefaultSettingsDir();
  static void traverseJson(json& obj,
                           std::function<void(json&, QString)> visitFn,
                           QString path = QString());
  static QString getUserSettingsPath();

  void loadJsonFile(const QString& filePath);
  void loadJsonFile(json* jsonObject, const QString& filePath);
  void applyTclVars();
  QStringList getSettingsJsonPtrPaths(bool includeHiddenSettings = true);
  static QStringList getSettingsJsonPtrPaths(json& jsonData,
                                             bool includeHiddenSettings = true);
  static QString getTclArgString(json& jsonData);

  json& getJson() { return m_json; }

  void syncWith(const QString& task);
  static QString Config(const std::filesystem::path& path, const QString& group,
                        const QString& key);

 signals:
  void sync(const QString& task);

 private:
  QVector<QString> m_syncTasks{};
};

}  // namespace FOEDAG

#endif
