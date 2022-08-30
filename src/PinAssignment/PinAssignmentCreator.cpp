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
#include <QDir>
#include <filesystem>

#include "Main/ToolContext.h"
#include "PackagePinsLoader.h"
#include "PackagePinsView.h"
#include "PinsBaseModel.h"
#include "PortsLoader.h"
#include "PortsView.h"

namespace FOEDAG {

PinAssignmentCreator::PinAssignmentCreator(ProjectManager *projectManager,
                                           ToolContext *context,
                                           QObject *parent)
    : QObject(parent) {
  PortsModel *portsModel = new PortsModel{this};
  PortsLoader portsLoader{portsModel, this};
  portsLoader.load(searchPortsFile(context));
  auto packagePinModel = new PackagePinsModel;
  const QString fileName = searchCsvFile(targetDevice(projectManager), context);
  PackagePinsLoader loader{packagePinModel, this};
  loader.load(fileName);

  baseModel = new PinsBaseModel;
  baseModel->setPackagePinModel(packagePinModel);
  baseModel->setPortsModel(portsModel);

  auto portsView = new PortsView(baseModel);
  connect(portsView, &PortsView::selectionHasChanged, this,
          &PinAssignmentCreator::selectionHasChanged);
  m_portsView = CreateLayoutedWidget(portsView);

  auto packagePins = new PackagePinsView(baseModel);
  connect(packagePins, &PackagePinsView::selectionHasChanged, this,
          &PinAssignmentCreator::selectionHasChanged);
  m_packagePinsView = CreateLayoutedWidget(packagePins);
}

QWidget *PinAssignmentCreator::GetPackagePinsWidget() {
  return m_packagePinsView;
}

QWidget *PinAssignmentCreator::GetPortsWidget() { return m_portsView; }

QString PinAssignmentCreator::generateSdc() const {
  if (baseModel->pinMap().isEmpty()) return QString();
  QString sdc;
  const auto pinMap = baseModel->pinMap();
  for (auto it = pinMap.constBegin(); it != pinMap.constEnd(); ++it) {
    sdc.append(QString("set_pin_loc %1 %2\n").arg(it.key(), it.value()));
  }
  return sdc;
}

QWidget *PinAssignmentCreator::CreateLayoutedWidget(QWidget *main) {
  QWidget *w = new QWidget;
  w->setLayout(new QVBoxLayout);
  w->layout()->addWidget(main);
  return w;
}

QString PinAssignmentCreator::searchCsvFile(const QString &targetDevice,
                                            ToolContext *context) const {
  std::filesystem::path path{context->DataPath()};
  path = path / "etc" / "devices";
  if (!targetDevice.isEmpty()) path /= targetDevice.toLower().toStdString();

  QDir dir{path.string().c_str()};
  auto files = dir.entryList({"*.csv"}, QDir::Files);
  if (!files.isEmpty()) return dir.filePath(files.first());

  std::filesystem::path pathDefault{context->DataPath()};
  pathDefault = pathDefault / "etc" / "templates" / "Pin_Table.csv";
  return QString(pathDefault.string().c_str());
}

QString PinAssignmentCreator::targetDevice(
    ProjectManager *projectManager) const {
  if (!projectManager->HasDesign()) return QString();
  if (projectManager->getTargetDevice().empty()) return QString();
  return QString::fromStdString(projectManager->getTargetDevice());
}

QString PinAssignmentCreator::searchPortsFile(ToolContext *context) const {
  // TODO @volodymyrk GEMINIEDA-229. The path will be changed after this ticket
  std::filesystem::path path{context->DataPath()};
  path = path / "etc" / "templates";

  QDir dir{path.string().c_str()};
  return dir.filePath("ports_test.json");
}

}  // namespace FOEDAG
