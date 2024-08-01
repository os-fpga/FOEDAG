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

#include <QCompleter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStringListModel>
#include <QToolButton>

#include "BufferedComboBox.h"

namespace FOEDAG {

constexpr int NameCol{0};
constexpr int AvailCol{1};
constexpr int PortsCol{2};
constexpr int ModeCol{3};
constexpr int InternalPinCol{4};

PackagePinsView::PackagePinsView(PinsBaseModel *model, QWidget *parent)
    : PinAssignmentBaseView(model, parent),
      MAX_ROWS{m_model->packagePinModel()->internalPinMax()} {
  header()->resizeSections(QHeaderView::ResizeToContents);
  setColumnCount(model->packagePinModel()->header().count() + 1);
  for (auto &h : model->packagePinModel()->header()) {
    headerItem()->setText(h.id, h.name);
    headerItem()->setToolTip(h.id, h.description);
  }

  QTreeWidgetItem *topLevelPackagePin = new QTreeWidgetItem(this);
  topLevelPackagePin->setText(NameCol, "All Pins");
  const auto banks = model->packagePinModel()->pinData();
  const bool useBallId{model->packagePinModel()->useBallId()};
  for (const auto &b : banks) {
    QTreeWidgetItem *bank = new QTreeWidgetItem(topLevelPackagePin);
    bank->setText(NameCol, b.name);
    bank->setText(AvailCol, QString::number(b.pinData.count()));
    const auto pins = b.pinData;
    for (auto &p : pins) {
      uint col = PortsCol + 1;
      QTreeWidgetItem *pinItem = new QTreeWidgetItem(bank);
      m_pinItems.append(pinItem);
      insertData(p.data, useBallId ? BallId : BallName, NameCol, pinItem);
      insertData(p.data, RefClock, col++, pinItem);
      insertData(p.data, Bank, col++, pinItem);
      insertData(p.data, ALT, col++, pinItem);
      insertData(p.data, DebugMode, col++, pinItem);
      insertData(p.data, ScanMode, col++, pinItem);
      insertData(p.data, MbistMode, col++, pinItem);
      insertData(p.data, Type, col++, pinItem);
#ifdef UPSTREAM_PINPLANNER
      insertData(p.data, Dir, col++, pinItem);
#else
      m_directionItemColumn = col;
      insertData(p.data, Direction, col++, pinItem);
#endif
      insertData(p.data, Voltage, col++, pinItem);
      insertData(p.data, PowerPad, col++, pinItem);
      insertData(p.data, Discription, col++, pinItem);
      insertData(p.data, Voltage2, col++, pinItem);

      initLine(pinItem);

      auto [widget, button] =
          prepareButtonWithLabel(pinItem->text(0), QIcon{":/images/add.png"});
#ifdef UPSTREAM_PINPLANNER
      connect(button, &QToolButton::clicked, this, [=]() {
        CreateNewLine(pinItem);
        auto btn = itemWidget(pinItem, NameCol)->findChild<QToolButton *>();
        if (btn) btn->setDisabled(pinItem->childCount() >= MAX_ROWS);
      });
#else
      button->hide();
#endif
      setItemWidget(pinItem, NameCol, widget);
    }
    expandItem(bank);
  }
#ifdef UPSTREAM_PINPLANNER
  connect(model->packagePinModel(), &PackagePinsModel::modeHasChanged, this,
          &PackagePinsView::modeChanged);
  connect(model->packagePinModel(), &PackagePinsModel::internalPinHasChanged,
          this, &PackagePinsView::internalPinChanged);
#endif
  connect(model, &PinsBaseModel::portAssignmentChanged, this,
          &PackagePinsView::portAssignmentChanged);
  connect(model->packagePinModel(), &PackagePinsModel::pinNameChanged, this,
          &PackagePinsView::updatePinNames);
  expandItem(topLevelPackagePin);
  setAlternatingRowColors(true);
  setColumnWidth(NameCol, 200);
  setColumnWidth(ModeCol, 180);
  setColumnWidth(InternalPinCol, 170);
  // temporary hide columns since no data available
  headerItem()->setText(InternalPinCol + 1, QString{});
  headerItem()->setToolTip(InternalPinCol + 1, QString{});
  for (int i = ModeCol; i < columnCount() - 1; i++) hideColumn(i);

  const int lastCol = columnCount() - 1;
  resizeColumnToContents(lastCol);
  header()->setSectionResizeMode(lastCol, QHeaderView::Fixed);
  headerItem()->setText(lastCol, QString{});
  headerItem()->setToolTip(lastCol, QString{});
}

#ifdef UPSTREAM_PINPLANNER
void PackagePinsView::SetMode(const QString &pin, const QString &mode) {
  QModelIndexList indexes{match(pin)};
  for (const auto &index : indexes) {
    setComboData(index, ModeCol, mode);
  }
}

void PackagePinsView::SetInternalPin(const QString &port,
                                     const QString &intPin) {
  for (auto it{m_allCombo.cbegin()}; it != m_allCombo.cend(); it++) {
    if (it.key()->currentIndex() != 0 && it.key()->currentText() == port) {
      auto index = it.value();
      setComboData(index, InternalPinCol, intPin);
      break;
    }
  }
}
#endif

void PackagePinsView::SetPort(const QString &pin, const QString &port,
                              int row) {
  if (pin.isEmpty()) return;
  if (row == -1) return;

  QModelIndexList indexes{match(pin)};
#ifdef UPSTREAM_PINPLANNER
  if (indexes.count() == 1 && row != 0) {  // make first row as child
    CreateNewLine(itemFromIndex(indexes.first()));
  }
  indexes = match(pin);
  if (row != 0) {
    if (!indexes.isEmpty()) {
      while ((indexes.count() - 1) <= row) {
        CreateNewLine(itemFromIndex(indexes.first()));
        indexes = match(pin);
      }
    }
  }

  if (indexes.count() > 1) indexes.pop_front();  // skip first parent item

#endif
#ifdef UPSTREAM_PINPLANNER
  if (!indexes.isEmpty()) {
#else
    if (row >= 1) {
      row = 0;
    }
  if (row < indexes.size()) {
#endif
    auto index = indexes.at(row);
    setComboData(index, PortsCol, port);
  }
}

void PackagePinsView::cleanTable() {
  for (auto it{m_allCombo.cbegin()}; it != m_allCombo.cend(); it++) {
    it.key()->setCurrentIndex(0);
  }
  for (const auto &item : m_pinItems) {
    while (item->childCount() != 0) {
      auto child = item->child(0);
      removeItem(item, child);
    }
  }
}

void PackagePinsView::ioPortsSelectionHasChanged(const QModelIndex &index) {
  // update here Mode selection
  auto item = itemFromIndex(index);
  auto combo = item ? GetCombo<BufferedComboBox *>(item, PortsCol) : nullptr;
#ifdef UPSTREAM_PINPLANNER
  if (combo) {
    updateModeCombo(combo->currentText(), index);
  }
#endif

  if (combo) {
    auto port = combo->currentText();
    removeDuplications(port, combo);

    auto pin = item->text(NameCol);
#ifdef UPSTREAM_PINPLANNER
    auto prevPort = combo->previousText();
#endif
    int index = item->parent() ? item->parent()->indexOfChild(item) : 0;
    m_blockUpdate = true;
#ifdef UPSTREAM_PINPLANNER // to fix https://github.com/QL-Proprietary/aurora2/issues/705
    if (!prevPort.isEmpty()) m_model->update(prevPort, QString{}, index);
#endif
    m_model->update(port, pin, index);
    m_blockUpdate = false;
    emit selectionHasChanged();
  }
}

#ifdef UPSTREAM_PINPLANNER
void PackagePinsView::modeSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = GetCombo(item, ModeCol);
    if (combo) {
      m_model->packagePinModel()->updateMode(item->text(NameCol),
                                             combo->currentText());
      updateInternalPinCombo(combo->currentText(), index);
      emit selectionHasChanged();
    }
  }
}

