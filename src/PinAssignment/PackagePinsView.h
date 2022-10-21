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
namespace FOEDAG {

class PackagePinsView : public PinAssignmentBaseView {
  Q_OBJECT
 public:
  PackagePinsView(PinsBaseModel *model, QWidget *parent = nullptr);
  void SetMode(const QString &pin, const QString &mode);

 signals:
  void selectionHasChanged();

 private:
  void ioPortsSelectionHasChanged(const QModelIndex &index);
  void modeSelectionHasChanged(const QModelIndex &index);
  void insertData(const QStringList &data, int index, int column,
                  QTreeWidgetItem *item);
  void updateModeCombo(const QString &port, const QModelIndex &index);

 private slots:
  void itemHasChanged(const QModelIndex &index, const QString &pin);
};

}  // namespace FOEDAG
