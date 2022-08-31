/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "PackagePinsLoader.h"

#include <QFile>

namespace FOEDAG {

PackagePinsLoader::PackagePinsLoader(PackagePinsModel *model, QObject *parent)
    : QObject(parent), m_model(model) {}

PackagePinsLoader::~PackagePinsLoader() {}

bool PackagePinsLoader::load(const QString &fileName) {
  if (!m_model) return false;

  QFile file{fileName};
  if (!file.open(QFile::ReadOnly)) {
    return false;
  }

  QString content = file.readAll();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QStringList lines = content.split("\n", Qt::SkipEmptyParts);
#else
  QStringList lines = content.split("\n", QString::SkipEmptyParts);
#endif
  lines.pop_front();  // header
  PackagePinGroup group{};
  for (const auto &line : lines) {
    QStringList data = line.split(",");
    if (!data.first().isEmpty()) {
      if (!group.name.isEmpty() && (group.name != data.first())) {
        m_model->append(group);
        group.pinData.clear();
      }
      group.name = data.first();
    }
    data.pop_front();
    if (!group.pinData.isEmpty()) {
      if (group.pinData.last().data.count() > PinName && data.count() > PinName)
        if (group.pinData.last().data.at(PinName) == data.at(PinName)) continue;
    }
    group.pinData.append({data});
  }
  m_model->append(group);  // append last
  m_model->initListModel();

  return true;
}

}  // namespace FOEDAG
