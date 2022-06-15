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
#include <QBoxLayout>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>

#include "TaskManager.h"

inline void initializeResources() { Q_INIT_RESOURCE(compiler_resources); }
namespace FOEDAG {

TaskTableView::TaskTableView(TaskManager *tManager, QWidget *parent)
    : QTableView(parent), m_taskManager(tManager) {
  verticalHeader()->hide();
  setItemDelegateForColumn(1, new ChildItemDelegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(customMenuRequested(const QPoint &)));
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  connect(m_taskManager, &TaskManager::taskStateChanged, this, [this]() {
    setEnabled(m_taskManager->status() != TaskStatus::InProgress);
  });
  initializeResources();
}

void TaskTableView::mousePressEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  if (idx.column() == TitleCol) {
    bool expandAreaClicked = expandArea(idx).contains(event->pos());
    if (expandAreaClicked) {
      model()->setData(idx, expandAreaClicked, ExpandAreaRole);
    }
  }

  QTableView::mousePressEvent(event);
}

void TaskTableView::mouseDoubleClickEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  if (idx.isValid() && (idx.column() == TitleCol) &&
      !expandArea(idx).contains(event->pos())) {
    userActionHandle(idx);
  }
  QTableView::mouseDoubleClickEvent(event);
}

void TaskTableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);
  for (int i = 0; i < this->model()->rowCount(); i++) {
    auto task = m_taskManager->task(i);
    if (task && task->type() == TaskType::Settings) {
      auto index = this->model()->index(i, TitleCol);
      auto button = new QPushButton(task->title());
      connect(button, &QPushButton::clicked, this,
              [=]() { emit TaskDialogRequested(task->settingsKey()); });
      setIndexWidget(index, button);
    }
  }
}

void TaskTableView::customMenuRequested(const QPoint &pos) {
  QModelIndex index = indexAt(pos);
  if (index.column() == TitleCol) {
    auto task = m_taskManager->task(index.row());
    if (task && task->type() != TaskType::Settings) {
      QMenu *menu = new QMenu(this);
      QAction *start = new QAction("Run", this);
      connect(start, &QAction::triggered, this,
              [this, index]() { userActionHandle(index); });
      menu->addAction(start);
      menu->popup(viewport()->mapToGlobal(pos));
    }
  }
}

void TaskTableView::userActionHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionRole);
}

void TaskTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight,
                                const QVector<int> &roles) {
  if (roles.contains(Qt::DecorationRole)) {
    auto indexRow = topLeft.row();
    while (indexRow <= bottomRight.row()) {
      setRowHidden(indexRow,
                   model()->data(bottomRight, RowVisibilityRole).toBool());
      indexRow++;
    }
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
