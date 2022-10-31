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
#pragma once
#include <QStringList>
#include <QVector>

#include "PackagePinsModel.h"
#include "PortsModel.h"

namespace FOEDAG {

class PinsBaseModel {
 public:
  PinsBaseModel();

  bool exists(const QString &port, const QString &pin) const;
  void update(const QString &port, const QString &pin);
  QString getPort(const QString &pin) const;

  PackagePinsModel *packagePinModel() const;
  void setPackagePinModel(PackagePinsModel *newPackagePinModel);

  PortsModel *portsModel() const;
  void setPortsModel(PortsModel *newPortsModel);

  const QMap<QString, QString> &pinMap() const;

 private:
  QMap<QString, QString> m_pinsMap;           // key - port, value - pin
  QMap<QString, QString> m_pinsMap_reverted;  // key - pin, value - port
  PackagePinsModel *m_packagePinModel;
  PortsModel *m_portsModel;
};

}  // namespace FOEDAG
