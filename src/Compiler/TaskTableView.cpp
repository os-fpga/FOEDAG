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
#include "TaskTableView.h"

#include <QDebug>
#include <QHeaderView>
#include <QMenu>

namespace FOEDAG {

TaskTableView::TaskTableView(QWidget *parent) : QTableView(parent) {
  verticalHeader()->hide();
  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  setItemDelegateForColumn(0, new AlignPicCenterDelegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(customMenuRequested(const QPoint &)));
  connect(this, &TaskTableView::actionTrigger, this,
          &TaskTableView::userActionHandle);
}

void TaskTableView::customMenuRequested(const QPoint &pos) {
  QModelIndex index = indexAt(pos);

  QMenu *menu = new QMenu(this);
  QAction *start = new QAction("Start", this);
  connect(start, &QAction::triggered, this,
          [this, index]() { emit actionTrigger(Start, index); });
  menu->addAction(start);
  menu->popup(viewport()->mapToGlobal(pos));
}

void TaskTableView::userActionHandle(UserAction action,
                                     const QModelIndex &index) {
  model()->setData(index, action, Qt::UserRole + 1);
}

}  // namespace FOEDAG
