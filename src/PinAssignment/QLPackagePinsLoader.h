#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "PackagePinsLoader.h"

namespace FOEDAG {

class QLPackagePinsLoader : public PackagePinsLoader {
  const int BALLID_COLUMN = 4;
  const int BALLNAME_COLUMN = 5;
  const int INTERNAL_PINNAME_COLUMN = 6;
 public:
  QLPackagePinsLoader(PackagePinsModel *model, QObject *parent = nullptr);
  std::pair<bool, QString> load(const QString &fileName) override final;
};

}  // namespace FOEDAG
