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
  RefClock = 43,
  Bank = 44,
  ALT = 45,
  DebugMode = 46,
  ScanMode = 47,
  MbistMode = 48,
  Type = 49,
  Dir = 50,
  Voltage = 51,
  PowerPad = 52,
  Discription = 53,
  Voltage2 = 54,
};

struct PackagePinData {
  QStringList data;
};

struct PackagePinGroup {
  QString name;
  QVector<PackagePinData> pinData;
};

struct HeaderData {
  QString name;
  QString description;
  int id;
  bool visible;
};

class PackagePinsModel : public QObject {
  Q_OBJECT
 public:
  PackagePinsModel(QObject *parent = nullptr);
  const QVector<HeaderData> &header() const;
  void appendHeaderData(const HeaderData &h);
  void updateMode(const QString &pin, const QString &mode);

  void append(const PackagePinGroup &g);
  const QVector<PackagePinGroup> &pinData() const;
  const QMap<QString, QString> &modeMap() const;

  QStringListModel *listModel() const;
  QStringListModel *modeModel() const;
  void initListModel();

  void insert(const QString &name, const QModelIndex &index);
  void itemChange(const QString &name, const QString &pin);

  const QVector<QString> &userGroups() const;
  void appendUserGroup(const QString &userGroup);

 signals:
  void itemHasChanged(const QModelIndex &index, const QString &pin);

 private:
  QVector<PackagePinGroup> m_pinData;
  QStringListModel *m_listModel{nullptr};
  QStringListModel *m_modeModel{nullptr};
  QMap<QString, QModelIndex> m_indexes;
  QVector<HeaderData> m_header;
  QVector<QString> m_userGroups;
  QMap<QString, QString> m_modeMap;
};

}  // namespace FOEDAG
