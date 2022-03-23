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

#include <QStyledItemDelegate>
#include <QTableView>

namespace FOEDAG {

class AlignPicCenterDelegate : public QStyledItemDelegate {
 public:
 protected:
  void initStyleOption(QStyleOptionViewItem *option,
                       const QModelIndex &index) const override {
    QStyledItemDelegate::initStyleOption(option, index);
    option->displayAlignment = Qt::AlignCenter;
    option->features = QStyleOptionViewItem::ViewItemFeature::HasDecoration;
    option->decorationSize = {50, 25};
  }
};

enum UserAction {
  Start,
};

class TaskTableView : public QTableView {
  Q_OBJECT
 public:
  TaskTableView(QWidget *parent = nullptr);

 signals:
  void actionTrigger(FOEDAG::UserAction action, const QModelIndex &index);

 private slots:
  void customMenuRequested(const QPoint &pos);
  void userActionHandle(FOEDAG::UserAction action, const QModelIndex &index);
};
}  // namespace FOEDAG
