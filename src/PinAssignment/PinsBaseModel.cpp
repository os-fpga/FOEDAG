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

PinsBaseModel::PinsBaseModel() {}

bool PinsBaseModel::exists(const QString &port, const QString &pin) const {
  for (auto it = m_pinsMap.constBegin(); it != m_pinsMap.constEnd(); ++it) {
    if (it.key() == port && it.value() == pin) return true;
  }
  return false;
}

void PinsBaseModel::update(const QString &port, const QString &pin) {
  if (pin.isEmpty()) {
    m_pinsMap.remove(port);
    auto iter = m_pinsMap.find(port);
    if (iter != m_pinsMap.end()) m_pinsMap_reverted.remove(*iter);
  } else if (port.isEmpty()) {
    auto iter = m_pinsMap_reverted.find(pin);
    if (iter != m_pinsMap_reverted.end()) {
      m_pinsMap.remove(*iter);
      m_pinsMap_reverted.erase(iter);
    }
  } else {
    m_pinsMap.insert(port, pin);
    m_pinsMap_reverted.insert(pin, port);
  }
}

QString PinsBaseModel::getPort(const QString &pin) const {
  return m_pinsMap_reverted.value(pin);
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

const QMap<QString, QString> &PinsBaseModel::pinMap() const {
  return m_pinsMap;
}

}  // namespace FOEDAG
