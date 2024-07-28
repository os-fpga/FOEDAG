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

#include "PinAssignmentBaseView.h"
#include "PinsBaseModel.h"

class QComboBox;
class QToolButton;
namespace FOEDAG {

class PackagePinsView : public PinAssignmentBaseView {
  Q_OBJECT
 public:
  PackagePinsView(PinsBaseModel *model, QWidget *parent = nullptr);
#ifdef UPSTREAM_PINPLANNER
  void SetMode(const QString &pin, const QString &mode);
  void SetInternalPin(const QString &port, const QString &intPin);
#endif
  void SetPort(const QString &pin, const QString &port, int row);
  void cleanTable();

 signals:
  void selectionHasChanged();

 private:
  void ioPortsSelectionHasChanged(const QModelIndex &index);
#ifdef UPSTREAM_PINPLANNER
  void modeSelectionHasChanged(const QModelIndex &index);
  void internalPinSelectionHasChanged(const QModelIndex &index);
#endif
  void insertData(const QStringList &data, int index, int column,
                  QTreeWidgetItem *item);
#ifdef UPSTREAM_PINPLANNER
  void updateModeCombo(const QString &port, const QModelIndex &index);
  void updateInternalPinCombo(const QString &mode, const QModelIndex &index);
#endif
  static std::pair<QWidget *, QToolButton *> prepareButtonWithLabel(
      const QString &text, const QIcon &icon);
  void initLine(QTreeWidgetItem *item);
  void copyData(QTreeWidgetItem *from, QTreeWidgetItem *to);
  void resetItem(QTreeWidgetItem *item);
  void removeItem(QTreeWidgetItem *parent, QTreeWidgetItem *child);
  QString GetPort(const QModelIndex &index) const;

 private slots:
 #ifdef UPSTREAM_PINPLANNER
  void modeChanged(const QString &pin, const QString &mode);
  void internalPinChanged(const QString &port, const QString &intPin);
#endif
  void portAssignmentChanged(const QString &port, const QString &pin, int row);
#ifdef UPSTREAM_PINPLANNER
  QTreeWidgetItem *CreateNewLine(QTreeWidgetItem *parent);
#endif
  void updatePinNames();

 private:
#ifndef UPSTREAM_PINPLANNER
  int m_directionItemColumn = -1;
#endif
  const int MAX_ROWS{};
  QVector<QTreeWidgetItem *> m_pinItems;
};

}  // namespace FOEDAG
