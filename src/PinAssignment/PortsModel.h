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
#include <QStringListModel>
#include <QVector>

namespace FOEDAG {

struct IOPort {
  QString name;
  QString dir;
  QString packagePin;
  QString type;
  QString range;
  bool isBus;
  QVector<IOPort> ports;
};

struct IOPortGroup {
  QString name;
  QVector<IOPort> ports;
};

class PortsModel : public QObject {
  Q_OBJECT
 public:
  PortsModel(QObject *parent = nullptr);
  QStringList headerList() const;
  void append(const IOPortGroup &p);
  const QVector<IOPortGroup> &ports() const;
  void initListModel();

  QStringListModel *listModel() const;
  void insert(const QString &name, const QModelIndex &index);
  void itemChange(const QString &name, const QString &newPin);

 signals:
  void itemHasChanged(const QModelIndex &index, const QString &newPin);

 private:
  QVector<IOPortGroup> m_ioPorts;
  QStringListModel *m_model;
  QMap<QString, QModelIndex> m_indexes;
};

}  // namespace FOEDAG
