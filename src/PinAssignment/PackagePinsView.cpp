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
#include "PackagePinsView.h"

#include <QComboBox>
#include <QHeaderView>
#include <QStringListModel>

namespace FOEDAG {

PackagePinsView::PackagePinsView(PinsBaseModel *baseModel, QWidget *parent)
    : QTreeWidget(parent) {
  setHeaderLabels({"Name",
                   "Available",
                   "Ports",
                   "Prohibit",
                   "I/O std",
                   "Dir",
                   "Vcco",
                   "Bank",
                   "Byte group",
                   "Type",
                   "Diff pair",
                   "Clock",
                   "Voltage",
                   "Config",
                   "XADC",
                   "Gigabit I/O",
                   "MCB",
                   "PCI",
                   "Min trace Dly (ps)",
                   "Max trace Dly (ps)",
                   "Site",
                   "Site type"});
  header()->resizeSections(QHeaderView::ResizeToContents);

  QTreeWidgetItem *topLevelPackagePin = new QTreeWidgetItem(this);
  topLevelPackagePin->setText(0, "All Pins");
  const auto banks = baseModel->packagePins();
  for (auto &b : banks) {
    QTreeWidgetItem *bank = new QTreeWidgetItem(topLevelPackagePin);
    bank->setText(0, b.id);
    const auto pins = b.pins;
    for (auto &p : pins) {
      QTreeWidgetItem *pin = new QTreeWidgetItem(bank);
      pin->setText(0, p.name);
      auto proxyModel = new QStringListModel;
      proxyModel->setStringList({"", "IN0", "IN1", "OUT"});

      auto combo = new QComboBox{this};
      combo->setModel(proxyModel);
      combo->setAutoFillBackground(true);
      setItemWidget(pin, 2, combo);
    }
  }
  expandItem(topLevelPackagePin);
  setAlternatingRowColors(true);
  setColumnWidth(0, 120);
}

}  // namespace FOEDAG
