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
#include "PortsModel.h"

namespace FOEDAG {

PortsModel::PortsModel(QObject *parent)
    : QObject(parent), m_model(new QStringListModel(this)) {}

QStringList PortsModel::headerList() const {
  return {"Name", "Dir", "Package Pin", "Type", "Range"};
}

void PortsModel::append(const IOPortGroup &p) { m_ioPorts.append(p); }

const QVector<IOPortGroup> &PortsModel::ports() const { return m_ioPorts; }

void PortsModel::initListModel() {
  QStringList portsList;
  portsList.append(QString());
  for (const auto &group : qAsConst(m_ioPorts))
    for (const auto &p : qAsConst(group.ports)) portsList.append(p.name);
  m_model->setStringList(portsList);
}

QStringListModel *PortsModel::listModel() const { return m_model; }

void PortsModel::insert(const QString &name, const QModelIndex &index) {
  m_indexes.insert(name, index);
}

void PortsModel::itemChange(const QString &name, const QString &newPin) {
  emit itemHasChanged(m_indexes.value(name), newPin);
}
}  // namespace FOEDAG
