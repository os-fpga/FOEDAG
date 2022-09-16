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
#include "PackagePinsModel.h"

namespace FOEDAG {

PackagePinsModel::PackagePinsModel(QObject *parent)
    : QObject(parent), m_listModel(new QStringListModel) {}

const QVector<HeaderData> &PackagePinsModel::header() const { return m_header; }

void PackagePinsModel::appendHeaderData(const HeaderData &h) {
  m_header.append(h);
}

void PackagePinsModel::append(const PackagePinGroup &g) { m_pinData.append(g); }

const QVector<PackagePinGroup> &PackagePinsModel::pinData() const {
  return m_pinData;
}

QStringListModel *PackagePinsModel::listModel() const { return m_listModel; }

void PackagePinsModel::initListModel() {
  QStringList pinsList;
  pinsList.append(QString());
  for (const auto &group : m_pinData) {
    for (const auto &pin : group.pinData) {
      pinsList.append(pin.data.at(PinName));
    }
  }
  m_listModel->setStringList(pinsList);
}

void PackagePinsModel::insert(const QString &name, const QModelIndex &index) {
  m_indexes.insert(name, index);
}

void PackagePinsModel::itemChange(const QString &name, const QString &pin) {
  emit itemHasChanged(m_indexes.value(name), pin);
}

}  // namespace FOEDAG
