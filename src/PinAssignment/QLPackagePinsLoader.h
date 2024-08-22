#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

#include "PackagePinsLoader.h"

namespace FOEDAG {

class QLPackagePinsLoader : public PackagePinsLoader {
  const QString COLUMN_ORIENTATION = "orientation";
  const QString COLUMN_PORT_NAME = "port_name";
  const QString COLUMN_MAPPED_PIN = "mapped_pin";

 public:
  QLPackagePinsLoader(PackagePinsModel *model, QObject *parent = nullptr);
  std::pair<bool, QString> load(const QString &fileName) override final;

private:
  void initHeader();
  void parseHeader(const QString &header);

  QMap<QString, int> m_header;
};

}  // namespace FOEDAG
