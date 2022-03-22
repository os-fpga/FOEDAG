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

Task::Task(QObject *parent) : QObject{parent} {}

Task::Task(const QString &title, QObject *parent)
    : QObject(parent), m_title(title) {}

const QString &Task::title() const { return m_title; }

void Task::setTitle(const QString &newTitle) { m_title = newTitle; }

TaskStatus Task::status() const { return m_status; }

void Task::setStatus(TaskStatus newStatus) {
  if (m_status != newStatus) {
    m_status = newStatus;
    emit statusChanged();
  }
}

void Task::trigger() { emit taskTriggered(); }

}  // namespace FOEDAG
