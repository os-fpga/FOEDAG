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

namespace FOEDAG {

TaskManager::TaskManager(QObject *parent) : QObject{parent} {
  m_tasks.insert(SYNTH_TASK, new Task{UserAction::Run, "Synthesis"});
  m_tasks.insert(SYNTH_TASK_SETTINGS,
                 new Task{UserAction::Settings, "Edit settings"});
  m_tasks.insert(PLACEMENT_TASK, new Task{UserAction::Run, "Placement"});
  m_tasks.insert(PLACEMENT_TASK_SETTINGS,
                 new Task{UserAction::Settings, "Edit settings"});

  m_tasks[SYNTH_TASK]->appendSubTask(m_tasks[SYNTH_TASK_SETTINGS]);
  m_tasks[PLACEMENT_TASK]->appendSubTask(m_tasks[PLACEMENT_TASK_SETTINGS]);
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

}  // namespace FOEDAG
