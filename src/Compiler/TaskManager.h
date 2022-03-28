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

/*!
 * \brief The TaskManager class
 * Contains all tasks for the compiler and manage running of the tasks.
 */
class TaskManager : public QObject {
  Q_OBJECT
 public:
  static constexpr uint invalid_id{1000};
  explicit TaskManager(QObject *parent = nullptr);
  ~TaskManager();
  QList<Task *> tasks() const;
  /*!
   * \brief task
   * \return task by id.
   */
  Task *task(uint id) const;
  /*!
   * \brief taskId
   * \return return id of the task \a t.
   */
  uint taskId(Task *t) const;

  /*!
   * \brief stopCurrentTask. Stop all tasks that are in progress.
   */
  void stopCurrentTask();
  /*!
   * \brief status
   * \return InProgress if some of the tasks has status InProgress, otherwise
   * return None.
   */
  TaskStatus status() const;

  /*!
   * \brief startAll. Starts chain of all tasks. They will run one by one.
   */
  void startAll();

  /*!
   * \brief startTask. Start task \a t.
   */
  void startTask(Task *t);

  /*!
   * \brief startTask. Start task with id \a id.
   */
  void startTask(uint id);

  /*!
   * \brief bindTaskCommand
   * Bind command \a cmd to task \a t. Command will be run when task triggered.
   */
  void bindTaskCommand(Task *t, const std::function<void()> &cmd);

 signals:
  /*!
   * \brief taskStateChanged. Emits whenever any task change its status.
   */
  void taskStateChanged();
  /*!
   * \brief started. Emits when tasks has started running.
   */
  void started();
  /*!
   * \brief done. Emits when all tasks done.
   */
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
