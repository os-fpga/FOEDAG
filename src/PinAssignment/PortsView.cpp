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

#include <QHeaderView>
#include <QStringListModel>

#include "BufferedComboBox.h"
#include "PinsBaseModel.h"

namespace FOEDAG {

constexpr uint PortName{0};
constexpr uint DirCol{1};
constexpr uint PackagePinCol{2};
constexpr uint TypeCol{3};

PortsView::PortsView(PinsBaseModel *model, QWidget *parent)
    : PinAssignmentBaseView(model, parent) {
  setHeaderLabels(model->portsModel()->headerList());
  header()->resizeSections(QHeaderView::ResizeToContents);

  QTreeWidgetItem *topLevel = new QTreeWidgetItem(this);
  topLevel->setText(0, "Design ports");
  addTopLevelItem(topLevel);
  auto portsModel = model->portsModel();
  for (const auto &group : portsModel->ports()) {
    for (const auto &p : group.ports) {
      if (p.isBus) {
        auto item = new QTreeWidgetItem;
        item->setText(PortName, p.name);
        topLevel->addChild(item);
        for (const auto &subPort : p.ports) insertTableItem(item, subPort);
      } else {
        insertTableItem(topLevel, p);
      }
    }
  }
  connect(model->portsModel(), &PortsModel::itemHasChanged, this,
          &PortsView::itemHasChanged);
  expandItem(topLevel);
  setAlternatingRowColors(true);
  setColumnWidth(PortName, 120);
  resizeColumnToContents(PackagePinCol);
}

void PortsView::packagePinSelectionHasChanged(const QModelIndex &index) {
  if (m_blockUpdate) return;
  auto item = itemFromIndex(index);
  if (item) {
    auto combo =
        qobject_cast<BufferedComboBox *>(itemWidget(item, PackagePinCol));
    auto pin = combo->currentText();
    removeDuplications(pin, combo);

    auto port = item->text(PortName);
    m_model->insert(port, pin);
    m_model->packagePinModel()->itemChange(pin, port);

    // unset previous selection
    auto prevPin = combo->previousText();
    m_model->packagePinModel()->itemChange(prevPin, QString());
    emit selectionHasChanged();
  }
}

void PortsView::insertTableItem(QTreeWidgetItem *parent, const IOPort &port) {
  auto it = new QTreeWidgetItem{parent};
  it->setText(PortName, port.name);
  it->setText(DirCol, port.dir);
  it->setText(TypeCol, port.type);

  auto combo = new BufferedComboBox{this};
  combo->setModel(m_model->packagePinModel()->listModel());
  combo->setAutoFillBackground(true);
  m_allCombo.append(combo);
  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() {
            packagePinSelectionHasChanged(indexFromItem(it, PackagePinCol));
          });
  setItemWidget(it, PackagePinCol, combo);
  m_model->portsModel()->insert(it->text(PortName),
                                indexFromItem(it, PortName));
}

void PortsView::itemHasChanged(const QModelIndex &index, const QString &pin) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<QComboBox *>(itemWidget(item, PackagePinCol));
    m_blockUpdate = true;
    const int index = combo->findData(pin, Qt::DisplayRole);
    combo->setCurrentIndex(index != -1 ? index : 0);
    m_blockUpdate = false;
  }
}

}  // namespace FOEDAG
