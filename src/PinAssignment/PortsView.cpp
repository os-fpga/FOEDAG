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
#include "PortsView.h"

#include <QComboBox>
#include <QHeaderView>
#include <QStringListModel>

namespace FOEDAG {

PortsView::PortsView(PinsBaseModel *baseModel, QWidget *parent)
    : QTreeWidget(parent) {
  setHeaderLabels({"Name", "Dir", "Package Pin", "Bank", "Neg diff pair",
                   "I/O std", "Vcco", "Vref", "Drive strength", "Slew type",
                   "Pull type", "Off-chip Termination", "IN_TERM"});
  header()->resizeSections(QHeaderView::ResizeToContents);

  QTreeWidgetItem *topLevel = new QTreeWidgetItem(this);
  topLevel->setText(0, "All ports");
  addTopLevelItem(topLevel);
  const auto ports = baseModel->ioPorts();
  for (auto &p : ports) {
    auto bankItem = new QTreeWidgetItem;
    bankItem->setText(0, p.name);
    bankItem->setText(1, p.dir);
    topLevel->addChild(bankItem);

    auto proxyModel = new QStringListModel;
    proxyModel->setStringList({"", "GPIO_A_0", "GPIO_A_1", "GPIO_A_2"});

    auto combo = new QComboBox{this};
    combo->setModel(proxyModel);
    combo->setAutoFillBackground(true);

    setItemWidget(bankItem, 2, combo);
    bankItem->setText(3, "1");
  }
  expandItem(topLevel);
  setAlternatingRowColors(true);
  setColumnWidth(0, 120);
}

}  // namespace FOEDAG
