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

#include "PackagePinsView.h"
#include "PinsBaseModel.h"
#include "PortsView.h"

namespace FOEDAG {

PinAssignmentCreator::PinAssignmentCreator() {
  auto baseModel = new PinsBaseModel;
  auto portsView = new PortsView(baseModel);
  m_portsView = CreateLayoutedWidget(portsView);

  auto packagePins = new PackagePinsView(baseModel);
  m_packagePinsView = CreateLayoutedWidget(packagePins);
}

QWidget *PinAssignmentCreator::GetPackagePinsWidget() {
  return m_packagePinsView;
}

QWidget *PinAssignmentCreator::GetPortsWidget() { return m_portsView; }

QWidget *PinAssignmentCreator::CreateLayoutedWidget(QWidget *main) {
  QWidget *w = new QWidget;
  w->setLayout(new QVBoxLayout);
  w->layout()->addWidget(main);
  return w;
}

}  // namespace FOEDAG
