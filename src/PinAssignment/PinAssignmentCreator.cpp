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
#ifdef UPSTREAM_PINPLANNER
#include "PackagePinsLoader.h"
#include "PortsLoader.h"
#else
#include "QLPackagePinsLoader.h"
#include "QLPortsLoader.h"
#include "PcfObserver.h"
#include "ErrorsModel.h"
#include "ErrorsView.h"
#include <QFile>
#endif
#include "PackagePinsView.h"
#include "PinsBaseModel.h"
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

#ifdef UPSTREAM_PINPLANNER
  loader->loadHeader(packagePinHeaderFile(data.context));
#else
  m_pcfObserver = new PcfObserver{this, m_data.pinFile, portsModel, packagePinModel};
  connect(m_pcfObserver, &PcfObserver::contentChecked, this, [this](bool status){
    for (QWidget* ioView: m_ioViews) {
      ioView->setEnabled(status);
    }

    for (ErrorsView* errorsView: m_errorsViews) {
      errorsView->setData(m_pcfObserver->errors());
      errorsView->setVisible(!status);
    }

    refresh(status);
    emit allowSaving(status);
  });
#endif

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

#ifdef UPSTREAM_PINPLANNER
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
#else

void PinAssignmentCreator::forceNextPcfFileCheck()
{
  m_pcfObserver->forceNextCheck();
}

std::pair<QString, bool> PinAssignmentCreator::generatePcf() const {
  QString pcf;
  auto convertToSet = [](const QList<QString>& l) {
    return QSet<QString>(l.begin(), l.end());
  };
  const QSet<QString> ports = convertToSet(m_baseModel->portsModel()->listModel()->stringList());
  const QSet<QString> pins = convertToSet(m_baseModel->packagePinModel()->listModel()->stringList());

  bool foundInvalidConnection = false;

  const auto pinMap = m_baseModel->pinMap();
  for (auto it = pinMap.constBegin(); it != pinMap.constEnd(); ++it) {
    QString port{it.key()};
    QString pin{it.value().first};
    QString assignment{QString("set_io %1 %2\n").arg(port, pin)};
    if (!ports.contains(port)) {
      //qInfo() << QString("skip assignment [%1], because of not existed port [%2]").arg(assignment).arg(port);
      foundInvalidConnection = true;
      continue;
    }
    if (!pins.contains(pin)) {
      //qInfo() << QString("skip assignment [%1], because of not existed pin [%2]").arg(assignment).arg(pin);
      foundInvalidConnection = true;
      continue;
    }
    pcf.append(assignment);
  }
  return std::make_pair(pcf, foundInvalidConnection);
}

// internally PinAssignmentCreator expects sdc custom format not pcf, so we do pcf to sdc conversion
QList<QString> PinAssignmentCreator::convertPcfToSdcCommands(const QList<PcfLineFrame>& lineFrames)
{
  QList<QString> commands;
  commands.reserve(lineFrames.size());
  for (const PcfLineFrame& lineFrame: lineFrames) {
    QString cmd{lineFrame.line};
    cmd = cmd.replace("set_io", "set_pin_loc");
    commands.append(cmd);
  }
  return commands;
}
#endif

QWidget *PinAssignmentCreator::CreateLayoutedWidget(QWidget *main) {
  QWidget *w = new QWidget;
#ifdef UPSTREAM_PINPLANNER
  w->setLayout(new QVBoxLayout);
#else
  w->setLayout(new QHBoxLayout);
  w->layout()->setContentsMargins(1, 1, 1, 1);
#endif
  w->layout()->addWidget(main);

#ifndef UPSTREAM_PINPLANNER
  m_ioViews.append(main);

  ErrorsModel* errorsModel = new ErrorsModel(m_baseModel);
  ErrorsView* errorsView = new ErrorsView(errorsModel);
  connect(errorsView, &ErrorsView::openFileRequested, this, [this](){
    emit openPcfFileRequested(m_data.pinFile);
  });

  m_errorsViews.append(errorsView);
  w->layout()->addWidget(errorsView);
  errorsView->setVisible(false); // initially hide
#endif
  return w;
}

QString PinAssignmentCreator::searchCsvFile() const {
  return m_data.pinMapFile;
}

#ifdef UPSTREAM_PINPLANNER
QString PinAssignmentCreator::packagePinHeaderFile(ToolContext *context) const {
  auto path = context->DataPath() / "etc" / "package_pin_info.json";
  return QString::fromStdString(path.string());
}
#endif

PackagePinsLoader *PinAssignmentCreator::FindPackagePinLoader(
    const QString &targetDevice) const {
  if (!m_loader.contains(targetDevice)) {
#ifdef UPSTREAM_PINPLANNER
    RegisterPackagePinLoader(targetDevice, new PackagePinsLoader{nullptr});
#else
    RegisterPackagePinLoader(targetDevice, new QLPackagePinsLoader{nullptr});
#endif
}
  auto loader = m_loader.value(targetDevice);
  loader->setModel(m_baseModel->packagePinModel());
  return loader;
}

PortsLoader *PinAssignmentCreator::FindPortsLoader(
    const QString &targetDevice) const {
  if (!m_portsLoader.contains(targetDevice)) {
#ifdef UPSTREAM_PINPLANNER
    RegisterPortsLoader(targetDevice, new PortsLoader{nullptr});
#else
    RegisterPortsLoader(targetDevice, new QLPortsLoader{nullptr});
#endif
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
#ifdef UPSTREAM_PINPLANNER
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
#endif
}

QString PinAssignmentCreator::searchPortsFile(const QString &projectPath) {
  const QDir dir{projectPath};
#ifdef UPSTREAM_PINPLANNER
  auto file = dir.filePath("port_info.json");
#else
  QString projectName = QDir(projectPath).dirName();
  auto file = dir.filePath(projectName + "_post_synth.blif");
#endif
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

#ifdef UPSTREAM_PINPLANNER
void PinAssignmentCreator::refresh(bool isPcfOk) {
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
#else
void PinAssignmentCreator::refresh(bool isPcfOk) {
  const QSignalBlocker signalBlocker{this};

  PortsLoader *portsLoader{FindPortsLoader(m_data.target)};
  auto [ok, message] = portsLoader->load(searchPortsFile(m_data.portsFilePath));
  if (!ok) qWarning() << message;

  auto portView = m_portsView->findChild<PortsView *>();
  if (portView) {
    portView->refreshContentFromModel();
  }

  auto ppView = m_packagePinsView->findChild<PackagePinsView *>();
  if (ppView) ppView->cleanTable();

  if (isPcfOk) {
    m_data.commands = convertPcfToSdcCommands(m_pcfObserver->lineFrames(false));
  } else {
    m_data.commands.clear();
  }

  m_baseModel->packagePinModel()->setUseBallId(m_data.useBallId);
  if (ppView && portView) parseConstraints(m_data.commands, ppView, portView);
}
#endif

}  // namespace FOEDAG
