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
#include <optional>

#include "Reports/TaskReportManagerRegistry.h"
#include "Task.h"

namespace FOEDAG {
class Compiler;
class DialogProvider;

/*!
 * \brief The TaskManager class
 * Contains all tasks for the compiler and manage running of the tasks.
 */
class TaskManager : public QObject {
  Q_OBJECT
 public:
  static constexpr uint invalid_id{1000};
  explicit TaskManager(Compiler *compiler, QObject *parent = nullptr);
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
   * \brief currentTask
   * \return Task* to task with status InProgress.
   */
  Task *currentTask() const;
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
   * @param simulation - add simulation tasks
   */
  void startAll(bool simulation = false);

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
  void bindTaskCommand(uint id, const std::function<void()> &cmd);

  void setTaskCount(int count);

  const TaskReportManagerRegistry &getReportManagerRegistry() const;

  static bool isSimulation(const Task *const task);
  void setDialogProvider(const DialogProvider *const dProvider);

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
  /*!
   * \brief progress
   * emits whenever current task done and send current progress and max steps.
   */
  void progress(int progress, int max, const QString &msg = {});

  void taskReportCreated(QString reportName);
  void enableChanged();

 private slots:
  void runNext(FOEDAG::TaskStatus status);

 private:
  void initCleanTasks();
  void run();
  void reset();
  void cleanDownStreamStatus(Task *t);
  void appendTask(Task *t);

  /*!
   * \brief getDownstreamClearTasks
   * \return vector of clean tasks in reverse order. Vector includes \param t.
   */
  QVector<Task *> getDownstreamCleanTasks(Task *t) const;

 private:
  QMap<uint, Task *> m_tasks;
  QVector<Task *> m_runStack;
  QVector<Task *> m_taskQueue;
  TaskReportManagerRegistry m_reportManagerRegistry;
  int m_taskCount{0};
  int counter{0};
  const DialogProvider *m_dialogProvider{nullptr};
  Compiler *m_compiler{nullptr};
};

}  // namespace FOEDAG
