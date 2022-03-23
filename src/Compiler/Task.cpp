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
#include "Task.h"

namespace FOEDAG {

Task::Task(UserAction act, QObject *parent) : QObject{parent}, m_action(act) {}

Task::Task(UserAction act, const QString &title, QObject *parent)
    : QObject(parent), m_title(title), m_action(act) {}

const QString &Task::title() const { return m_title; }

void Task::setTitle(const QString &newTitle) { m_title = newTitle; }

bool Task::hasSubTask() const { return !m_subTask.isEmpty(); }

void Task::appendSubTask(Task *t) {
  m_subTask.append(t);
  t->m_parent = this;
}

Task *Task::parentTask() const { return m_parent; }

TaskStatus Task::status() const { return m_status; }

void Task::setStatus(TaskStatus newStatus) {
  if (m_status != newStatus) {
    m_status = newStatus;
    emit statusChanged();
  }
}

void Task::trigger() {
  if (m_status != TaskStatus::InProgress) emit taskTriggered(m_action);
}

UserAction Task::action() const { return m_action; }

}  // namespace FOEDAG
