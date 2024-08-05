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

#ifndef UPSTREAM_PINPLANNER
#include "IODirection.h"
#include <QSet>
#endif

namespace FOEDAG {

PinsBaseModel::PinsBaseModel(QObject *parent) : QObject(parent) {}

bool PinsBaseModel::exists(const QString &port, const QString &pin) const {
  for (auto it = m_pinsMap.constBegin(); it != m_pinsMap.constEnd(); ++it) {
    if (it.key() == port && it.value().first == pin) return true;
  }
  return false;
}

void PinsBaseModel::update(const QString &port, const QString &pin, int index) {
  if (port.isEmpty()) return;
  if (pin.isEmpty()) {
    auto values = m_pinsMap.value(port);
    m_pinsMap.remove(port);
    emit portAssignmentChanged(port, values.first, values.second);
  } else {
    m_pinsMap.insert(port, std::make_pair(pin, index));
    // there is a cases (on refreshing UI) when PortsView didn't refresh properly due to portAssignmentChanged not emiting
    // to solve this we emit signal portAssignmentChanged without relying on changed flag
#ifdef UPSTREAM_PINPLANNER
    auto pinPair = m_pinsMap.value(port);
    bool changed = (pinPair.first != pin || pinPair.second != index);
    if (changed) emit portAssignmentChanged(port, pin, index);
#else
    emit portAssignmentChanged(port, pin, index);
#endif
  }
#ifndef UPSTREAM_PINPLANNER
#ifdef PINPLANNER_EXCLUDE_USED_ITEMS
  invalidate();
#endif
#endif
}

void PinsBaseModel::remove(const QString &port, const QString &pin, int index) {
  m_pinsMap.remove(port);
  emit portAssignmentChanged(port, QString{}, index);
}

QStringList PinsBaseModel::getPort(const QString &pin) const {
  QStringList ports;
  for (auto it{m_pinsMap.begin()}; it != m_pinsMap.end(); ++it) {
    if (it.value().first == pin) ports.append(it.key());
  }
  return ports;
}

int PinsBaseModel::getIndex(const QString &pin) const {
  QVector<int> indexes;
  for (auto it{m_pinsMap.begin()}; it != m_pinsMap.end(); ++it) {
    if (it.value().first == pin) {
      indexes.append(it.value().second);
    }
  }
  std::sort(indexes.begin(), indexes.end());
  for (int i = 0; i < indexes.count(); i++) {
    if (i != indexes.at(i)) return i;
  }
  return indexes.count();
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

const QMap<QString, std::pair<QString, int> > &PinsBaseModel::pinMap() const {
  return m_pinsMap;
}

#ifndef UPSTREAM_PINPLANNER
#ifdef PINPLANNER_EXCLUDE_USED_ITEMS
void PinsBaseModel::invalidate()
{
  QSet<QString> busyPorts;
  QSet<QString> busyPins;
  for (auto it = m_pinsMap.constBegin(); it != m_pinsMap.constEnd(); ++it) {
    busyPorts.insert(it.key());
    busyPins.insert(it.value().first);
  }

  invalidatePortsModel(busyPorts);
  invalidatePackagePinsModel(busyPins);
}

void PinsBaseModel::invalidatePortsModel(const QSet<QString>& busyPorts)
{
 const QStringList& inputPortsOrig = m_portsModel->inputPortsOrig();
  QStringList freeInputPorts;
  for (const QString& port: inputPortsOrig) {
    if (!busyPorts.contains(port)) {
      freeInputPorts.append(port);
    }
  }

  const QStringList& outputPortsOrig = m_portsModel->outputPortsOrig();
  QStringList freeOutputPorts;
  for (const QString& port: outputPortsOrig) {
    if (!busyPorts.contains(port)) {
      freeOutputPorts.append(port);
    }
  }

  setListModelSilently(m_portsModel->listModel(IODirection::INPUT), freeInputPorts);
  setListModelSilently(m_portsModel->listModel(IODirection::OUTPUT), freeOutputPorts);
}

void PinsBaseModel::invalidatePackagePinsModel(const QSet<QString>& busyPins)
{
 const QStringList& inputPinsOrig = m_packagePinModel->inputPinsOrig();
  QStringList freeInputPins;
  for (const QString& pin: inputPinsOrig) {
    if (!busyPins.contains(pin)) {
      freeInputPins.append(pin);
    }
  }

  const QStringList& outputPinsOrig = m_packagePinModel->outputPinsOrig();
  QStringList freeOutputPins;
  for (const QString& pin: outputPinsOrig) {
    if (!busyPins.contains(pin)) {
      freeOutputPins.append(pin);
    }
  }

  setListModelSilently(m_packagePinModel->listModel(IODirection::INPUT), freeInputPins);
  setListModelSilently(m_packagePinModel->listModel(IODirection::OUTPUT), freeOutputPins);
}

void PinsBaseModel::setListModelSilently(QStringListModel* model, const QStringList& list)
{
  model->blockSignals(true);
  model->setStringList(list);
  model->blockSignals(false);
}
#endif // PINPLANNER_EXCLUDE_USED_ITEMS
#endif // UPSTREAM_PINPLANNER

}  // namespace FOEDAG
