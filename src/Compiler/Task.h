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

#include <QObject>
#include <QVector>

#include "Compiler/CompilerDefines.h"

namespace FOEDAG {

enum class TaskStatus {
  None,
  InProgress,
  Success,
  Fail,
};

class Task : public QObject {
  Q_OBJECT
 public:
  explicit Task(UserAction act, QObject *parent = nullptr);
  explicit Task(UserAction act, const QString &title,
                QObject *parent = nullptr);
  const QString &title() const;
  void setTitle(const QString &newTitle);
  bool hasSubTask() const;
  void appendSubTask(Task *t);
  Task *parentTask() const;

  TaskStatus status() const;
  void setStatus(TaskStatus newStatus);

  void trigger();

  UserAction action() const;

  const QVector<Task *> &subTask() const;

 signals:
  void statusChanged();
  void taskTriggered(FOEDAG::UserAction);

 private:
  QString m_title;
  TaskStatus m_status{TaskStatus::None};
  QVector<Task *> m_subTask;
  Task *m_parent{nullptr};
  UserAction m_action;
};

}  // namespace FOEDAG
