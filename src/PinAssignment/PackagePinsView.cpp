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

#include <QHeaderView>
#include <QStringListModel>

#include "BufferedComboBox.h"

namespace FOEDAG {

constexpr int NameCol{0};
constexpr int AvailCol{1};
constexpr int PortsCol{2};

PackagePinsView::PackagePinsView(PinsBaseModel *model, QWidget *parent)
    : PinAssignmentBaseView(model, parent) {
  setHeaderLabels(model->packagePinModel()->headerList());
  header()->resizeSections(QHeaderView::ResizeToContents);

  QTreeWidgetItem *topLevelPackagePin = new QTreeWidgetItem(this);
  topLevelPackagePin->setText(0, "All Pins");
  const auto banks = model->packagePinModel()->pinData();
  for (const auto &b : banks) {
    QTreeWidgetItem *bank = new QTreeWidgetItem(topLevelPackagePin);
    bank->setText(NameCol, b.name);
    bank->setText(AvailCol, QString::number(b.pinData.count()));
    const auto pins = b.pinData;
    for (auto &p : pins) {
      uint col = PortsCol + 1;
      QTreeWidgetItem *pinItem = new QTreeWidgetItem(bank);
      insertData(p.data, PinName, NameCol, pinItem);
      insertData(p.data, RefClock, col++, pinItem);
      insertData(p.data, Bank, col++, pinItem);
      insertData(p.data, ALT, col++, pinItem);
      insertData(p.data, DebugMode, col++, pinItem);
      insertData(p.data, ScanMode, col++, pinItem);
      insertData(p.data, MbistMode, col++, pinItem);
      insertData(p.data, Type, col++, pinItem);
      insertData(p.data, Dir, col++, pinItem);
      insertData(p.data, Voltage, col++, pinItem);
      insertData(p.data, PowerPad, col++, pinItem);
      insertData(p.data, Discription, col++, pinItem);
      insertData(p.data, Voltage2, col++, pinItem);
      insertData(p.data, RefClock, col++, pinItem);

      auto combo = new BufferedComboBox{this};
      combo->setModel(model->portsModel()->listModel());
      combo->setAutoFillBackground(true);
      m_allCombo.append(combo);
      connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
              [=]() {
                ioPortsSelectionHasChanged(indexFromItem(pinItem, PortsCol));
              });
      setItemWidget(pinItem, PortsCol, combo);
      model->packagePinModel()->insert(pinItem->text(NameCol),
                                       indexFromItem(pinItem, NameCol));
    }
  }
  connect(model->packagePinModel(), &PackagePinsModel::itemHasChanged, this,
          &PackagePinsView::itemHasChanged);
  expandItem(topLevelPackagePin);
  setAlternatingRowColors(true);
  setColumnWidth(NameCol, 120);
}

void PackagePinsView::ioPortsSelectionHasChanged(const QModelIndex &index) {
  if (m_blockUpdate) return;
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<BufferedComboBox *>(itemWidget(item, PortsCol));
    auto port = combo->currentText();
    removeDuplications(port, combo);

    auto pin = item->text(NameCol);
    m_model->insert(port, pin);
    m_model->portsModel()->itemChange(port, pin);

    // unset previous selection
    auto prevPort = combo->previousText();
    m_model->portsModel()->itemChange(prevPort, QString());
    emit selectionHasChanged();
  }
}

void PackagePinsView::insertData(const QStringList &data, int index, int column,
                                 QTreeWidgetItem *item) {
  if (data.count() > index) item->setText(column, data.at(index));
}

void PackagePinsView::itemHasChanged(const QModelIndex &index,
                                     const QString &pin) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<QComboBox *>(itemWidget(item, PortsCol));
    m_blockUpdate = true;
    const int index = combo->findData(pin, Qt::DisplayRole);
    combo->setCurrentIndex(index != -1 ? index : 0);
    m_blockUpdate = false;
  }
}

}  // namespace FOEDAG
