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

Task::Task(const QString &title, TaskType tType, QObject *parent)
    : QObject(parent), m_title(title), m_type(tType) {}

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
    emit statusChanged(m_status);
  }
}

TaskType Task::type() const { return m_type; }
void Task::setTaskType(TaskType newType) { m_type = newType; }

QString Task::settingsKey() const { return m_settings_key; }
void Task::setSettingsKey(QString key) { m_settings_key = key; }

QString Task::logFileReadPath() const { return m_logFilePath; }
void Task::setLogFileReadPath(QString path) { m_logFilePath = path; }

QString Task::abbreviation() const { return m_abbreviation; }
void Task::setAbbreviation(QString abbreviation) {
  m_abbreviation = abbreviation;
}

void Task::setCustomData(const CustomData &data) { m_customData = data; }

const CustomData &Task::cusomData() const { return m_customData; }

void Task::trigger() {
  if (m_status != TaskStatus::InProgress) emit taskTriggered();
}

const QVector<Task *> &Task::subTask() const { return m_subTask; }

bool Task::isValid() const { return m_valid; }

void Task::setValid(bool newValid) {
  m_valid = newValid;
  for (const auto &sub : qAsConst(m_subTask)) sub->setValid(newValid);
}

bool Task::isEnable() const { return m_enable; }

bool Task::isEnableDefault() const { return m_enableDefault; }

void Task::setEnable(bool newEnable, bool enableDefault) {
  m_enableDefault = enableDefault;
  if (m_enable != newEnable) {
    m_enable = newEnable;
    emit enableChanged();
    for (const auto &sub : qAsConst(m_subTask))
      sub->setEnable(newEnable, enableDefault);
  }
}

Task *Task::cleanTask() const { return m_clean; }

void Task::setCleanTask(Task *newClean) { m_clean = newClean; }

}  // namespace FOEDAG
