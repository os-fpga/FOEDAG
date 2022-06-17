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

#include <QAbstractTableModel>
#include <vector>

#include "Task.h"

namespace FOEDAG {

class TaskManager;
class TaskModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  TaskModel(TaskManager *tManager = nullptr, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  TaskManager *taskManager() const;
  void setTaskManager(TaskManager *newTaskManager);

 private:
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;

 private slots:
  void taskStatusChanged();

 private:
  void appendTask(Task *newTask);
  bool hasChildren(const QModelIndex &parent) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  uint ToTaskId(const QModelIndex &index) const;

 private:
  TaskManager *m_taskManager{nullptr};
  static constexpr uint STATUS_COL{0};
  static constexpr uint TITLE_COL{1};
  static constexpr uint TIMING_COL{2};
  QMap<QModelIndex, bool> m_expanded;
  std::vector<std::pair<int, uint>> m_taskOrder;
};

}  // namespace FOEDAG
