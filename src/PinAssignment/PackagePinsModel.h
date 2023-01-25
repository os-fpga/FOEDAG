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
  BallId = 2,
  InternalPinName = 12,
  ModeFirst = 13,
  ModeLast = 42,
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

using BallData = QMap<QString, QString>;  // <id, name>
using InternalPins = QMap<QString, QMap<int, QStringList>>;
class PinsBaseModel;
class PackagePinsModel : public QObject {
  Q_OBJECT
 public:
  PackagePinsModel(QObject *parent = nullptr);
  const QVector<HeaderData> &header() const;
  void appendHeaderData(const HeaderData &h);
  void updateMode(const QString &pin, const QString &mode);
  QString getMode(const QString &pin) const;
  void updateInternalPin(const QString &port, const QString &intPin);
  void insertMode(int id, const QString &mode);
  InternalPins &internalPinsRef();
  QStringList GetInternalPinsList(const QString &pin, const QString &mode,
                                  const QString &current = QString{}) const;
  int internalPinMax() const;

  void append(const PackagePinGroup &g);
  const QVector<PackagePinGroup> &pinData() const;
  const QMap<QString, QString> &modeMap() const;
  QString internalPin(const QString &port) const;

  QStringListModel *listModel() const;
  QStringListModel *modeModelTx() const;
  QStringListModel *modeModelRx() const;
  void initListModel();

  void insert(const QString &pin, const QModelIndex &index);
  void remove(const QString &pin);
  void itemChange(const QString &name, const QString &pin);

  const QVector<QString> &userGroups() const;
  void appendUserGroup(const QString &userGroup);

  void setBaseModel(PinsBaseModel *m);

  void setUseBallId(bool useBallId);
  bool useBallId() const;

  QString convertPinName(const QString &name) const;
  void insertBallData(const QString &name, const QString &id);
  QString convertPinNameUsage(const QString &nameOrId);

 signals:
  void modeHasChanged(const QString &pin, const QString &mode);
  void internalPinHasChanged(const QString &pin, const QString &intPin);
  void pinNameChanged();

 private:
  QVector<PackagePinGroup> m_pinData;
  QStringListModel *m_listModel{nullptr};
  QStringListModel *m_modeModelTx{nullptr};
  QStringListModel *m_modeModelRx{nullptr};
  QVector<HeaderData> m_header;
  QVector<QString> m_userGroups;
  QMap<QString, QString> m_modeMap;
  QMap<QString, QString> m_internalPinMap;
  QMap<QString, int> m_modes;
  InternalPins m_internalPinsData;  // <PinName, <ModeId, InternalPins>>
  PinsBaseModel *m_baseModel;
  bool m_useBallId{false};
  BallData m_ballData;
};

}  // namespace FOEDAG
