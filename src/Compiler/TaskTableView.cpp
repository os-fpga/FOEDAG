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

#include "NewProject/ProjectManager/project.h"
#include "NewProject/ProjectManager/project_manager.h"
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
    m_viewDisabled = m_taskManager->status() == TaskStatus::InProgress;
    viewport()->setEnabled(!m_viewDisabled);
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
  // We only disabled the viewport in order to have scrollbar enabled.
  // Mouse events keep on coming in this case though, so we catch them manually.
  if (m_viewDisabled) return;
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
    auto task = m_taskManager->task(
        this->model()->data(this->model()->index(i, 0), TaskId).toUInt());
    if (task && ((task->type() == TaskType::Settings) ||
                 (task->type() == TaskType::Button))) {
      auto index = this->model()->index(i, TitleCol);
      auto button = new QPushButton(task->title());
      button->setObjectName(task->title());
      if (task->type() == TaskType::Settings)
        connect(button, &QPushButton::clicked, this,
                [=]() { emit TaskDialogRequested(task->settingsKey()); });
      else
        connect(button, &QPushButton::clicked, this, [=]() {
          this->model()->setData(index, QVariant(), UserActionRole);
        });
      setIndexWidget(index, button);
    }
  }
}

void TaskTableView::customMenuRequested(const QPoint &pos) {
  // We only disabled the viewport in order to have scrollbar enabled.
  // Mouse events keep on coming in this case though, so we catch them manually.
  if (m_viewDisabled) return;
  QModelIndex index = indexAt(pos);
  if (index.column() == TitleCol) {
    auto task = m_taskManager->task(model()->data(index, TaskId).toUInt());
    if (task && task->type() != TaskType::Settings) {
      QMenu *menu = new QMenu(this);
      QAction *start = new QAction("Run", this);
      connect(start, &QAction::triggered, this,
              [this, index]() { userActionHandle(index); });
      menu->addAction(start);
      addTaskLogAction(menu, task);
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

void TaskTableView::addTaskLogAction(QMenu *menu, FOEDAG::Task *task) {
  // Grab the title and log file path of the given task
  QString title{task->title()};
  QString logFilePath{task->logFileReadPath()};
  auto parent = task->parentTask();
  // If this is a sub-task
  if (parent != nullptr) {
    // If the sub-task has its own log file
    if (!logFilePath.isEmpty() && parent != nullptr) {
      title = parent->title() + " - " + title;
      logFilePath = parent->logFileReadPath();
    } else {
      // otherwise take parent title and log file
      title = parent->title();
      logFilePath = parent->logFileReadPath();
    }
  }

  // Create View Log action and disable it if the file doesn't exist
  QString viewLogStr = "View " + title + " Logs";
  QAction *viewLog = new QAction(viewLogStr, this);
  logFilePath.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
  if (!QFile(logFilePath).exists()) {
    viewLog->setEnabled(false);
  }
  connect(viewLog, &QAction::triggered, this,
          [this, logFilePath]() { emit ViewFileRequested(logFilePath); });
  menu->addAction(viewLog);
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
