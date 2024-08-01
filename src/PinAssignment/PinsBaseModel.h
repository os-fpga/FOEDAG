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
#include <QObject>
#include <QStringList>
#include <QVector>

#include "PackagePinsModel.h"
#include "PortsModel.h"

namespace FOEDAG {

class PinsBaseModel : public QObject {
  Q_OBJECT

 public:
  PinsBaseModel(QObject *parent = nullptr);

  bool exists(const QString &port, const QString &pin) const;
  void update(const QString &port, const QString &pin, int index);
  void remove(const QString &port, const QString &pin, int index);
  QStringList getPort(const QString &pin) const;
  int getIndex(const QString &pin) const;

  PackagePinsModel *packagePinModel() const;
  void setPackagePinModel(PackagePinsModel *newPackagePinModel);

  PortsModel *portsModel() const;
  void setPortsModel(PortsModel *newPortsModel);

  const QMap<QString, std::pair<QString, int>> &pinMap() const;

 signals:
  void portAssignmentChanged(const QString &port, const QString &pin, int row);

 private:
  QMap<QString, std::pair<QString, int>> m_pinsMap;  // key - port, value - pin
  PackagePinsModel *m_packagePinModel;
  PortsModel *m_portsModel;

#ifndef UPSTREAM_PINPLANNER
#ifdef PINPLANNER_EXCLUDE_USED_ITEMS
  void invalidate();
  void invalidatePortsModel(const QSet<QString>& busyPorts);
  void invalidatePackagePinsModel(const QSet<QString>& busyPins);
  void setListModelSilently(QStringListModel* model, const QStringList& list);
#endif
#endif
};

}  // namespace FOEDAG
