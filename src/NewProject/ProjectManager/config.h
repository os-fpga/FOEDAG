#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QObject>
#include <QSet>
#include <filesystem>

namespace FOEDAG {

class Config : public QObject {
  Q_OBJECT

 public:
  static Config *Instance();

  int InitConfigs(const QStringList &devicexmlList);
  QStringList getDeviceItem() const;
  QStringList getSerieslist() const;
  QStringList getFamilylist(const QString &series) const;
  QStringList getPackagelist(const QString &series,
                             const QString &family) const;
  QList<QStringList> getDevicelist(QString series = "", QString family = "",
                                   QString package = "") const;
  void dataPath(const std::filesystem::path &path) { m_dataPath = path; }
  std::filesystem::path dataPath() const { return m_dataPath; }
  void executable(const std::string &exe);
  std::filesystem::path userSpacePath() const;
  std::filesystem::path layoutsPath() const;
  std::filesystem::path customDeviceXml() const;

 private:
  QStringList m_device_xml{};
  QStringList m_lsit_device_item;
  QMap<QString, QMap<QString, QStringList>> m_map_device;
  QMap<QString, QStringList> m_map_device_info;

  void MakeDeviceMap(QString series, QString family, QString package);
  int InitConfig(const QString &devicexml);
  void clear();
  std::filesystem::path m_dataPath;
  std::string m_executable{};
};
}  // namespace FOEDAG
#endif  // CONFIG_H
