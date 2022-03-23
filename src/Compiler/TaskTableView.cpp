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

#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>

namespace FOEDAG {

TaskTableView::TaskTableView(QWidget *parent) : QTableView(parent) {
  verticalHeader()->hide();
  setItemDelegateForColumn(1, new ChildItemDelegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(customMenuRequested(const QPoint &)));
  connect(this, &TaskTableView::actionTrigger, this,
          &TaskTableView::userActionHandle);
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
}

void TaskTableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);

  setColumnWidth(0, 30);
  setColumnWidth(1, 150);
  horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  horizontalHeader()->setStretchLastSection(true);
}

void TaskTableView::mousePressEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  bool expandAreaClicked = expandArea(idx).contains(event->pos());
  if (expandAreaClicked) {
    model()->setData(idx, expandAreaClicked, ExpandAreaRole);
  }
  QTableView::mousePressEvent(event);
}

void TaskTableView::mouseDoubleClickEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  if (idx.isValid() && !expandArea(idx).contains(event->pos())) {
    emit actionTrigger(idx);
  }
  QTableView::mouseDoubleClickEvent(event);
}

void TaskTableView::customMenuRequested(const QPoint &pos) {
  QModelIndex index = indexAt(pos);
  QMenu *menu = new QMenu(this);
  QAction *start = new QAction("Run", this);
  connect(start, &QAction::triggered, this,
          [this, index]() { emit actionTrigger(index); });
  menu->addAction(start);
  menu->popup(viewport()->mapToGlobal(pos));
}

void TaskTableView::userActionHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionRole);
}

void TaskTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight,
                                const QVector<int> &roles) {
  if (roles.contains(Qt::DecorationRole)) {
    setRowHidden(bottomRight.row(),
                 model()->data(bottomRight, RowVisibilityRole).toBool());
  }
  QTableView::dataChanged(topLeft, bottomRight, roles);
}

QRect TaskTableView::expandArea(const QModelIndex &index) const {
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.rect = visualRect(index);
  auto r = style()->proxy()->subElementRect(QStyle::SE_ItemViewItemDecoration,
                                            &opt, this);
  int h = rowHeight(0);
  r.setSize({h, h});
  return r;
}

void ChildItemDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const {
  if (index.data(ParentDataRole).toBool()) {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    opt.rect.setTopLeft(opt.rect.topLeft() - QPoint(-30, 0));
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    return;
  }
  QStyledItemDelegate::paint(painter, option, index);
}

}  // namespace FOEDAG