void PackagePinsView::internalPinSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = GetCombo(item, InternalPinCol);
    if (combo) {
      m_model->packagePinModel()->updateInternalPin(GetPort(index),
                                                    combo->currentText());
      const auto combos{m_intPins[item->text(NameCol)]};
      for (auto c : combos) {
        if (combo != c)
          updateInternalPinSelection(itemFromIndex(index)->text(NameCol), c);
      }
      emit selectionHasChanged();
    }
  }
}
#endif

void PackagePinsView::insertData(const QStringList &data, int index, int column,
                                 QTreeWidgetItem *item) {
  if (data.count() > index) item->setText(column, data.at(index));
}

#ifdef UPSTREAM_PINPLANNERs
void PackagePinsView::updateModeCombo(const QString &port,
                                      const QModelIndex &index) {
  auto modeIndex = model()->index(index.row(), ModeCol, index.parent());
  auto pin = itemFromIndex(modeIndex)->text(NameCol);
  QComboBox *modeCombo{GetCombo(modeIndex, ModeCol)};
  if (modeCombo) {
    modeCombo->setEnabled(!port.isEmpty());
    if (port.isEmpty()) {
      const QSignalBlocker blocker{modeCombo};
      modeCombo->setCurrentIndex(0);
      // update model here
      bool resetMode{true};
      const auto indexes{match(pin)};
      for (const auto &idx : indexes) {
        if (auto combo = GetCombo(idx, PortsCol)) {
          if (combo->currentIndex() != 0) resetMode = false;
        }
      }
      if (resetMode) m_model->packagePinModel()->updateMode(pin, QString{});
      // cleanup internal pin selection
      updateInternalPinCombo(QString{}, index);
    } else {
      auto currentMode = m_model->packagePinModel()->getMode(pin);
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
        const int index = modeCombo->findData(currentMode, Qt::DisplayRole);
        if (index > -1) modeCombo->setCurrentIndex(index);
        updateInternalPinCombo(currentMode, modeIndex);
      }
      modeCombo->blockSignals(false);
    }
  }
}

