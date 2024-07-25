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
#include "PortsModel.h"

#ifndef UPSTREAM_PINPLANNER
#include "IODirection.h"
#endif

namespace FOEDAG {

PortsModel::PortsModel(QObject *parent)
#ifdef UPSTREAM_PINPLANNER
    : QObject(parent), m_model(new QStringListModel(this)) {}
#else
    : QObject(parent),
    m_model(new QStringListModel(this)),
    m_inputModel(new QStringListModel(this)),
    m_outputModel(new QStringListModel(this))  {}
#endif

QStringList PortsModel::headerList() const {
  return {"Name", "Dir", "Package Pin", "Mode", "Internal pins", "Type"};
}

void PortsModel::append(const IOPortGroup &p) { m_ioPorts.append(p); }

const QVector<IOPortGroup> &PortsModel::ports() const { return m_ioPorts; }

void PortsModel::initListModel() {
  QStringList portsList;
  portsList.append(QString());

#ifndef UPSTREAM_PINPLANNER
  QStringList inputPortsList;
  inputPortsList.append(QString());
  QStringList outputPortsList;
  outputPortsList.append(QString());
#endif

  for (const auto &group : std::as_const(m_ioPorts))
    for (const auto &p : std::as_const(group.ports)) {
      if (p.isBus) {
        for (const auto &sub : p.ports) portsList.append(sub.name);
      } else {
        portsList.append(p.name);
#ifndef UPSTREAM_PINPLANNER
        if (p.dir == IODirection::INPUT) {
          inputPortsList.append(p.name);
        } else if (p.dir == IODirection::OUTPUT) {
          outputPortsList.append(p.name);
        }
#endif
      }
    }
  m_model->setStringList(portsList);
#ifndef UPSTREAM_PINPLANNER
  m_inputModel->setStringList(inputPortsList);
  m_outputModel->setStringList(outputPortsList);
#endif
}

IOPort PortsModel::GetPort(const QString &portName) const {
  auto findPort =
      [&portName](const QVector<IOPort> &ports) -> std::pair<bool, IOPort> {
    for (const auto &p : ports) {
      if (p.name == portName) return std::make_pair(true, p);
    }
    return std::make_pair(false, IOPort{});
  };
  for (const auto &group : m_ioPorts) {
    const auto &[find, ioPort] = findPort(group.ports);
    if (find) return ioPort;
    for (const auto &subPort : group.ports) {
      const auto &[find, ioPort] = findPort(subPort.ports);
      if (find) return ioPort;
    }
  }
  return IOPort{};
}

QStringListModel *PortsModel::listModel() const { return m_model; }

#ifndef UPSTREAM_PINPLANNER
QStringListModel *PortsModel::listModel(const QString& direction) const
{
  if (direction == IODirection::INPUT) {
    return m_inputModel;
  }
  if (direction == IODirection::OUTPUT) {
    return m_outputModel;
  }
  return m_model;
}
#endif

}  // namespace FOEDAG
