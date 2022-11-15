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
#include "PinsBaseModel.h"

namespace FOEDAG {

PinsBaseModel::PinsBaseModel(QObject *parent) : QObject(parent) {}

bool PinsBaseModel::exists(const QString &port, const QString &pin) const {
  for (auto it = m_pinsMap.constBegin(); it != m_pinsMap.constEnd(); ++it) {
    if (it.key() == port && it.value().first == pin) return true;
  }
  return false;
}

void PinsBaseModel::update(const QString &port, const QString &pin, int index) {
  if (port.isEmpty()) return;
  if (pin.isEmpty()) {
    auto values = m_pinsMap.value(port);
    m_pinsMap.remove(port);
    emit portAssignmentChanged(port, values.first, values.second);
  } else {
    auto pinPair = m_pinsMap.value(port);
    bool changed = (pinPair.first != pin || pinPair.second != index);
    m_pinsMap.insert(port, std::make_pair(pin, index));
    if (changed) emit portAssignmentChanged(port, pin, index);
  }
}

void PinsBaseModel::remove(const QString &port, const QString &pin, int index) {
  m_pinsMap.remove(port);
  emit portAssignmentChanged(port, QString{}, index);
}

QStringList PinsBaseModel::getPort(const QString &pin) const {
  QStringList ports;
  for (auto it{m_pinsMap.begin()}; it != m_pinsMap.end(); ++it) {
    if (it.value().first == pin) ports.append(it.key());
  }
  return ports;
}

int PinsBaseModel::getIndex(const QString &pin) const {
  QVector<int> indexes;
  for (auto it{m_pinsMap.begin()}; it != m_pinsMap.end(); ++it) {
    if (it.value().first == pin) {
      indexes.append(it.value().second);
    }
  }
  std::sort(indexes.begin(), indexes.end());
  for (int i = 0; i < indexes.count(); i++) {
    if (i != indexes.at(i)) return i;
  }
  return indexes.count();
}

PackagePinsModel *PinsBaseModel::packagePinModel() const {
  return m_packagePinModel;
}

void PinsBaseModel::setPackagePinModel(PackagePinsModel *newPackagePinModel) {
  m_packagePinModel = newPackagePinModel;
}

PortsModel *PinsBaseModel::portsModel() const { return m_portsModel; }

void PinsBaseModel::setPortsModel(PortsModel *newPortsModel) {
  m_portsModel = newPortsModel;
}

const QMap<QString, std::pair<QString, int> > &PinsBaseModel::pinMap() const {
  return m_pinsMap;
}

}  // namespace FOEDAG
