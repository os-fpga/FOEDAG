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
#include "PinAssignmentCreator.h"

#include <QBoxLayout>
#include <QDebug>
#include <QDir>
#include <filesystem>

#include "Main/ToolContext.h"
#include "PackagePinsLoader.h"
#include "PackagePinsView.h"
#include "PinsBaseModel.h"
#include "PortsLoader.h"
#include "PortsView.h"
#include "Utils/QtUtils.h"

namespace FOEDAG {

QMap<QString, PackagePinsLoader *> PinAssignmentCreator::m_loader{};
QMap<QString, PortsLoader *> PinAssignmentCreator::m_portsLoader{};

PinAssignmentCreator::PinAssignmentCreator(const PinAssignmentData &data,
                                           QObject *parent)
    : QObject(parent), m_data(data) {
  PortsModel *portsModel = new PortsModel{this};
  auto packagePinModel = new PackagePinsModel;
  const QString fileName = searchCsvFile();
  m_baseModel = new PinsBaseModel;
  m_baseModel->setPackagePinModel(packagePinModel);
  m_baseModel->setPortsModel(portsModel);
  packagePinModel->setBaseModel(m_baseModel);

  PortsLoader *portsLoader{FindPortsLoader(data.target)};
  auto [ok, message] = portsLoader->load(searchPortsFile(data.portsFilePath));
  if (!ok) qWarning() << message;

  PackagePinsLoader *loader{FindPackagePinLoader(data.target)};
  loader->loadHeader(packagePinHeaderFile(data.context));
  loader->load(fileName);

  auto portsView = new PortsView(m_baseModel);
  connect(portsView, &PortsView::selectionHasChanged, this,
          &PinAssignmentCreator::changed);
  m_portsView = CreateLayoutedWidget(portsView);

  auto packagePins = new PackagePinsView(m_baseModel);
  connect(packagePins, &PackagePinsView::selectionHasChanged, this,
          &PinAssignmentCreator::changed);
  m_packagePinsView = CreateLayoutedWidget(packagePins);
  packagePinModel->setUseBallId(data.useBallId);
  parseConstraints(data.commands, packagePins, portsView);
}

QWidget *PinAssignmentCreator::GetPackagePinsWidget() {
  return m_packagePinsView;
}

QWidget *PinAssignmentCreator::GetPortsWidget() { return m_portsView; }

QString PinAssignmentCreator::generateSdc() const {
  QString sdc;
  const auto pinMap = m_baseModel->pinMap();
  // generate pin location
  for (auto it = pinMap.constBegin(); it != pinMap.constEnd(); ++it) {
    auto internalPin = m_baseModel->packagePinModel()->internalPin(it.key());
    if (internalPin.isEmpty())
      sdc.append(
          QString("set_pin_loc %1 %2\n").arg(it.key(), it.value().first));
    else
      sdc.append(QString("set_pin_loc %1 %2 %3\n")
                     .arg(it.key(), it.value().first, internalPin));
  }
  // generate mode
  auto modeMap = m_baseModel->packagePinModel()->modeMap();
  for (auto it{modeMap.begin()}; it != modeMap.end(); ++it) {
    sdc.append(QString("set_mode %1 %2\n").arg(it.value(), it.key()));
  }
  return sdc;
}

QWidget *PinAssignmentCreator::CreateLayoutedWidget(QWidget *main) {
  QWidget *w = new QWidget;
  w->setLayout(new QVBoxLayout);
  w->layout()->addWidget(main);
  return w;
}

QString PinAssignmentCreator::searchCsvFile() const {
  return m_data.pinMapFile;
}

QString PinAssignmentCreator::packagePinHeaderFile(ToolContext *context) const {
  auto path = context->DataPath() / "etc" / "package_pin_info.json";
  return QString::fromStdString(path.string());
}

PackagePinsLoader *PinAssignmentCreator::FindPackagePinLoader(
    const QString &targetDevice) const {
  if (!m_loader.contains(targetDevice)) {
    RegisterPackagePinLoader(targetDevice, new PackagePinsLoader{nullptr});
  }
  auto loader = m_loader.value(targetDevice);
  loader->setModel(m_baseModel->packagePinModel());
  return loader;
}

PortsLoader *PinAssignmentCreator::FindPortsLoader(
    const QString &targetDevice) const {
  if (!m_portsLoader.contains(targetDevice)) {
    RegisterPortsLoader(targetDevice, new PortsLoader{nullptr});
  }
  auto loader = m_portsLoader.value(targetDevice);
  loader->SetModel(m_baseModel->portsModel());
  return loader;
}

void PinAssignmentCreator::parseConstraints(const QStringList &commands,
                                            PackagePinsView *packagePins,
                                            PortsView *portsView) {
  QStringList convertedCommands = commands;
  // convert pin name to ball name or ball id
  for (int i = 0; i < convertedCommands.size(); i++) {
    if (convertedCommands.at(i).startsWith("set_pin_loc") ||
        convertedCommands.at(i).startsWith("set_mode")) {
      auto list = QtUtils::StringSplit(convertedCommands.at(i), ' ');
      if (list.size() >= 3) {
        auto convertedName =
            m_baseModel->packagePinModel()->convertPinNameUsage(list.at(2));
        if (!convertedName.isEmpty()) {
          list[2] = convertedName;
          convertedCommands[i] = list.join(' ');
        }
      }
    } else if (convertedCommands.at(i).startsWith("set_property mode")) {
      auto list = QtUtils::StringSplit(convertedCommands.at(i), ' ');
      if (list.size() >= 4) {
        auto convertedName =
            m_baseModel->packagePinModel()->convertPinNameUsage(list.at(3));
        if (!convertedName.isEmpty()) {
          list[3] = convertedName;
          convertedCommands[i] = list.join(' ');
        }
      }
    }
  }

  // First need to setup ports and then modes sinse mode will apply only when
  // port is selected.
  QVector<QStringList> internalPins;
  QMap<QString, int> indx{};
  for (const auto &cmd : std::as_const(convertedCommands)) {
    if (cmd.startsWith("set_pin_loc")) {
      auto list = QtUtils::StringSplit(cmd, ' ');
      if (list.size() >= 3) {
        packagePins->SetPort(list.at(2), list.at(1), indx[list.at(2)]++);
      }
      if (list.size() >= 4) internalPins.append(list);
    }
  }
  for (const auto &cmd : std::as_const(convertedCommands)) {
    if (cmd.startsWith("set_mode")) {
      auto list = QtUtils::StringSplit(cmd, ' ');
      if (list.size() >= 3) {
        packagePins->SetMode(list.at(2), list.at(1));
      }
    } else if (cmd.startsWith("set_property mode")) {
      auto list = QtUtils::StringSplit(cmd, ' ');
      if (list.size() >= 4) {
        packagePins->SetMode(list.at(3), list.at(2));
      }
    }
  }
  for (const auto &intPins : internalPins) {
    packagePins->SetInternalPin(intPins.at(1), intPins.at(3));
  }
}

QString PinAssignmentCreator::searchPortsFile(const QString &projectPath) {
  const QDir dir{projectPath};
  auto file = dir.filePath("port_info.json");
  const QFileInfo fileInfo{file};
  if (fileInfo.exists()) return file;
  return QString();
}

void PinAssignmentCreator::RegisterPackagePinLoader(const QString &device,
                                                    PackagePinsLoader *l) {
  m_loader.insert(device, l);
}

void PinAssignmentCreator::RegisterPortsLoader(const QString &device,
                                               PortsLoader *l) {
  m_portsLoader.insert(device, l);
}

PinsBaseModel *PinAssignmentCreator::baseModel() const { return m_baseModel; }

const PinAssignmentData &PinAssignmentCreator::data() const { return m_data; }

void PinAssignmentCreator::setPinFile(const QString &file) {
  m_data.pinFile = file;
}

void PinAssignmentCreator::setUseBallId(bool useBallId) {
  if (m_data.useBallId != useBallId) {
    m_data.useBallId = useBallId;
    refresh();
  }
}

void PinAssignmentCreator::refresh() {
  const QSignalBlocker signalBlocker{this};
  auto portView = m_portsView->findChild<PortsView *>();
  if (portView) portView->cleanTable();
  auto ppView = m_packagePinsView->findChild<PackagePinsView *>();
  if (ppView) ppView->cleanTable();
  QFile file{m_data.pinFile};
  if (file.open(QFile::ReadOnly)) {
    m_data.commands = QtUtils::StringSplit(QString{file.readAll()}, '\n');
  }
  m_baseModel->packagePinModel()->setUseBallId(m_data.useBallId);
  if (ppView && portView) parseConstraints(m_data.commands, ppView, portView);
}

}  // namespace FOEDAG
