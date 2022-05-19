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
    m_lsit_device_item.append("name");
    m_lsit_device_item.append("pin_count");
    m_lsit_device_item.append("speedgrade");
    m_lsit_device_item.append("core_voltage");
    QDomElement e = node.toElement();

    QDomNodeList list = e.childNodes();
    for (int i = 0; i < list.count(); i++) {
      QDomNode n = list.at(i);
      if (!n.isNull() && n.isElement()) {
        if (n.nodeName() == "resource")
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

      QStringList devlist;
      devlist.append(e.attribute("name"));
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
      m_map_device_info.insert(series + family + package, devlist);
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
    mapfamily[family].append(package);
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
