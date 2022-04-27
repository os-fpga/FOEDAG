#include "Settings.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include "stdio.h"

using namespace FOEDAG;

Settings* Settings::m_instance = nullptr;
Settings::Settings() { fprintf(stderr, "Settings Created\n"); }

Settings* Settings::getInstance() {
  if (!m_instance) {
    m_instance = new Settings();
  }
  return m_instance;
}

void Settings::setValue(QJsonObject& parent, const QString& key,
                        const QJsonValue& value) {
  parent.insert(key, value);
}

void Settings::setValue(const QString& key, const QJsonValue& value) {
  setValue(getInstance()->m_data, key, value);
}

void Settings::setString(const QString& key, const QString& string) {
  QJsonValue value = QJsonValue(string);
  setValue(key, value);
}

void Settings::updateJson(const QString& key, QJsonObject& newJson) {
  updateJson(getInstance()->m_data, key, newJson);
}

void Settings::updateJson(QJsonObject& parent, const QString& key,
                          QJsonObject& newJson) {
  QJsonValue val = parent.value(key);

  // Change this value to an object if it isn't one already
  // Whatever the former value was will be lost
  if (!val.isObject()) {
    setValue(parent, key, QJsonObject{});
  }

  // Qt currently returns a value instead of a reference when calling toObject,
  // as such we can't updated nested JSON objects and will have to manually set
  // the new values after they are updated.
  // https://stackoverflow.com/a/29361151
  QJsonObject parentObj =
      parent.value(key).toObject();  // toObject call should be superfluous from
                                     // previous line

  // Step through each new key
  for (const QString& newKey : newJson.keys()) {
    QJsonValue val = parentObj.value(newKey);
    if (!val.isObject()) {
      QJsonValue newVal = newJson.value(newKey);
      if (newVal.isObject()) {
        QJsonObject obj = newVal.toObject();
        updateJson(parentObj, newKey, obj);
      } else {
        setValue(parentObj, newKey, newVal);
      }
    } else {
      // add QJsonObject recursion
      QJsonObject tempObj = newJson.value(newKey).toObject();
      updateJson(parentObj, newKey, tempObj);
      ;
    }
  }

  // Manually update json
  parent.insert(key, parentObj);
}

QJsonValue Settings::get(QJsonObject& parent, const QString& key) {
  return parent.value(key);
}

QJsonValue Settings::get(const QString& key) {
  return getInstance()->m_data.value(key);
}

QJsonValue Settings::getNested(QJsonObject& parent, const QString& keyPath,
                               const QString& pathSeparator) {
  QStringList keys = keyPath.split(pathSeparator);
  QJsonObject tempObj = parent;

  QJsonValue prevVal = QJsonValue();
  for (QString key : keys) {
    QJsonValue val = tempObj.value(key);
    if (!prevVal.isNull() && prevVal.isArray()) {
      QJsonArray array = prevVal.toArray();
      for (QJsonValue tempVal : array) {
        if (tempVal.toObject().contains(key)) {
          // sma might not need to check before grabbing?
          val = tempVal.toObject().value(key);
        }
      }
    } else if (val.isArray()) {
      // If this is an array, do nothing this for this loop and wait until we
      // read the next key
    } else {
      // convert the value back to an object for the next key read
      tempObj = tempObj.value(key).toObject();
    }

    // update our previous value in case we are working with an array
    prevVal = val;
  }

  return prevVal;
}

QJsonValue Settings::getNested(const QString& keyPath,
                               const QString& pathSeparator) {
  return getNested(getInstance()->m_data, keyPath, pathSeparator);
}

QString Settings::getJsonStr(const QJsonObject& object) {
  QJsonDocument jsonDoc(object);

  return QString(jsonDoc.toJson(QJsonDocument::Indented));
}

QString Settings::getJsonStr() { return getJsonStr(getInstance()->m_data); }

void Settings::loadJsonFile(const QString& filePath, const QString& key) {
  loadJsonFile(getInstance()->m_data, filePath, key);
}

void Settings::loadJsonFile(QJsonObject& parent, const QString& filePath,
                            const QString& key) {
  QFile jsonFile;
  jsonFile.setFileName(filePath);
  if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read JSON from file
    QString jsonStr = jsonFile.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QJsonObject jsonObj = jsonDoc.object();
    jsonFile.close();

    // Add this JSON object to the parent object under key
    updateJson(parent, key, jsonObj);
  } else {
    // SMA add proper error message
    qDebug() << "\n\n\n FILE READ ERROR: " << filePath << "\n\n\n";
  }
}