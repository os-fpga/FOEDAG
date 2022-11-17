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

PinAssignmentBaseView::~PinAssignmentBaseView() { m_allCombo.clear(); }

void PinAssignmentBaseView::removeDuplications(const QString &text,
                                               QComboBox *current) {
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

}  // namespace FOEDAG
