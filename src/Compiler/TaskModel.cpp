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

#include <QIcon>

#include "CompilerDefines.h"
#include "TaskGlobal.h"
#include "TaskManager.h"

namespace FOEDAG {

TaskModel::TaskModel(TaskManager *tManager, QObject *parent)
    : QAbstractTableModel(parent) {
  setTaskManager(tManager);
}

void TaskModel::appendTask(Task *newTask) {
  beginInsertRows(QModelIndex(), 0, 0);
  endInsertRows();

  connect(newTask, &Task::statusChanged, this, &TaskModel::taskStatusChanged,
          Qt::UniqueConnection);
  connect(newTask, &Task::enableChanged, this, &TaskModel::taskEnabledChanged,
          Qt::UniqueConnection);
}

bool TaskModel::hasChildren(const QModelIndex &parent) const {
  if (parent.isValid() && m_taskManager->task(ToTaskId(parent)))
    return m_taskManager->task(ToTaskId(parent))->hasSubTask();
  return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const {
  auto taskId = ToTaskId(index);
  auto task = m_taskManager ? m_taskManager->task(taskId) : nullptr;
  auto flags = QAbstractTableModel::flags(index);
  if (task) {
    if (task->isEnable())
      flags |= Qt::ItemIsEnabled;
    else
      flags = flags & ~(Qt::ItemIsEnabled);
  }
  return flags;
}

uint TaskModel::ToTaskId(const QModelIndex &index) const {
  return m_taskOrder[index.row()].second;
}

int TaskModel::ToRowIndex(uint taskId) const {
  auto it = std::find_if(
      m_taskOrder.cbegin(), m_taskOrder.cend(),
      [=](const std::pair<int, uint> &p) { return p.second == taskId; });
  if (it != m_taskOrder.cend()) return it->first;
  return -1;
}

int TaskModel::rowCount(const QModelIndex &parent) const {
  return m_taskOrder.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const {
  return TIMING_COL + 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const {
  auto taskID = ToTaskId(index);
  if (role == TaskId) return taskID;

  auto task = m_taskManager->task(taskID);
  if (!task) return QVariant();

  if (role == Qt::TextAlignmentRole && index.column() == TIMING_COL)
    return Qt::AlignCenter;

  if ((role == Qt::DisplayRole || role == Qt::ToolTipRole) &&
      index.column() == TIMING_COL) {
    auto registry =
        m_taskManager->getReportManagerRegistry().getReportManager(taskID);
    // Bitstream generation step should not display Fmax
    if (registry && taskID != BITSTREAM) {
      auto fmax = registry->FMax();
      if (!fmax.isEmpty()) return fmax;
    }
    return QString{"---"};
  }

  if (role == Qt::DisplayRole && index.column() == TITLE_COL) {
    if (task->type() != TaskType::Settings) return task->title();
    return QVariant();
  } else if (role == Qt::DecorationRole) {
    if (task->type() != TaskType::Action && task->type() != TaskType::None)
      return QVariant();
    if (index.column() == STATUS_COL) {
      return QVariant{};
    } else if (index.column() == TITLE_COL) {
      return task->icon();
    }
  } else if (role == ParentDataRole) {
    return task->parentTask() != nullptr;
  } else if (role == StatusRole) {
    return static_cast<int>(task->status());
  } else if (role == TaskTypeRole && index.column() == TITLE_COL) {
    return QVariant((uint)task->type());
  } else if (role == TaskEnabledRole) {
    return task->isEnable();
  }
  return QVariant();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
      case STATUS_COL:
        return "Run";
      case TITLE_COL:
        return "Task";
      case TIMING_COL:
        return "Fmax (MHz)";
    }
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

void TaskModel::taskStatusChanged() {
  if (auto task = dynamic_cast<Task *>(sender())) {
    auto taskId = m_taskManager->taskId(task);
    for (const auto &[row, id] : m_taskOrder) {
      if (id == taskId) {
        auto idx = createIndex(row, TITLE_COL);
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        break;
      }
    }
  }
}

void TaskModel::taskEnabledChanged() {
  if (auto task = dynamic_cast<Task *>(sender())) {
    auto taskId = m_taskManager->taskId(task);
    for (const auto &[row, id] : m_taskOrder) {
      if (id == taskId) {
        auto idx = createIndex(row, STATUS_COL);
        emit dataChanged(idx, idx, {TaskEnabledRole});
        break;
      }
    }
  }
}

TaskManager *TaskModel::taskManager() const { return m_taskManager; }

void TaskModel::setTaskManager(TaskManager *newTaskManager) {
  if (!newTaskManager) return;
  m_taskManager = newTaskManager;
  int row{0};
  m_taskOrder.push_back({row++, IP_GENERATE});
  m_taskOrder.push_back({row++, ANALYSIS});
  m_taskOrder.push_back({row++, SIMULATE_RTL});
  m_taskOrder.push_back({row++, SYNTHESIS});
  m_taskOrder.push_back({row++, SIMULATE_GATE});
  m_taskOrder.push_back({row++, PACKING});
  m_taskOrder.push_back({row++, PLACEMENT});
  m_taskOrder.push_back({row++, ROUTING});
  m_taskOrder.push_back({row++, SIMULATE_PNR});
  m_taskOrder.push_back({row++, TIMING_SIGN_OFF});
  m_taskOrder.push_back({row++, POWER});
  m_taskOrder.push_back({row++, BITSTREAM});
  m_taskOrder.push_back({row++, SIMULATE_BITSTREAM});
  for (const auto &[row, id] : m_taskOrder) appendTask(m_taskManager->task(id));
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value,
                        int role) {
  if (role == Qt::CheckStateRole) {
    m_taskManager->task(ToTaskId(index))
        ->setEnable(value.toInt() == Qt::Checked);
    return true;
  }
  if (role == UserActionRole) {
    m_taskManager->startTask(ToTaskId(index));
    return true;
  } else if (role == UserActionCleanRole) {
    auto taskId = ToTaskId(index);
    auto task = m_taskManager->task(taskId);
    if (task && task->cleanTask() != nullptr) {
      m_taskManager->startTask(task->cleanTask());
    }
    return true;
  }
  return QAbstractTableModel::setData(index, value, role);
}

}  // namespace FOEDAG
