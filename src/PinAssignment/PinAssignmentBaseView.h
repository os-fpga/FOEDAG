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
#pragma once

#include <QSet>
#include <QTreeWidget>

class QComboBox;
namespace FOEDAG {

class PinsBaseModel;

/*!
 * \brief The PinAssignmentBaseView class
 * The implemenation provide common funtionality to Package pin table and Ports
 * table.
 */
class PinAssignmentBaseView : public QTreeWidget {
 public:
  PinAssignmentBaseView(PinsBaseModel *model, QWidget *parent = nullptr);
  ~PinAssignmentBaseView();

 protected:
  void removeDuplications(const QString &text, QComboBox *current);
  QModelIndexList match(const QString &text) const;
  QModelIndexList indexFromText(QTreeWidgetItem *i, const QString &text) const;
  void updateInternalPinSelection(const QString &pin, QComboBox *combo);

  template <class ComboPtr = QComboBox *>
  ComboPtr GetCombo(const QModelIndex &index) const {
    return qobject_cast<ComboPtr>(indexWidget(index));
  }
  template <class ComboPtr = QComboBox *>
  ComboPtr GetCombo(const QModelIndex &index, int column) const {
    return qobject_cast<ComboPtr>(itemWidget(itemFromIndex(index), column));
  }
  template <class Combo = QComboBox *>
  Combo GetCombo(QTreeWidgetItem *item, int column) const {
    return qobject_cast<Combo>(itemWidget(item, column));
  }
  void setComboData(const QModelIndex &index, const QString &data);
  void setComboData(const QModelIndex &index, int column, const QString &data);

  static QComboBox *CreateCombo(QWidget *parent);

 protected:
  PinsBaseModel *m_model{nullptr};
  QMap<QComboBox *, QModelIndex> m_allCombo;
  bool m_blockUpdate{false};
#ifdef UPSTREAM_PINPLANNER
  QMap<QString, QSet<QComboBox *>> m_intPins;
#endif
};

}  // namespace FOEDAG
