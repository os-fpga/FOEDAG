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
  return m_pinsMap.contains(port) &&
         std::find(m_pinsMap.constBegin(), m_pinsMap.constEnd(), pin) !=
             m_pinsMap.constEnd();
}

void PinsBaseModel::insert(const QString &port, const QString &pin) {
  m_pinsMap.insert(port, pin);
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
