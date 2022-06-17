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

class TaskManager;

/*!
 * \brief The ChildItemDelegate class
 * Implements item that has parent and it looks similar like in the tree view.
 */
class ChildItemDelegate : public QStyledItemDelegate {
 public:
 protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
};

/*!
 * \brief The TaskTableView class
 * Implements view for tasl manager. Some of the rows could be expandable like
 * in the tree view.
 */
class TaskTableView : public QTableView {
  Q_OBJECT
 public:
  explicit TaskTableView(TaskManager *tManager, QWidget *parent = nullptr);
  void setModel(QAbstractItemModel *model) override;

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private slots:
  void customMenuRequested(const QPoint &pos);
  void userActionHandle(const QModelIndex &index);
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QVector<int> &roles = QVector<int>()) override;

 signals:
  void TaskDialogRequested(const QString &category);

 private:
  QRect expandArea(const QModelIndex &index) const;

 private:
  TaskManager *m_taskManager{nullptr};
  static constexpr uint TitleCol{1};
};
}  // namespace FOEDAG
