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

enum PinData {
  PinName = 0,
  BallName = 1,
  RefClock = 12,
  Bank = 13,
  ALT = 14,
  DebugMode = 15,
  ScanMode = 16,
  MbistMode = 17,
  Type = 18,
  Dir = 19,
  Voltage = 20,
  PowerPad = 21,
  Discription = 22,
  Voltage2 = 23,
};

struct PackagePinData {
  QStringList data;
};

struct PackagePinGroup {
  QString name;
  QVector<PackagePinData> pinData;
};

class PackagePinsModel : public QObject {
  Q_OBJECT
 public:
  PackagePinsModel(QObject *parent = nullptr);
  QStringList headerList() const;

  void append(const PackagePinGroup &g);
  const QVector<PackagePinGroup> &pinData() const;

  QStringListModel *listModel() const;
  void initListModel();

  void insert(const QString &name, const QModelIndex &index);
  void itemChange(const QString &name, const QString &pin);

 signals:
  void itemHasChanged(const QModelIndex &index, const QString &pin);

 private:
  QVector<PackagePinGroup> m_pinData;
  QStringListModel *m_listModel{nullptr};
  QMap<QString, QModelIndex> m_indexes;
};

}  // namespace FOEDAG
