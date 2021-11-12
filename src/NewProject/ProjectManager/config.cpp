#include "config.h"

#include <QDomDocument>
#include <QFile>
#include <QTextStream>

Q_GLOBAL_STATIC(Config, config)

Config *Config::Instance() { return config(); }

void Config::InitConfig(const QString &devicexml) {
  if (devicexml == m_device_xml) {
    return;
  } else {
    m_lsit_device_item.clear();
    m_map_device.clear();
    m_map_device_info.clear();
    m_device_xml = devicexml;
  }

  QFile file(devicexml);
  if (!file.open(QFile::ReadOnly)) {
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return;
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  if (!node.isNull() && node.isElement()) {
    m_lsit_device_item.append("name");
    m_lsit_device_item.append("pin_count");
    m_lsit_device_item.append("speedgrade");
    m_lsit_device_item.append("core_voltage");
    QDomElement e = node.toElement();

    QDomNodeList list = e.childNodes();
    for (int i = 0; i < list.count(); i++) {
      QDomNode n = list.at(i);
      if (!n.isNull() && n.isElement()) {
        m_lsit_device_item.append(n.toElement().attribute("type"));
      }
    }
    m_lsit_device_item.append("series");
    m_lsit_device_item.append("family");
    m_lsit_device_item.append("package");
  }

  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      QList<QString> devlist;
      devlist.append(e.attribute("name"));
      devlist.append(e.attribute("pin_count"));
      devlist.append(e.attribute("speedgrade"));
      devlist.append(e.attribute("core_voltage"));

      QDomNodeList list = e.childNodes();
      for (int i = 0; i < list.count(); i++) {
        QDomNode n = list.at(i);
        if (!n.isNull() && n.isElement()) {
          devlist.append(n.toElement().attribute("num"));
        }
      }
      QString series = e.attribute("series");
      QString family = e.attribute("family");
      QString package = e.attribute("package");
      devlist.append(series);
      devlist.append(family);
      devlist.append(package);
      m_map_device_info.insert(series + family + package, devlist);
      MakeDeviceMap(series, family, package);
    }

    node = node.nextSibling();
  }
}

QList<QString> Config::getDeviceItem() const { return m_lsit_device_item; }

void Config::MakeDeviceMap(QString series, QString family, QString package) {
  QMap<QString, QSet<QString>> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
    mapfamily[family].insert(package);
  } else {
    QSet<QString> setpkg;
    setpkg.insert(package);
    mapfamily.insert(family, setpkg);
  }
  m_map_device.insert(series, mapfamily);
}

QList<QString> Config::getSerieslist() const { return m_map_device.keys(); }

QList<QString> Config::getFamilylist(const QString &series) const {
  QMap<QString, QSet<QString>> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
  }
  return mapfamily.keys();
}

QList<QString> Config::getPackagelist(const QString &series,
                                      const QString &family) const {
  QMap<QString, QSet<QString>> mapfamily;
  auto iter = m_map_device.find(series);
  if (iter != m_map_device.end()) {
    mapfamily = iter.value();
  }

  QSet<QString> setpkg;
  auto iterf = mapfamily.find(family);
  if (iterf != mapfamily.end()) {
    setpkg = iterf.value();
  }

  return setpkg.values();
}

QList<QList<QString>> Config::getDevicelist(QString series, QString family,
                                            QString package) const {
  QList<QList<QString>> listdevice;
  QString strkey = series + family + package;
  QList<QString> listkey = m_map_device_info.keys();
  for (int i = 0; i < listkey.size(); ++i) {
    if (listkey.at(i).contains(strkey, Qt::CaseSensitive)) {
      listdevice.append(m_map_device_info[listkey.at(i)]);
    }
  }
  return listdevice;
}
