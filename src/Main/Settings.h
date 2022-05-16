// sma find where to get current copyright text from

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QJsonObject>
#include <QJsonValue>

namespace FOEDAG {

class Settings {
 private:
  QJsonObject m_data;

 public:
  Settings();
  void setValue(QJsonObject& parent, const QString& key,
                const QJsonValue& value);
  void setValue(const QString& key, const QJsonValue& value);
  void setString(const QString& key, const QString& string);
  void updateJson(const QString& key, QJsonObject& newJson);
  void updateJson(QJsonObject& parent, const QString& key,
                  QJsonObject& newJson);

  QJsonValue get(QJsonObject& parent, const QString& key);
  QJsonValue get(const QString& key);
  QJsonValue getNested(QJsonObject& parent, const QString& keyPath,
                       const QString& pathSeparator = ".");
  QJsonValue getNested(const QString& keyPath,
                       const QString& pathSeparator = ".");
  QString getJsonStr(const QJsonObject& object);
  QString getJsonStr();

  void loadJsonFile(const QString& filePath, const QString& key);
  void loadJsonFile(QJsonObject& parent, const QString& filePath,
                    const QString& key);
};

}  // namespace FOEDAG

#endif