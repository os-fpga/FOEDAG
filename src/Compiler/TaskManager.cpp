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
#include "TaskManager.h"

#include <QDebug>

namespace FOEDAG {

TaskManager::TaskManager(QObject *parent) : QObject{parent} {
  m_tasks.insert(IP_GENERATE, new Task{"IP Generate"});
  m_tasks.insert(SYNTHESIS, new Task{"Synthesis"});
  m_tasks.insert(SYNTHESIS_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SYNTHESIS_SETTINGS,
                 new Task{"Edit settings", TaskType::Settings});
  m_tasks.insert(SYNTHESIS_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(SYNTHESIS_TIMING_REPORT, new Task{"Timing report"});
  m_tasks.insert(PACKING, new Task{"Packing"});
  m_tasks.insert(PACKING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(GLOBAL_PLACEMENT, new Task{"Global Placement"});
  m_tasks.insert(GLOBAL_PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT, new Task{"Placement"});
  m_tasks.insert(PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT_SETTINGS,
                 new Task{"Edit settings", TaskType::Settings});
  m_tasks.insert(PLACEMENT_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(PLACEMENT_TIMING_REPORT, new Task{"Timing report"});
  m_tasks.insert(ROUTING, new Task{"Routing"});
  m_tasks.insert(ROUTING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(ROUTING_SETTINGS,
                 new Task{"Edit settings", TaskType::Settings});
  m_tasks.insert(ROUTING_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(TIMING_SIGN_OFF, new Task{"Timing Analysis"});
  m_tasks.insert(POWER, new Task{"Power"});
  m_tasks.insert(BITSTREAM, new Task{"Bitstream Generation"});

  m_tasks[PACKING]->appendSubTask(m_tasks[PACKING_CLEAN]);
  m_tasks[GLOBAL_PLACEMENT]->appendSubTask(m_tasks[GLOBAL_PLACEMENT_CLEAN]);
  m_tasks[SYNTHESIS]->appendSubTask(m_tasks[SYNTHESIS_CLEAN]);
  m_tasks[SYNTHESIS]->appendSubTask(m_tasks[SYNTHESIS_SETTINGS]);
  m_tasks[SYNTHESIS]->appendSubTask(m_tasks[SYNTHESIS_WRITE_NETLIST]);
  m_tasks[SYNTHESIS]->appendSubTask(m_tasks[SYNTHESIS_TIMING_REPORT]);
  m_tasks[PLACEMENT]->appendSubTask(m_tasks[PLACEMENT_CLEAN]);
  m_tasks[PLACEMENT]->appendSubTask(m_tasks[PLACEMENT_SETTINGS]);
  m_tasks[PLACEMENT]->appendSubTask(m_tasks[PLACEMENT_WRITE_NETLIST]);
  m_tasks[PLACEMENT]->appendSubTask(m_tasks[PLACEMENT_TIMING_REPORT]);
  m_tasks[ROUTING]->appendSubTask(m_tasks[ROUTING_CLEAN]);
  m_tasks[ROUTING]->appendSubTask(m_tasks[ROUTING_SETTINGS]);
  m_tasks[ROUTING]->appendSubTask(m_tasks[ROUTING_WRITE_NETLIST]);

  m_tasks[SYNTHESIS_SETTINGS]->setSettingsKey("Synthesis");
  m_tasks[PLACEMENT_SETTINGS]->setSettingsKey("Placement");
  m_tasks[ROUTING_SETTINGS]->setSettingsKey("Routing");

  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    connect((*task), &Task::statusChanged, this,
            &TaskManager::taskStateChanged);
    connect((*task), &Task::finished, this, &TaskManager::runNext);
  }
}

TaskManager::~TaskManager() { qDeleteAll(m_tasks); }

QList<Task *> TaskManager::tasks() const { return m_tasks.values(); }

Task *TaskManager::task(uint id) const { return m_tasks.value(id, nullptr); }

uint TaskManager::taskId(Task *t) const { return m_tasks.key(t, invalid_id); }

void TaskManager::stopCurrentTask() {
  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    if ((*task)->status() == TaskStatus::InProgress)
      (*task)->setStatus(TaskStatus::Fail);
  }
}

TaskStatus TaskManager::status() const {
  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    if ((*task)->status() == TaskStatus::InProgress)
      return TaskStatus::InProgress;
  }
  return TaskStatus::None;
}

void TaskManager::startAll() {
  if (!m_runStack.isEmpty()) return;
  reset();
  m_runStack.append(m_tasks[IP_GENERATE]);
  m_runStack.append(m_tasks[SYNTHESIS]);
  m_runStack.append(m_tasks[PACKING]);
  m_runStack.append(m_tasks[GLOBAL_PLACEMENT]);
  m_runStack.append(m_tasks[PLACEMENT]);
  m_runStack.append(m_tasks[ROUTING]);
  m_runStack.append(m_tasks[TIMING_SIGN_OFF]);
  m_runStack.append(m_tasks[POWER]);
  m_runStack.append(m_tasks[BITSTREAM]);
  run();
  emit started();
}

void TaskManager::startTask(Task *t) {
  if (!m_runStack.isEmpty()) return;
  if (!t->isValid()) return;
  m_runStack.append(t);
  run();
  emit started();
}

void TaskManager::startTask(uint id) {
  if (auto t = task(id)) startTask(t);
}

void TaskManager::bindTaskCommand(Task *t, const std::function<void()> &cmd) {
  connect(t, &Task::taskTriggered, [cmd]() { cmd(); });
  t->setValid(true);
}

void TaskManager::bindTaskCommand(uint id, const std::function<void()> &cmd) {
  if (auto t = task(id)) bindTaskCommand(t, cmd);
}

void TaskManager::runNext() {
  Task *t = qobject_cast<Task *>(sender());
  if (t) {
    if (t->status() == TaskStatus::Success) {
      m_runStack.removeAll(t);
      if (!m_runStack.isEmpty()) run();
    } else if (t->status() == TaskStatus::Fail) {
      m_runStack.clear();
    }
  }

  if (m_runStack.isEmpty()) emit done();
}

void TaskManager::run() { m_runStack.first()->trigger(); }

void TaskManager::reset() {
  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    (*task)->setStatus(TaskStatus::None);
  }
}

}  // namespace FOEDAG
