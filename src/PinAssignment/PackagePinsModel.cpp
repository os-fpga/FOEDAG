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
    : QObject(parent),
      m_listModel(new QStringListModel),
      m_modeModelTx(new QStringListModel),
      m_modeModelRx(new QStringListModel) {}

const QVector<HeaderData> &PackagePinsModel::header() const { return m_header; }

void PackagePinsModel::appendHeaderData(const HeaderData &h) {
  m_header.append(h);
}

void PackagePinsModel::updateMode(const QString &pin, const QString &mode) {
  if (mode.isEmpty())
    m_modeMap.remove(pin);
  else
    m_modeMap.insert(pin, mode);
}

void PackagePinsModel::append(const PackagePinGroup &g) { m_pinData.append(g); }

const QVector<PackagePinGroup> &PackagePinsModel::pinData() const {
  return m_pinData;
}

const QMap<QString, QString> &PackagePinsModel::modeMap() const {
  return m_modeMap;
}

QStringListModel *PackagePinsModel::listModel() const { return m_listModel; }

QStringListModel *PackagePinsModel::modeModelTx() const {
  return m_modeModelTx;
}

QStringListModel *PackagePinsModel::modeModelRx() const {
  return m_modeModelRx;
}

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

const QVector<QString> &PackagePinsModel::userGroups() const {
  return m_userGroups;
}

void PackagePinsModel::appendUserGroup(const QString &userGroup) {
  m_userGroups.append(userGroup);
}

}  // namespace FOEDAG