void PackagePinsView::updateInternalPinCombo(const QString &mode,
                                             const QModelIndex &index) {
  auto intPinIndex =
      model()->index(index.row(), InternalPinCol, index.parent());
  QComboBox *intPinCombo{GetCombo(intPinIndex, InternalPinCol)};
  if (intPinCombo) {
    if (mode.isEmpty()) {
      intPinCombo->setCurrentIndex(0);
      intPinCombo->setEnabled(false);
    } else {
      intPinCombo->setEnabled(true);
      auto current = intPinCombo->currentText();
      auto pin = itemFromIndex(index)->text(NameCol);
      auto model = new QStringListModel{};
      QStringList list{{""}};
      list.append(m_model->packagePinModel()->GetInternalPinsList(pin, mode));
      model->setStringList(list);
      intPinCombo->setModel(model);
      if (list.contains(current)) {
        const int index = intPinCombo->findData(current, Qt::DisplayRole);
        if (index > -1) intPinCombo->setCurrentIndex(index);
      }
    }
  }
}
#endif

std::pair<QWidget *, QToolButton *> PackagePinsView::prepareButtonWithLabel(
    const QString &text, const QIcon &icon) {
  QWidget *w = new QWidget;
  w->setLayout(new QHBoxLayout);
  w->layout()->setContentsMargins(0, 0, 0, 0);
  w->layout()->addWidget(new QLabel{text});
  auto btn = new QToolButton{};
  btn->setIcon(icon);
  w->layout()->addWidget(btn);
  w->setAutoFillBackground(true);
  return std::make_pair(w, btn);
}

void PackagePinsView::initLine(QTreeWidgetItem *item) {
  auto combo = new BufferedComboBox;

#ifdef UPSTREAM_PINPLANNER
  combo->setModel(m_model->portsModel()->listModel());
#else
  QString direction = item->text(m_directionItemColumn);
  combo->setModel(m_model->portsModel()->listModel(direction));
#endif

  combo->setAutoFillBackground(true);
  combo->setEditable(true);
#ifdef UPSTREAM_PINPLANNER
  auto completer{new QCompleter{m_model->portsModel()->listModel()}};
#else
  auto completer{new QCompleter{m_model->portsModel()->listModel(direction)}};
#endif
  completer->setFilterMode(Qt::MatchContains);
  combo->setCompleter(completer);
  combo->setInsertPolicy(QComboBox::NoInsert);
  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() { ioPortsSelectionHasChanged(indexFromItem(item, PortsCol)); });
#ifndef UPSTREAM_PINPLANNER
  connect(combo, &QComboBox::currentTextChanged, this,
          [=]() { ioPortsSelectionHasChanged(indexFromItem(item, PortsCol)); });
#endif
  connect(combo, &QComboBox::destroyed, this,
          [=]() { m_allCombo.remove(combo); });
  setItemWidget(item, PortsCol, combo);
  m_allCombo.insert(combo, indexFromItem(item));

#ifdef UPSTREAM_PINPLANNER
  auto modeCombo = CreateCombo(nullptr);
  modeCombo->setEnabled(false);
  connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() { modeSelectionHasChanged(indexFromItem(item, ModeCol)); });
  setItemWidget(item, ModeCol, modeCombo);

  auto internalPinCombo = CreateCombo(nullptr);
  internalPinCombo->setEnabled(false);
  internalPinCombo->setMinimumWidth(170);
  connect(internalPinCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [=]() {
            internalPinSelectionHasChanged(indexFromItem(item, ModeCol));
          });
  setItemWidget(item, InternalPinCol, internalPinCombo);
  m_intPins[item->text(NameCol)].insert(internalPinCombo);
#endif
}

