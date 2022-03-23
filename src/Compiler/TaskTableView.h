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

#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QTableView>

#include "CompilerDefines.h"

namespace FOEDAG {

class ChildItemDelegate : public QStyledItemDelegate {
 public:
 protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
};

struct TaskTableViewInternal {
  QMap<QModelIndex, bool> expand;
};

class TaskTableView : public QTableView {
  Q_OBJECT
 public:
  TaskTableView(QWidget *parent = nullptr);
  void setModel(QAbstractItemModel *model) override;

 signals:
  void actionTrigger(const QModelIndex &index);

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private slots:
  void customMenuRequested(const QPoint &pos);
  void userActionHandle(const QModelIndex &index);
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QVector<int> &roles = QVector<int>()) override;

 private:
  QRect expandArea(const QModelIndex &index) const;

 private:
  TaskTableViewInternal m_internal;
};
}  // namespace FOEDAG
