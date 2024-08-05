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
  IOPort GetPort(const QString &portName) const;

  QStringListModel *listModel() const;
#ifndef UPSTREAM_PINPLANNER
  void clear();
  const QStringList& inputPortsOrig() const { return m_inputPortsOrig; }
  const QStringList& outputPortsOrig() const { return m_outputPortsOrig; }
  QStringListModel *listModel(const QString& direction) const;
#endif

 private:
  QVector<IOPortGroup> m_ioPorts;
  QStringListModel *m_model;
#ifndef UPSTREAM_PINPLANNER
  QStringList m_inputPortsOrig;
  QStringList m_outputPortsOrig;
  QStringListModel *m_inputModel{nullptr};
  QStringListModel *m_outputModel{nullptr};
#endif
};

}  // namespace FOEDAG
