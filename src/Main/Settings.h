// sma find where to get current copyright text from

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonObject>
#include <QJsonValue>

namespace FOEDAG {

class Settings {
 private:
  Settings();

  static Settings* m_instance;
  QJsonObject m_data;

 public:
  static Settings* getInstance();
  static void setValue(QJsonObject& parent, const QString& key,
                       const QJsonValue& value);
  static void setValue(const QString& key, const QJsonValue& value);
  static void setString(const QString& key, const QString& string);
  static void updateJson(const QString& key, QJsonObject& newJson);
  static void updateJson(QJsonObject& parent, const QString& key,
                         QJsonObject& newJson);

  static QJsonValue get(QJsonObject& parent, const QString& key);
  static QJsonValue get(const QString& key);
  static QJsonValue getNested(QJsonObject& parent, const QString& keyPath,
                              const QString& pathSeparator = ".");
  static QJsonValue getNested(const QString& keyPath,
                              const QString& pathSeparator = ".");
  static QString getJsonStr(const QJsonObject& object);
  static QString getJsonStr();

  static void loadJsonFile(const QString& filePath, const QString& key);
  static void loadJsonFile(QJsonObject& parent, const QString& filePath,
                           const QString& key);
};

}  // namespace FOEDAG

#endif