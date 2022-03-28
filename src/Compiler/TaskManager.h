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

#include <QList>
#include <QMap>
#include <QObject>

#include "Task.h"

namespace FOEDAG {

class TaskManager : public QObject {
  Q_OBJECT
 public:
  static constexpr uint invalid_id{1000};
  explicit TaskManager(QObject *parent = nullptr);
  ~TaskManager();
  QList<Task *> tasks() const;
  Task *task(uint id) const;
  uint taskId(Task *t) const;

  void stopCurrentTask();
  TaskStatus status() const;

  void startAll();
  void startTask(Task *t);
  void startTask(uint id);

 signals:
  void taskStateChanged();
  void started();
  void done();

 private slots:
  void runNext();

 private:
  void run();
  void reset();

 private:
  QMap<uint, Task *> m_tasks;
  QVector<Task *> m_runStack;
};

}  // namespace FOEDAG
