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
#include "TaskModel.h"

#include <QDebug>
#include <QIcon>

#include "TaskManager.h"

namespace FOEDAG {

TaskModel::TaskModel(TaskManager *tManager, QObject *parent)
    : QAbstractTableModel(parent) {
  setTaskManager(tManager);
}

void TaskModel::appendTask(Task *newTask) {
  beginInsertRows(QModelIndex(), 0, 0);
  endInsertRows();

  connect(newTask, &Task::statusChanged, this, &TaskModel::taskStatusChanged);
}

bool TaskModel::hasChildren(const QModelIndex &parent) const {
  if (parent.isValid()) return m_taskManager->task(parent.row())->hasSubTask();
  return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const {
  return QAbstractItemModel::flags(index);
}

int TaskModel::rowCount(const QModelIndex &parent) const {
  return m_taskManager ? m_taskManager->tasks().count() : 0;
}

int TaskModel::columnCount(const QModelIndex &parent) const {
  return TIMING_COL + 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const {
  if (role == Qt::DisplayRole && index.column() == TITLE_COL) {
    return m_taskManager->tasks().at(index.row())->title();
  } else if (role == Qt::DecorationRole) {
    if (index.column() == STATUS_COL) {
      switch (m_taskManager->tasks().at(index.row())->status()) {
        case TaskStatus::Success:
          return QIcon(":/checked.png");
        case TaskStatus::Fail:
          return QIcon(":/failed.png");
        case TaskStatus::InProgress:
          return QIcon(":/loading.png");
        default:
          return QVariant();
      }
    } else if (index.column() == TITLE_COL) {
      if (hasChildren(index)) {
        if (m_expanded.contains(index)) {
          if (m_expanded[index])
            return QIcon(":/next.png");
          else
            return QIcon(":/down-arrow.png");
        } else {
          return QIcon(":/down-arrow.png");
        }
      }
    }
  } else if (role == RowVisibilityRole) {
    if (auto task = m_taskManager->task(index.row())) {
      if (auto p = task->parentTask()) {
        uint id = m_taskManager->taskId(p);
        if (id != TaskManager::invalid_id) {
          return m_expanded.value(createIndex(id, index.column()), true);
        }
      }
    }
    return false;
  } else if (role == ParentDataRole) {
    if (auto task = m_taskManager->task(index.row())) {
      return task->parentTask() != nullptr;
    }
  } else if (role == TaskTypeRole && index.column() == TITLE_COL) {
    if (auto task = m_taskManager->task(index.row())) {
      return QVariant((uint)task->type());
    }
  }
  return QVariant();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
      case 0:
        return "Status";
      case 1:
        return "Task";
      case 2:
        return "Stats";
    }
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

void TaskModel::taskStatusChanged() {
  if (auto task = dynamic_cast<Task *>(sender())) {
    auto idx = createIndex(m_taskManager->tasks().indexOf(task), STATUS_COL);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
  }
}

TaskManager *TaskModel::taskManager() const { return m_taskManager; }

void TaskModel::setTaskManager(TaskManager *newTaskManager) {
  if (!newTaskManager) return;
  m_taskManager = newTaskManager;
  for (const auto &task : m_taskManager->tasks()) {
    appendTask(task);
  }
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value,
                        int role) {
  if (role == UserActionRole) {
    m_taskManager->startTask(index.row());
    return true;
  } else if (role == ExpandAreaRole && hasChildren(index)) {
    if (!m_expanded.contains(index)) {
      m_expanded.insert(index, true);
    } else {
      m_expanded[index] = !m_expanded[index];
    }
    auto task = m_taskManager->task(index.row());
    emit dataChanged(
        createIndex(index.row() + 1, index.column()),
        createIndex(index.row() + task->subTask().count(), index.column()),
        {Qt::DecorationRole});
  }
  return QAbstractTableModel::setData(index, value, role);
}

}  // namespace FOEDAG
