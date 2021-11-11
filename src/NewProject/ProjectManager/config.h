#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QObject>
#include <QSet>

class Config : public QObject {
  Q_OBJECT

 public:
  static Config *Instance();

  void InitConfig(const QString &devicexml);
  QList<QString> getDeviceItem() const;
  QList<QString> getSerieslist() const;
  QList<QString> getFamilylist(const QString &series) const;
  QList<QString> getPackagelist(const QString &series,
                                const QString &family) const;
  QList<QList<QString>> getDevicelist(QString series = "", QString family = "",
                                      QString package = "") const;

 private:
  QString m_device_xml;
  QList<QString> m_lsit_device_item;
  QMap<QString, QMap<QString, QSet<QString>>> m_map_device;
  QMap<QString, QList<QString>> m_map_device_info;

  void MakeDeviceMap(QString series, QString family, QString package);
};

#endif  // CONFIG_H
