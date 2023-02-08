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
}

bool TaskModel::hasChildren(const QModelIndex &parent) const {
  if (parent.isValid() && m_taskManager->task(ToTaskId(parent)))
    return m_taskManager->task(ToTaskId(parent))->hasSubTask();
  return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const {
  auto taskId = ToTaskId(index);
  auto task = m_taskManager ? m_taskManager->task(taskId) : nullptr;
  auto flags = QAbstractItemModel::flags(index);
  if (task) {
    if (task->isEnable())
      flags |= Qt::ItemIsEnabled;
    else
      flags &= ~Qt::ItemIsEnabled;
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
  if (role == TaskId) return ToTaskId(index);

  auto task = m_taskManager->task(ToTaskId(index));
  if (!task) return QVariant();

  if (role == Qt::DisplayRole && index.column() == TITLE_COL) {
    if (task->type() != TaskType::Settings) return task->title();
    return QVariant();
  } else if (role == Qt::DecorationRole) {
    if (task->type() != TaskType::Action && task->type() != TaskType::None)
      return QVariant();
    if (index.column() == STATUS_COL) {
      switch (task->status()) {
        case TaskStatus::Success:
          return QIcon(":/checked.png");
        case TaskStatus::Fail:
          return QIcon(":/failed.png");
        case TaskStatus::InProgress:
          return true;
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
    if (auto p = task->parentTask()) {
      uint id = m_taskManager->taskId(p);
      if (id != TaskManager::invalid_id) {
        return m_expanded.value(createIndex(ToRowIndex(id), index.column()),
                                true);
      }
    }
    return false;
  } else if (role == ParentDataRole) {
    return task->parentTask() != nullptr;
  } else if (role == TaskTypeRole && index.column() == TITLE_COL) {
    return QVariant((uint)task->type());
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
    auto taskId = m_taskManager->taskId(task);
    for (const auto &[row, id] : m_taskOrder) {
      if (id == taskId) {
        auto idx = createIndex(row, STATUS_COL);
        emit dataChanged(idx, idx, {Qt::DisplayRole});
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
  m_taskOrder.push_back({row++, ANALYSIS_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_RTL});
  m_taskOrder.push_back({row++, SIMULATE_RTL_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_RTL_SETTINGS});
  m_taskOrder.push_back({row++, SYNTHESIS});
  m_taskOrder.push_back({row++, SYNTHESIS_CLEAN});
  m_taskOrder.push_back({row++, SYNTHESIS_SETTINGS});
  m_taskOrder.push_back({row++, SIMULATE_GATE});
  m_taskOrder.push_back({row++, SIMULATE_GATE_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_GATE_SETTINGS});
  m_taskOrder.push_back({row++, PACKING});
  m_taskOrder.push_back({row++, PACKING_CLEAN});
  m_taskOrder.push_back({row++, PACKING_SETTINGS});
  // m_taskOrder.push_back({row++, GLOBAL_PLACEMENT});
  // m_taskOrder.push_back({row++, GLOBAL_PLACEMENT_CLEAN});
  m_taskOrder.push_back({row++, PLACEMENT});
  m_taskOrder.push_back({row++, PLACEMENT_CLEAN});
  m_taskOrder.push_back({row++, PLACEMENT_SETTINGS});
  m_taskOrder.push_back({row++, ROUTING});
  m_taskOrder.push_back({row++, ROUTING_CLEAN});
  m_taskOrder.push_back({row++, PLACE_AND_ROUTE_VIEW});
  m_taskOrder.push_back({row++, SIMULATE_PNR});
  m_taskOrder.push_back({row++, SIMULATE_PNR_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_PNR_SETTINGS});
  m_taskOrder.push_back({row++, TIMING_SIGN_OFF});
  m_taskOrder.push_back({row++, TIMING_SIGN_OFF_CLEAN});
#ifndef PRODUCTION_BUILD
  m_taskOrder.push_back({row++, TIMING_SIGN_OFF_SETTINGS});
#endif
  m_taskOrder.push_back({row++, POWER});
  m_taskOrder.push_back({row++, POWER_CLEAN});
  m_taskOrder.push_back({row++, BITSTREAM});
  m_taskOrder.push_back({row++, BITSTREAM_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_BITSTREAM});
  m_taskOrder.push_back({row++, SIMULATE_BITSTREAM_CLEAN});
  m_taskOrder.push_back({row++, SIMULATE_BITSTREAM_SETTINGS});
  for (const auto &[row, id] : m_taskOrder) appendTask(m_taskManager->task(id));
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value,
                        int role) {
  if (role == UserActionRole) {
    m_taskManager->startTask(ToTaskId(index));
    return true;
  } else if (role == ExpandAreaRole && hasChildren(index)) {
    auto task = m_taskManager->task(ToTaskId(index));
    if (task && task->isEnable()) {
      ExpandAreaAction act = value.value<ExpandAreaAction>();
      switch (act) {
        case ExpandAreaAction::Invert:
          m_expanded[index] =
              m_expanded.contains(index) ? !m_expanded[index] : true;
          break;
        case ExpandAreaAction::Expand:
          m_expanded[index] = false;
          break;
        case ExpandAreaAction::Collapse:
          m_expanded[index] = true;
          break;
      }
      emit dataChanged(
          createIndex(index.row() + 1, index.column()),
          createIndex(index.row() + task->subTask().count(), index.column()),
          {Qt::DecorationRole});
      emit layoutChanged();
    }
  }
  return QAbstractTableModel::setData(index, value, role);
}

}  // namespace FOEDAG
