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

#include <QCompleter>
#include <QHeaderView>
#include <QStringListModel>

#include "BufferedComboBox.h"
#include "PinsBaseModel.h"

namespace FOEDAG {

constexpr uint PortName{0};
constexpr uint DirCol{1};
constexpr uint PackagePinCol{2};
constexpr uint ModeCol{3};
constexpr uint InternalPinsCol{4};
constexpr uint TypeCol{5};

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
  connect(model->packagePinModel(), &PackagePinsModel::modeHasChanged, this,
          &PortsView::modeChanged);
  connect(model->packagePinModel(), &PackagePinsModel::internalPinHasChanged,
          this, &PortsView::intPinChanged);
  connect(model, &PinsBaseModel::portAssignmentChanged, this,
          &PortsView::portAssignmentChanged);
  expandItem(topLevel);
  setAlternatingRowColors(true);
  setColumnWidth(PortName, 120);
  setColumnWidth(ModeCol, 180);
  setColumnWidth(InternalPinsCol, 150);
  resizeColumnToContents(PackagePinCol);
  hideColumn(ModeCol);
  hideColumn(InternalPinsCol);
}

void PortsView::SetPin(const QString &port, const QString &pin) {
  QModelIndexList indexes{match(port)};
  if (!indexes.isEmpty()) {
    auto index = indexes.first();
    setComboData(index, PackagePinCol, pin);
  }
}

void PortsView::cleanTable() {
  for (auto it{m_allCombo.cbegin()}; it != m_allCombo.cend(); it++) {
    it.key()->setCurrentIndex(0);
  }
}

void PortsView::packagePinSelectionHasChanged(const QModelIndex &index) {
  // update here Mode selection
  auto item = itemFromIndex(index);
  auto combo =
      item ? GetCombo<BufferedComboBox *>(item, PackagePinCol) : nullptr;
  if (combo) {
    const QString port =
        combo->currentText().isEmpty() ? QString{} : item->text(PortName);
    updateModeCombo(port, index);
  }

  if (item) {
    auto combo = GetCombo<BufferedComboBox *>(item, PackagePinCol);
    auto internalPinCombo = GetCombo(item, InternalPinsCol);
    if (combo) {
      auto pin = combo->currentText();
      auto prevPin = combo->previousText();

      if (!pin.isEmpty())
        m_intPins[combo->currentText()].insert(internalPinCombo);
      if (!prevPin.isEmpty()) m_intPins[prevPin].remove(internalPinCombo);

      auto port = item->text(PortName);
      int index = m_model->getIndex(pin);
      m_blockUpdate = true;
      if (!prevPin.isEmpty()) m_model->update(QString{}, prevPin, -1);
      m_model->update(port, pin, index);
      m_blockUpdate = false;
      emit selectionHasChanged();
    }
  }
}

void PortsView::insertTableItem(QTreeWidgetItem *parent, const IOPort &port) {
  auto it = new QTreeWidgetItem{parent};
  it->setText(PortName, port.name);
  it->setText(DirCol, port.dir);
  it->setText(TypeCol, port.type);

  auto combo = new BufferedComboBox{this};
#ifdef UPSTREAM_PINPLANNER
  combo->setModel(m_model->packagePinModel()->listModel());
#else
  combo->setModel(m_model->packagePinModel()->listModel(port.dir));
#endif

  combo->setAutoFillBackground(true);
  combo->setEditable(true);
#ifdef UPSTREAM_PINPLANNER
  auto completer{new QCompleter{m_model->packagePinModel()->listModel()}};
#else
  auto completer{new QCompleter{m_model->packagePinModel()->listModel(port.dir)}};
#endif
  completer->setFilterMode(Qt::MatchContains);
  combo->setCompleter(completer);
  combo->setInsertPolicy(QComboBox::NoInsert);
  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() {
            packagePinSelectionHasChanged(indexFromItem(it, PackagePinCol));
          });
  setItemWidget(it, PackagePinCol, combo);
  m_allCombo.insert(combo, indexFromItem(it));

  auto modeCombo = CreateCombo(this);
  modeCombo->setEnabled(modeCombo->count() > 0);
  connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() { modeSelectionHasChanged(indexFromItem(it, ModeCol)); });
  setItemWidget(it, ModeCol, modeCombo);

  auto internalPinCombo = CreateCombo(this);
  internalPinCombo->setEnabled(internalPinCombo->count() > 0);
  connect(internalPinCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [=]() {
            internalPinSelectionHasChanged(indexFromItem(it, ModeCol));
          });
  setItemWidget(it, InternalPinsCol, internalPinCombo);
}