void PackagePinsView::copyData(QTreeWidgetItem *from, QTreeWidgetItem *to) {
  auto fromCombo = GetCombo(from, PortsCol);
  int portIndex{0};
  int modeIndex{0};
  int intPin{0};

#ifdef UPSTREAM_PINPLANNER
  if (fromCombo) portIndex = fromCombo->currentIndex();
  fromCombo = GetCombo(from, ModeCol);
  if (fromCombo) modeIndex = fromCombo->currentIndex();
  fromCombo = GetCombo(from, InternalPinCol);
  if (fromCombo) {
    intPin = fromCombo->currentIndex();
    m_intPins[from->text(NameCol)].remove(fromCombo);
  }
#endif

  for (auto column : {PortsCol, ModeCol, InternalPinCol})
    removeItemWidget(from, column);

  auto toCombo = GetCombo(to, PortsCol);
  if (toCombo) toCombo->setCurrentIndex(portIndex);
#ifdef UPSTREAM_PINPLANNER
  toCombo = GetCombo(to, ModeCol);
  if (toCombo) toCombo->setCurrentIndex(modeIndex);
  toCombo = GetCombo(to, InternalPinCol);
  if (toCombo) toCombo->setCurrentIndex(intPin);
#endif
}

void PackagePinsView::resetItem(QTreeWidgetItem *item) {
  auto combo = GetCombo(item, InternalPinCol);
  if (combo) combo->setCurrentIndex(0);
  combo = GetCombo(item, ModeCol);
  if (combo) combo->setCurrentIndex(0);
  combo = GetCombo(item, PortsCol);
  if (combo) combo->setCurrentIndex(0);
}

void PackagePinsView::removeItem(QTreeWidgetItem *parent,
                                 QTreeWidgetItem *child) {
  if (parent->childCount() == 1) {
    initLine(parent);
    copyData(child, parent);
  } else {
#ifdef UPSTREAM_PINPLANNER
    auto combo = GetCombo(child, PortsCol);
    if (combo && combo->currentIndex() != 0) {
      m_model->remove(combo->currentText(), child->text(NameCol),
                      parent->indexOfChild(child));
    }
    auto intCombo = GetCombo(child, InternalPinCol);
    if (intCombo) m_intPins[child->text(NameCol)].remove(intCombo);
#endif
  }
  parent->removeChild(child);

  auto btn = itemWidget(parent, NameCol)->findChild<QToolButton *>();
  if (btn) btn->setDisabled(parent->childCount() >= MAX_ROWS);
}

QString PackagePinsView::GetPort(const QModelIndex &index) const {
  auto portIndex = model()->index(index.row(), PortsCol, index.parent());
  QComboBox *portCombo{GetCombo(portIndex, PortsCol)};
  return (portCombo) ? portCombo->currentText() : QString{};
}

#ifdef UPSTREAM_PINPLANNER
void PackagePinsView::modeChanged(const QString &pin, const QString &mode) {
  SetMode(pin, mode);
}

void PackagePinsView::internalPinChanged(const QString &port,
                                         const QString &intPin) {
  SetInternalPin(port, intPin);
}
#endif

void PackagePinsView::portAssignmentChanged(const QString &port,
                                            const QString &pin, int row) {
  if (m_blockUpdate) return;
  auto ports = m_model->getPort(pin);
  if (ports.contains(port))
    SetPort(pin, port, row);
  else
    SetPort(pin, QString{}, row);
}

#ifdef UPSTREAM_PINPLANNER
QTreeWidgetItem *PackagePinsView::CreateNewLine(QTreeWidgetItem *parent) {
  auto child = new QTreeWidgetItem;
  child->setText(NameCol, parent->text(NameCol));
  parent->addChild(child);

  auto [widget, button] = prepareButtonWithLabel(parent->text(NameCol),
                                                 QIcon{":/images/minus.png"});
  connect(button, &QToolButton::clicked, this,
          [=]() { removeItem(parent, child); });
  setItemWidget(child, NameCol, widget);

  initLine(child);

  if (parent->childCount() == 1) {  // remove last
    copyData(parent, child);
    expandItem(parent);
  }

  updateEditorGeometries();
  return child;
}
#endif

void PackagePinsView::updatePinNames() {
  for (auto &pinItem : m_pinItems) {
    auto widget = itemWidget(pinItem, NameCol);
    QLabel *label = widget->findChild<QLabel *>();
    if (label) {
      auto current{label->text()};
      auto convertedName =
          m_model->packagePinModel()->convertPinNameUsage(current);
      label->setText(convertedName);
      pinItem->setText(NameCol, convertedName);
    }
  }
}

}  // namespace FOEDAG
