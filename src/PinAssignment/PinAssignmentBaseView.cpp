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
#include "PinAssignmentBaseView.h"

#include <QComboBox>

#include "ComboBox.h"
#include "PinsBaseModel.h"

namespace FOEDAG {

PinAssignmentBaseView::PinAssignmentBaseView(PinsBaseModel *model,
                                             QWidget *parent)
    : QTreeWidget(parent), m_model(model) {}

PinAssignmentBaseView::~PinAssignmentBaseView() { m_allCombo.clear(); }

void PinAssignmentBaseView::removeDuplications(const QString &text,
                                               QComboBox *current) {
#ifndef UPSTREAM_PINPLANNER
  if (text.isEmpty()) {
    return;
  }
#endif
  for (auto it{m_allCombo.cbegin()}; it != m_allCombo.cend(); it++) {
    if ((it.key() != current) && (it.key()->currentText() == text)) {
      it.key()->setCurrentIndex(0);
      break;
    }
  }
}

QModelIndexList PinAssignmentBaseView::match(const QString &text) const {
  int count = topLevelItemCount();
  QModelIndexList indexList;
  for (int i = 0; i < count; i++) {
    indexList = indexFromText(topLevelItem(i), text);
    if (!indexList.isEmpty()) break;
  }
  return indexList;
}

QModelIndexList PinAssignmentBaseView::indexFromText(
    QTreeWidgetItem *i, const QString &text) const {
  auto indexList = model()->match(indexFromItem(i), Qt::DisplayRole, text, -1,
                                  Qt::MatchExactly | Qt::MatchRecursive);
  return indexList;
}

#ifdef UPSTREAM_PINPLANNER
void PinAssignmentBaseView::updateInternalPinSelection(const QString &pin,
                                                       QComboBox *combo) {
  auto current = combo->currentText();

  auto mode = m_model->packagePinModel()->getMode(pin);
  auto model = new QStringListModel{combo};
  QStringList list{{""}};
  list.append(
      m_model->packagePinModel()->GetInternalPinsList(pin, mode, current));
  model->setStringList(list);
  combo->blockSignals(true);
  combo->setModel(model);
  combo->setCurrentIndex(combo->findData(current, Qt::DisplayRole));
  combo->blockSignals(false);
}
#endif

void PinAssignmentBaseView::setComboData(const QModelIndex &index,
                                         const QString &data) {
  if (auto combo = GetCombo(index)) {
    const auto index = combo->findData(data, Qt::DisplayRole);
    if (index > -1) combo->setCurrentIndex(index);
  }
}

void PinAssignmentBaseView::setComboData(const QModelIndex &index, int column,
                                         const QString &data) {
  if (auto combo = GetCombo(index, column)) {
    const auto index = combo->findData(data, Qt::DisplayRole);
    if (index > -1) combo->setCurrentIndex(index);
#ifndef UPSTREAM_PINPLANNER
    else {
      combo->setCurrentText(data);
    }
#endif
  }
}

QComboBox *PinAssignmentBaseView::CreateCombo(QWidget *parent) {
  return new ComboBox{parent};
}

}  // namespace FOEDAG
