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

namespace FOEDAG {

PinAssignmentBaseView::PinAssignmentBaseView(PinsBaseModel *model,
                                             QWidget *parent)
    : QTreeWidget(parent), m_model(model) {}

void PinAssignmentBaseView::removeDuplications(const QString &text,
                                               QComboBox *current) {
  for (auto &c : m_allCombo) {
    if ((c != current) && (c->currentText() == text)) {
      c->setCurrentIndex(0);
      break;
    }
  }
}

QModelIndex PinAssignmentBaseView::match(const QString &text) const {
  int count = topLevelItemCount();
  QModelIndex index;
  for (int i = 0; i < count; i++) {
    index = indexFromText(topLevelItem(i), text);
    if (index.isValid()) break;
  }
  return index;
}

QModelIndex PinAssignmentBaseView::indexFromText(QTreeWidgetItem *i,
                                                 const QString &text) const {
  auto indexList = model()->match(indexFromItem(i), Qt::DisplayRole, text, -1,
                                  Qt::MatchExactly);
  if (!indexList.isEmpty()) return indexList.first();

  int children = i->childCount();
  for (int u = 0; u < children; u++) {
    auto c = i->child(u);
    QModelIndex index = indexFromText(c, text);
    if (index.isValid()) return index;
  }
  return QModelIndex{};
}

}  // namespace FOEDAG