void PortsView::modeSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto comboMode = GetCombo(item, ModeCol);
    if (comboMode) {
      m_model->packagePinModel()->updateMode(getPinSelection(index),
                                             comboMode->currentText());
      updateIntPinCombo(comboMode->currentText(), index);
      emit selectionHasChanged();
    }
  }
}

void PortsView::internalPinSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto comboIntPin = GetCombo(item, InternalPinsCol);
    if (comboIntPin) {
      m_model->packagePinModel()->updateInternalPin(item->text(PortName),
                                                    comboIntPin->currentText());
      const auto combos{m_intPins[getPinSelection(index)]};
      for (auto c : combos) {
        if (comboIntPin != c)
          updateInternalPinSelection(getPinSelection(index), c);
      }
      emit selectionHasChanged();
    }
  }
}

void PortsView::updateModeCombo(const QString &port, const QModelIndex &index) {
  auto modeIndex = model()->index(index.row(), ModeCol, index.parent());
  QComboBox *modeCombo{GetCombo(modeIndex, ModeCol)};
  if (modeCombo) {
    modeCombo->setEnabled(!port.isEmpty());
    if (port.isEmpty()) {
      const QSignalBlocker blocker{modeCombo};
      modeCombo->setCurrentIndex(0);
      // update model in PackagePinsView
      // cleanup internal pin selection
      updateIntPinCombo(QString{}, index);
    } else {
      auto currentMode =
          m_model->packagePinModel()->getMode(getPinSelection(index));

      auto ioPort = m_model->portsModel()->GetPort(port);
      const bool output = ioPort.dir == "Output";
      QAbstractItemModel *modeModel =
          output ? m_model->packagePinModel()->modeModelTx()
                 : m_model->packagePinModel()->modeModelRx();
      modeCombo->blockSignals(!currentMode.isEmpty());
      if (modeCombo->model() != modeModel) {
        modeCombo->setModel(modeModel);
      }
      if (!currentMode.isEmpty()) {
        const int idx = modeCombo->findData(currentMode, Qt::DisplayRole);
        if (idx > -1) modeCombo->setCurrentIndex(idx);
        updateIntPinCombo(currentMode, index);
      }
      modeCombo->blockSignals(false);
    }
  }
}

void PortsView::updateIntPinCombo(const QString &mode,
                                  const QModelIndex &index) {
  auto intPinIndex =
      model()->index(index.row(), InternalPinsCol, index.parent());
  QComboBox *intPinCombo{GetCombo(intPinIndex, InternalPinsCol)};
  if (intPinCombo) {
    if (mode.isEmpty()) {
      intPinCombo->setCurrentIndex(0);
      intPinCombo->setEnabled(false);
    } else {
      intPinCombo->setEnabled(true);
      auto pin = getPinSelection(index);
      intPinCombo->setEnabled(true);
      auto model = new QStringListModel{};
      QStringList list{{""}};
      list.append(m_model->packagePinModel()->GetInternalPinsList(pin, mode));
      model->setStringList(list);
      intPinCombo->setModel(model);
    }
  }
}

QString PortsView::getPinSelection(const QModelIndex &index) const {
  auto pinIndex = model()->index(index.row(), PackagePinCol, index.parent());
  QComboBox *pinCombo{GetCombo(pinIndex, PackagePinCol)};
  return pinCombo ? pinCombo->currentText() : QString{};
}

void PortsView::modeChanged(const QString &pin, const QString &mode) {
  if (pin.isEmpty()) return;

  const auto ports = m_model->getPort(pin);
  for (const auto &port : ports) {
    QModelIndexList indexes{match(port)};
    for (const auto &index : indexes) {
      QModelIndex modeIndex =
          model()->index(index.row(), ModeCol, index.parent());
      setComboData(modeIndex, ModeCol, mode);
    }
  }
}

void PortsView::intPinChanged(const QString &port, const QString &intPin) {
  if (port.isEmpty()) return;

  QModelIndexList indexes{match(port)};
  if (!indexes.isEmpty()) {
    auto index = indexes.first();
    QModelIndex modeIndex =
        model()->index(index.row(), InternalPinsCol, index.parent());
    setComboData(modeIndex, InternalPinsCol, intPin);
  }
}

void PortsView::portAssignmentChanged(const QString &port, const QString &pin,
                                      int /* unused */) {
  if (m_blockUpdate) return;
  if (m_model->exists(port, pin))
    SetPin(port, pin);
  else
    SetPin(port, QString{});
}

}  // namespace FOEDAG
