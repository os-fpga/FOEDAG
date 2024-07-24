#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "PackagePinsLoader.h"

namespace FOEDAG {

class QLPackagePinsLoader : public PackagePinsLoader {
  enum {
    COLUMN_ORIENTATION=0,
    COLUMN_ROW,
    COLUMN_COL,
    COLUMN_PIN_NUMBER_IN_CELL,
    COLUMN_PORT_NAME,
    COLUMN_MAPPED_PIN,
    COLUMN_NETLIST_NAME,
    COLUMN_GPIO_TYPE
  };

 public:
  QLPackagePinsLoader(PackagePinsModel *model, QObject *parent = nullptr);
  std::pair<bool, QString> load(const QString &fileName) override final;

private:
  void initHeader();
};

}  // namespace FOEDAG
