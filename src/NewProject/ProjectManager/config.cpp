#include "config.h"

#include <QDomDocument>
#include <QFile>
#include <QTextStream>

using namespace FOEDAG;

Q_GLOBAL_STATIC(Config, config)

Config *Config::Instance() { return config(); }

int Config::InitConfig(const QString &devicexml) {
  int ret = 0;
  if ("" != devicexml && devicexml == m_device_xml) {
    return ret;
  } else {
    m_lsit_device_item.clear();
    m_map_device.clear();
    m_map_device_info.clear();
    m_device_xml = devicexml;
  }

  QFile file(devicexml);
  if (!file.open(QFile::ReadOnly)) {
    return -1;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return -2;
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  if (!node.isNull() && node.isElement()) {
    m_lsit_device_item.append("Name");
    m_lsit_device_item.append("Pin Count");
    m_lsit_device_item.append("Speed Grade");
    m_lsit_device_item.append("Core Voltage");
    QDomElement e = node.toElement();

    QDomNodeList list = e.childNodes();
    for (int i = 0; i < list.count(); i++) {
      QDomNode n = list.at(i);
      if (!n.isNull() && n.isElement()) {
        if (n.nodeName() == "resource") {
          QString type = n.toElement().attribute("type");
          QString label = n.toElement().attribute("label");
          if (label == "") {
            label = type;
          }
          m_lsit_device_item.append(label);
        }
      }
    }
    m_lsit_device_item.append("Series");
    m_lsit_device_item.append("Family");
    m_lsit_device_item.append("Package");
  }

  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      QStringList devlist;
      QString name = e.attribute("name");
      devlist.append(name);
      devlist.append(e.attribute("pin_count"));
      devlist.append(e.attribute("speedgrade"));
      devlist.append(e.attribute("core_voltage"));

      QDomNodeList list = e.childNodes();
      for (int i = 0; i < list.count(); i++) {
        QDomNode n = list.at(i);
        if (!n.isNull() && n.isElement()) {
          if (n.nodeName() == "resource")
            devlist.append(n.toElement().attribute("num"));
        }
      }
      QString series = e.attribute("series");
      QString family = e.attribute("family");
      QString package = e.attribute("package");
      devlist.append(series);
      devlist.append(family);
      devlist.append(package);

      // adding name to avoid key collisions when there are multiple devices
      // with the same series/family/package
      QString key = series + family + package + "_" + name;
      m_map_device_info.insert(key, devlist);
      MakeDeviceMap(series, family, package);
    }

    node = node.nextSibling();
  }
  return ret;
}

QStringList Config::getDeviceItem() const { return m_lsit_device_item; }

void Config::MakeDeviceMap(QString series, QString family, QString package) {
  QMap<QString, QStringList> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
    if (!mapfamily[family].contains(package)) mapfamily[family].append(package);
  } else {
    QStringList listpkg;
    listpkg.append(package);
    mapfamily.insert(family, listpkg);
  }
  m_map_device.insert(series, mapfamily);
}

QStringList Config::getSerieslist() const { return m_map_device.keys(); }

QStringList Config::getFamilylist(const QString &series) const {
  QMap<QString, QStringList> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
  }
  return mapfamily.keys();
}

QStringList Config::getPackagelist(const QString &series,
                                   const QString &family) const {
  QMap<QString, QStringList> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
  }

  QStringList listpkg;
  auto iterf = mapfamily.find(family);
  if (iterf != mapfamily.end()) {
    listpkg = iterf.value();
  }

  return listpkg;
}

QList<QStringList> Config::getDevicelist(QString series, QString family,
                                         QString package) const {
  QList<QStringList> listdevice;
  QString strkey = series + family + package;
  QList<QString> listkey = m_map_device_info.keys();
  for (int i = 0; i < listkey.size(); ++i) {
    if (listkey.at(i).contains(strkey, Qt::CaseSensitive)) {
      listdevice.append(m_map_device_info[listkey.at(i)]);
    }
  }
  return listdevice;
}
