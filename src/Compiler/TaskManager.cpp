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

#include "Reports/SynthesisReportManager.h"

namespace FOEDAG {

TaskManager::TaskManager(QObject *parent) : QObject{parent} {
  m_tasks.insert(IP_GENERATE, new Task{"IP Generate"});
  m_tasks.insert(ANALYSIS, new Task{"Analysis"});
  m_tasks.insert(ANALYSIS_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SYNTHESIS, new Task{"Synthesis"});
  m_tasks.insert(SYNTHESIS_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SYNTHESIS_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(SYNTHESIS_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(SYNTHESIS_TIMING_REPORT, new Task{"Timing report"});
  m_tasks.insert(PACKING, new Task{"Packing"});
  m_tasks.insert(PACKING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(GLOBAL_PLACEMENT, new Task{"Global Placement"});
  m_tasks.insert(GLOBAL_PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT, new Task{"Placement"});
  m_tasks.insert(PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(PLACEMENT_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(PLACEMENT_TIMING_REPORT, new Task{"Timing report"});
  m_tasks.insert(ROUTING, new Task{"Routing"});
  m_tasks.insert(ROUTING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(ROUTING_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(ROUTING_WRITE_NETLIST, new Task{"Write netlist"});
  m_tasks.insert(TIMING_SIGN_OFF, new Task{"Timing Analysis"});
  m_tasks.insert(TIMING_SIGN_OFF_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(POWER, new Task{"Power"});
  m_tasks.insert(POWER_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(BITSTREAM, new Task{"Bitstream Generation"});
  m_tasks.insert(BITSTREAM_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACE_AND_ROUTE_VIEW, new Task{"P&&R View", TaskType::Button});
  m_tasks.insert(SIMULATE_RTL, new Task{"Simulate RTL", TaskType::Button});
  m_tasks.insert(SIMULATE_GATE, new Task{"Simulate Gate", TaskType::Button});
  m_tasks.insert(SIMULATE_PNR, new Task{"Simulate PNR", TaskType::Button});
  m_tasks.insert(SIMULATE_BITSTREAM,
                 new Task{"Simulate Bitstream", TaskType::Button});

  m_tasks[PACKING]->appendSubTask(m_tasks[PACKING_CLEAN]);
  m_tasks[GLOBAL_PLACEMENT]->appendSubTask(m_tasks[GLOBAL_PLACEMENT_CLEAN]);
  m_tasks[ANALYSIS]->appendSubTask(m_tasks[ANALYSIS_CLEAN]);
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
  m_tasks[BITSTREAM]->appendSubTask(m_tasks[BITSTREAM_CLEAN]);
  m_tasks[POWER]->appendSubTask(m_tasks[POWER_CLEAN]);
  m_tasks[TIMING_SIGN_OFF]->appendSubTask(m_tasks[TIMING_SIGN_OFF_CLEAN]);

  m_tasks[SYNTHESIS_SETTINGS]->setSettingsKey("Synthesis");
  m_tasks[PLACEMENT_SETTINGS]->setSettingsKey("Placement");
  m_tasks[ROUTING_SETTINGS]->setSettingsKey("Routing");

  // These point to log files that can be opened via r-click in the task view
  // By default, sub-tasks open their parent log file, but you can set
  // a specific log file on sub-tasks as well for finer control
  // Ex: m_tasks[ANALYSIS_CLEAN]->setLogFileReadPath(<some_path>);
  m_tasks[IP_GENERATE]->setLogFileReadPath("$OSRCDIR/ip_generate.rpt");
  m_tasks[ANALYSIS]->setLogFileReadPath("$OSRCDIR/analysis.rpt");
  m_tasks[SYNTHESIS]->setLogFileReadPath("$OSRCDIR/synthesis.rpt");
  m_tasks[PACKING]->setLogFileReadPath("$OSRCDIR/packing.rpt");
  m_tasks[GLOBAL_PLACEMENT]->setLogFileReadPath(
      "$OSRCDIR/global_placement.rpt");
  m_tasks[PLACEMENT]->setLogFileReadPath("$OSRCDIR/placement.rpt");
  m_tasks[ROUTING]->setLogFileReadPath("$OSRCDIR/routing.rpt");
  m_tasks[TIMING_SIGN_OFF]->setLogFileReadPath("$OSRCDIR/timing_analysis.rpt");
  m_tasks[POWER]->setLogFileReadPath("$OSRCDIR/power_analysis.rpt");
  m_tasks[BITSTREAM]->setLogFileReadPath("$OSRCDIR/bitstream.rpt");

  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    connect((*task), &Task::statusChanged, this,
            &TaskManager::taskStateChanged);
    connect((*task), &Task::finished, this, &TaskManager::runNext);
  }
  m_taskQueue.append(m_tasks[IP_GENERATE]);
  m_taskQueue.append(m_tasks[ANALYSIS]);
  m_taskQueue.append(m_tasks[ANALYSIS_CLEAN]);
  m_taskQueue.append(m_tasks[SYNTHESIS]);
  m_taskQueue.append(m_tasks[SYNTHESIS_CLEAN]);
  m_taskQueue.append(m_tasks[PACKING]);
  m_taskQueue.append(m_tasks[PACKING_CLEAN]);
  m_taskQueue.append(m_tasks[GLOBAL_PLACEMENT]);
  m_taskQueue.append(m_tasks[GLOBAL_PLACEMENT_CLEAN]);
  m_taskQueue.append(m_tasks[PLACEMENT]);
  m_taskQueue.append(m_tasks[PLACEMENT_CLEAN]);
  m_taskQueue.append(m_tasks[ROUTING]);
  m_taskQueue.append(m_tasks[ROUTING_CLEAN]);
  m_taskQueue.append(m_tasks[TIMING_SIGN_OFF]);
  m_taskQueue.append(m_tasks[TIMING_SIGN_OFF_CLEAN]);
  m_taskQueue.append(m_tasks[POWER]);
  m_taskQueue.append(m_tasks[POWER_CLEAN]);
  m_taskQueue.append(m_tasks[BITSTREAM]);
  m_taskQueue.append(m_tasks[BITSTREAM_CLEAN]);
  m_taskQueue.append(m_tasks[SIMULATE_RTL]);
  m_taskQueue.append(m_tasks[SIMULATE_GATE]);
  m_taskQueue.append(m_tasks[SIMULATE_PNR]);
  m_taskQueue.append(m_tasks[SIMULATE_BITSTREAM]);

  m_reportManagerRegistry.registerReportManager(
      SYNTHESIS, std::make_unique<SynthesisReportManager>());
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
  m_runStack.append(m_tasks[ANALYSIS]);
  m_runStack.append(m_tasks[SYNTHESIS]);
  m_runStack.append(m_tasks[PACKING]);
  m_runStack.append(m_tasks[GLOBAL_PLACEMENT]);
  m_runStack.append(m_tasks[PLACEMENT]);
  m_runStack.append(m_tasks[ROUTING]);
  m_runStack.append(m_tasks[TIMING_SIGN_OFF]);
  m_runStack.append(m_tasks[POWER]);
  m_runStack.append(m_tasks[BITSTREAM]);
  m_taskCount = m_runStack.count();
  counter = 0;
  emit started();
  run();
}

void TaskManager::startTask(Task *t) {
  if (!m_runStack.isEmpty()) return;
  if (!t->isValid()) return;
  m_runStack.append(t);
  m_taskCount = m_runStack.count();
  counter = 0;
  emit started();
  run();
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

void TaskManager::setTaskCount(int count) { m_taskCount = count; }

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
  QString status{"Complete"};
  if (t->status() == TaskStatus::Fail) {
    status = "Failed";
  }
  emit progress(++counter, m_taskCount,
                QString("%1 %2").arg(t->title()).arg(status));

  if (m_runStack.isEmpty()) {
    emit done();
  }
}

void TaskManager::run() {
  auto task = m_runStack.first();
  if (m_taskCount) {
    const int max = m_taskCount;
    emit progress(max - m_runStack.count(), max,
                  QString("%1 Running").arg(task->title()));
  }
  cleanDownStreamStatus(task);
  m_runStack.first()->trigger();
}

void TaskManager::reset() {
  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    (*task)->setStatus(TaskStatus::None);
  }
}

void TaskManager::cleanDownStreamStatus(Task *t) {
  for (auto it{m_taskQueue.begin()}; it != m_taskQueue.end(); ++it) {
    if (*it == t) {
      // In case clean action, clean parent is required.
      if (((*it)->type() == TaskType::Clean) && (it != m_taskQueue.begin()))
        it--;
      for (; it != m_taskQueue.end(); ++it) {
        (*it)->setStatus(TaskStatus::None);
      }
      break;
    }
  }
}

TaskReportManagerRegistry &TaskManager::getReportManagerRegistry() {
  return m_reportManagerRegistry;
}

}  // namespace FOEDAG
