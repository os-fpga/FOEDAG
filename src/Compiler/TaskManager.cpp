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

#include "Compiler/Compiler.h"
#include "Compiler/CompilerDefines.h"
#include "Main/DialogProvider.h"
#include "Reports/BitstreamReportManager.h"
#include "Reports/PackingReportManager.h"
#include "Reports/PlacementReportManager.h"
#include "Reports/RoutingReportManager.h"
#include "Reports/SynthesisReportManager.h"
#include "Reports/TimingAnalysisReportManager.h"

namespace FOEDAG {

static constexpr auto CleanTitle{"Clean files"};
static constexpr auto CleanText{
    "Do you want to proceed with the cleaning of the %1?\n\nNote: All the "
    "files generated from the selected task will be deleted "
    "from disk. Also, this will trigger the cleaning of the subsequent tasks "
    "in compile order."};

static constexpr auto ParentTitle{"ParentTitle"};

TaskManager::TaskManager(Compiler *compiler, QObject *parent)
    : QObject{parent}, m_compiler(compiler) {
  qRegisterMetaType<FOEDAG::TaskStatus>("FOEDAG::TaskStatus");

  m_tasks.insert(IP_GENERATE, new Task{"IP Generate"});
  m_tasks.insert(ANALYSIS, new Task{"Analysis"});
  m_tasks.insert(ANALYSIS_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SYNTHESIS, new Task{"Synthesis"});
  m_tasks.insert(SYNTHESIS_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SYNTHESIS_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(PACKING, new Task{"Packing"});
  m_tasks.insert(PACKING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PACKING_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  // m_tasks.insert(GLOBAL_PLACEMENT, new Task{"Global Placement"});
  // m_tasks.insert(GLOBAL_PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT, new Task{"Placement"});
  m_tasks.insert(PLACEMENT_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACEMENT_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(ROUTING, new Task{"Routing"});
  m_tasks.insert(ROUTING_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(ROUTING_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(TIMING_SIGN_OFF, new Task{"Timing Analysis"});
  m_tasks.insert(TIMING_SIGN_OFF_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(TIMING_SIGN_OFF_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(POWER, new Task{"Power"});
  m_tasks.insert(POWER_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(BITSTREAM, new Task{"Bitstream Generation"});
  m_tasks.insert(BITSTREAM_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(PLACE_AND_ROUTE_VIEW, new Task{"P&&R View", TaskType::Button});
  m_tasks.insert(SIMULATE_RTL, new Task{"Simulate RTL"});
  m_tasks.insert(SIMULATE_RTL_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SIMULATE_RTL_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(SIMULATE_GATE, new Task{"Simulate Gate"});
  m_tasks.insert(SIMULATE_GATE_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SIMULATE_GATE_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(SIMULATE_PNR, new Task{"Simulate PNR"});
  m_tasks.insert(SIMULATE_PNR_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SIMULATE_PNR_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});
  m_tasks.insert(SIMULATE_BITSTREAM, new Task{"Simulate Bitstream"});
  m_tasks.insert(SIMULATE_BITSTREAM_CLEAN, new Task{"Clean", TaskType::Clean});
  m_tasks.insert(SIMULATE_BITSTREAM_SETTINGS,
                 new Task{"Edit settings...", TaskType::Settings});

  m_tasks[PACKING]->appendSubTask(m_tasks[PACKING_SETTINGS]);
  m_tasks[SYNTHESIS]->appendSubTask(m_tasks[SYNTHESIS_SETTINGS]);
  m_tasks[PLACEMENT]->appendSubTask(m_tasks[PLACEMENT_SETTINGS]);
#ifndef PRODUCTION_BUILD
  m_tasks[TIMING_SIGN_OFF]->appendSubTask(m_tasks[TIMING_SIGN_OFF_SETTINGS]);
#endif
  m_tasks[SIMULATE_RTL]->appendSubTask(m_tasks[SIMULATE_RTL_SETTINGS]);
  m_tasks[SIMULATE_GATE]->appendSubTask(m_tasks[SIMULATE_GATE_SETTINGS]);
  m_tasks[SIMULATE_PNR]->appendSubTask(m_tasks[SIMULATE_PNR_SETTINGS]);
  m_tasks[SIMULATE_BITSTREAM]->appendSubTask(
      m_tasks[SIMULATE_BITSTREAM_SETTINGS]);

  m_tasks[SYNTHESIS_SETTINGS]->setSettingsKey(SYNTH_SETTING_KEY);
  m_tasks[PLACEMENT_SETTINGS]->setSettingsKey(PLACE_SETTING_KEY);
  m_tasks[ROUTING_SETTINGS]->setSettingsKey(ROUTE_SETTING_KEY);
  m_tasks[PACKING_SETTINGS]->setSettingsKey(PACKING_SETTING_KEY);
  m_tasks[TIMING_SIGN_OFF_SETTINGS]->setSettingsKey(TIMING_SETTING_KEY);
  m_tasks[SIMULATE_RTL_SETTINGS]->setSettingsKey(SIM_RTL_SETTING_KEY);
  m_tasks[SIMULATE_GATE_SETTINGS]->setSettingsKey(SIM_GATE_SETTING_KEY);
  m_tasks[SIMULATE_PNR_SETTINGS]->setSettingsKey(SIM_PNR_SETTING_KEY);
  m_tasks[SIMULATE_BITSTREAM_SETTINGS]->setSettingsKey(
      SIM_BITSTREAM_SETTING_KEY);

  // These point to log files that can be opened via r-click in the task view
  // By default, sub-tasks open their parent log file, but you can set
  // a specific log file on sub-tasks as well for finer control
  // Ex: m_tasks[ANALYSIS_CLEAN]->setLogFileReadPath(<some_path>);
  m_tasks[IP_GENERATE]->setLogFileReadPath("$OSRCDIR/ip_generate.rpt");
  m_tasks[ANALYSIS]->setLogFileReadPath("$OSRCDIR/analysis.rpt");
  m_tasks[SYNTHESIS]->setLogFileReadPath("$OSRCDIR/synthesis.rpt");
  m_tasks[PACKING]->setLogFileReadPath("$OSRCDIR/packing.rpt");
  // m_tasks[GLOBAL_PLACEMENT]->setLogFileReadPath(
  //     "$OSRCDIR/global_placement.rpt");
  m_tasks[PLACEMENT]->setLogFileReadPath("$OSRCDIR/placement.rpt");
  m_tasks[ROUTING]->setLogFileReadPath("$OSRCDIR/routing.rpt");
  m_tasks[TIMING_SIGN_OFF]->setLogFileReadPath("$OSRCDIR/timing_analysis.rpt");
  m_tasks[POWER]->setLogFileReadPath("$OSRCDIR/power_analysis.rpt");
  m_tasks[BITSTREAM]->setLogFileReadPath("$OSRCDIR/bitstream.rpt");
  m_tasks[SIMULATE_RTL]->setLogFileReadPath("$OSRCDIR/simulation_rtl.rpt");
  m_tasks[SIMULATE_GATE]->setLogFileReadPath("$OSRCDIR/simulation_gate.rpt");
  m_tasks[SIMULATE_PNR]->setLogFileReadPath("$OSRCDIR/simulation_pnr.rpt");
  m_tasks[SIMULATE_BITSTREAM]->setLogFileReadPath(
      "$OSRCDIR/simulation_bitstream_back.rpt");

  // If a task has its abbreviation set the calls to Message() will be appended
  // with the set abbreviation when that task is the current task.
  // Current task is determine by which task has a status of InProgress
  m_tasks[IP_GENERATE]->setAbbreviation("IPG");
  m_tasks[ANALYSIS]->setAbbreviation("ANL");
  m_tasks[SYNTHESIS]->setAbbreviation("SYN");
  m_tasks[PACKING]->setAbbreviation("PAC");
  // m_tasks[GLOBAL_PLACEMENT]->setAbbreviation("GPL");
  m_tasks[PLACEMENT]->setAbbreviation("PLC");
  m_tasks[ROUTING]->setAbbreviation("RTE");
  m_tasks[TIMING_SIGN_OFF]->setAbbreviation("TMN");
  m_tasks[POWER]->setAbbreviation("PWR");
  m_tasks[BITSTREAM]->setAbbreviation("BIT");
  m_tasks[SIMULATE_RTL]->setAbbreviation("SRT");
  m_tasks[SIMULATE_GATE]->setAbbreviation("SGT");
  m_tasks[SIMULATE_PNR]->setAbbreviation("SPR");
  m_tasks[SIMULATE_BITSTREAM]->setAbbreviation("SBS");

  for (auto task = m_tasks.begin(); task != m_tasks.end(); task++) {
    connect((*task), &Task::statusChanged, this, &TaskManager::runNext);
    connect((*task), &Task::statusChanged, this,
            &TaskManager::taskStateChanged);
    connect((*task), &Task::enableChanged, this, &TaskManager::enableChanged);
  }
  m_taskQueue.append(m_tasks[IP_GENERATE]);
  m_taskQueue.append(m_tasks[ANALYSIS]);
  m_taskQueue.append(m_tasks[ANALYSIS_CLEAN]);
  m_taskQueue.append(m_tasks[SIMULATE_RTL]);
  m_taskQueue.append(m_tasks[SIMULATE_RTL_CLEAN]);
  m_taskQueue.append(m_tasks[SYNTHESIS]);
  m_taskQueue.append(m_tasks[SYNTHESIS_CLEAN]);
  m_taskQueue.append(m_tasks[SIMULATE_GATE]);
  m_taskQueue.append(m_tasks[SIMULATE_GATE_CLEAN]);
  m_taskQueue.append(m_tasks[PACKING]);
  m_taskQueue.append(m_tasks[PACKING_CLEAN]);
  m_taskQueue.append(m_tasks[PACKING_SETTINGS]);
  // m_taskQueue.append(m_tasks[GLOBAL_PLACEMENT]);
  // m_taskQueue.append(m_tasks[GLOBAL_PLACEMENT_CLEAN]);
  m_taskQueue.append(m_tasks[PLACEMENT]);
  m_taskQueue.append(m_tasks[PLACEMENT_CLEAN]);
  m_taskQueue.append(m_tasks[ROUTING]);
  m_taskQueue.append(m_tasks[ROUTING_CLEAN]);
  m_taskQueue.append(m_tasks[SIMULATE_PNR]);
  m_taskQueue.append(m_tasks[SIMULATE_PNR_CLEAN]);
  m_taskQueue.append(m_tasks[TIMING_SIGN_OFF]);
  m_taskQueue.append(m_tasks[TIMING_SIGN_OFF_CLEAN]);
  m_taskQueue.append(m_tasks[POWER]);
  m_taskQueue.append(m_tasks[POWER_CLEAN]);
  m_taskQueue.append(m_tasks[BITSTREAM]);
  m_taskQueue.append(m_tasks[BITSTREAM_CLEAN]);
  m_taskQueue.append(m_tasks[SIMULATE_BITSTREAM]);
  m_taskQueue.append(m_tasks[SIMULATE_BITSTREAM_CLEAN]);

  // bitstream is disabled by default
  m_tasks[BITSTREAM]->setEnable(false, false);

  auto synthesisReportManager = std::make_shared<SynthesisReportManager>(*this);
  connect(synthesisReportManager.get(), &AbstractReportManager::reportCreated,
          this, &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(
      SYNTHESIS, std::move(synthesisReportManager));

  auto placementReportManager = std::make_shared<PlacementReportManager>(*this);
  connect(placementReportManager.get(), &AbstractReportManager::reportCreated,
          this, &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(
      PLACEMENT, std::move(placementReportManager));
  auto routingReportManager = std::make_shared<RoutingReportManager>(*this);
  connect(routingReportManager.get(), &AbstractReportManager::reportCreated,
          this, &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(
      ROUTING, std::move(routingReportManager));
  auto taReportManager =
      std::make_shared<TimingAnalysisReportManager>(*this, compiler);
  connect(taReportManager.get(), &AbstractReportManager::reportCreated, this,
          &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(TIMING_SIGN_OFF,
                                                std::move(taReportManager));
  auto packingReportManager = std::make_shared<PackingReportManager>(*this);
  connect(packingReportManager.get(), &AbstractReportManager::reportCreated,
          this, &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(
      PACKING, std::move(packingReportManager));
  auto bitstreamReportManager = std::make_shared<BitstreamReportManager>(*this);
  connect(bitstreamReportManager.get(), &AbstractReportManager::reportCreated,
          this, &TaskManager::taskReportCreated);
  m_reportManagerRegistry.registerReportManager(
      BITSTREAM, std::move(bitstreamReportManager));
  initCleanTasks();
}

TaskManager::~TaskManager() { qDeleteAll(m_tasks); }

QList<Task *> TaskManager::tasks() const { return m_tasks.values(); }

Task *TaskManager::task(uint id) const { return m_tasks.value(id, nullptr); }

uint TaskManager::taskId(Task *t) const { return m_tasks.key(t, invalid_id); }

Task *TaskManager::currentTask() const {
  for (auto task : m_tasks) {
    if (task->status() == TaskStatus::InProgress) {
      return task;
    }
  }
  return nullptr;
}

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

void TaskManager::startAll(bool simulation) {
  if (!m_runStack.isEmpty()) return;
  if (m_compiler) m_compiler->ResetStopFlag();
  reset();
  appendTask(m_tasks[IP_GENERATE]);
  appendTask(m_tasks[ANALYSIS]);
  if (simulation) appendTask(m_tasks[SIMULATE_RTL]);
  appendTask(m_tasks[SYNTHESIS]);
  if (simulation) appendTask(m_tasks[SIMULATE_GATE]);
  appendTask(m_tasks[PACKING]);
  // appendTask(m_tasks[GLOBAL_PLACEMENT]);
  appendTask(m_tasks[PLACEMENT]);
  appendTask(m_tasks[ROUTING]);
  if (simulation) appendTask(m_tasks[SIMULATE_PNR]);
  appendTask(m_tasks[TIMING_SIGN_OFF]);
  appendTask(m_tasks[POWER]);
  appendTask(m_tasks[BITSTREAM]);
  if (simulation) appendTask(m_tasks[SIMULATE_BITSTREAM]);
  m_taskCount = m_runStack.count();
  counter = 0;
  if (m_taskCount != 0) {
    emit started();
    run();
  } else {
    emit done();
  }
}

void TaskManager::startTask(Task *t) {
  if (!m_runStack.isEmpty()) return;
  if (!t->isValid()) return;
  if (m_compiler) m_compiler->ResetStopFlag();
  if (t->type() == TaskType::Clean) {
    if (m_dialogProvider) {
      if (m_dialogProvider->question(
              CleanTitle,
              QString{CleanText}.arg(t->property(ParentTitle).toString())) !=
          UserSelection::Accept)
        return;
    }
    // put clean commands in the reverse order. Otherwise it will brake compiler
    // state machine
    for (auto &clearTask : getDownstreamCleanTasks(t))
      if (clearTask->isValid()) m_runStack.append(clearTask);
  } else {
    if (t->isValid()) m_runStack.append(t);
  }
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

void TaskManager::runNext(TaskStatus status) {
  Task *t = qobject_cast<Task *>(sender());
  if (!t) return;

  const bool finished =
      (status == TaskStatus::Success || status == TaskStatus::Fail);
  if (!finished) {
    if (status == TaskStatus::InProgress && counter == 0 && m_taskCount != 0)
      emit progress(counter, m_taskCount,
                    QString("%1 Running").arg(t->title()));
    return;
  }

  QString statusStr{"Complete"};
  if (status == TaskStatus::Fail) {
    statusStr = "Failed";
  }
  emit progress(++counter, m_taskCount,
                QString("%1 %2").arg(t->title(), statusStr));

  if (status == TaskStatus::Success) {
    m_runStack.removeAll(t);
    if (!m_runStack.isEmpty()) run();
  } else if (status == TaskStatus::Fail) {
    m_runStack.clear();
  }

  if (this->status() != TaskStatus::InProgress) emit done();
}

void TaskManager::initCleanTasks() {
  QVector<QPair<uint, uint>> tasks{{ANALYSIS, ANALYSIS_CLEAN},
                                   {SYNTHESIS, SYNTHESIS_CLEAN},
                                   {SIMULATE_RTL, SIMULATE_RTL_CLEAN},
                                   {SIMULATE_GATE, SIMULATE_GATE_CLEAN},
                                   {PACKING, PACKING_CLEAN},
                                   {PLACEMENT, PLACEMENT_CLEAN},
                                   {ROUTING, ROUTING_CLEAN},
                                   {SIMULATE_PNR, SIMULATE_PNR_CLEAN},
                                   {TIMING_SIGN_OFF, TIMING_SIGN_OFF_CLEAN},
                                   {BITSTREAM, BITSTREAM_CLEAN}};
  for (const auto &[parent, clean] : tasks) {
    m_tasks[parent]->setCleanTask(m_tasks[clean]);
    m_tasks[clean]->setProperty(ParentTitle, m_tasks[parent]->title());
  }
}

void TaskManager::run() {
  if (!m_runStack.isEmpty()) {
    auto task = m_runStack.first();
    cleanDownStreamStatus(task);
    task->trigger();
  }
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
      if (isSimulation(*it)) {
        // in case simulation task, we don't need to clean all downstream tasks
        (*it)->setStatus(TaskStatus::None);
        break;
      }
      for (; it != m_taskQueue.end(); ++it) {
        (*it)->setStatus(TaskStatus::None);
      }
      break;
    }
  }
}

const TaskReportManagerRegistry &TaskManager::getReportManagerRegistry() const {
  return m_reportManagerRegistry;
}

bool TaskManager::isSimulation(const Task *const task) {
  return task->cusomData().type == CustomDataType::Sim;
}

void TaskManager::setDialogProvider(const DialogProvider *const dProvider) {
  m_dialogProvider = dProvider;
}

void TaskManager::appendTask(Task *t) {
  if (t->isEnable() && t->isValid()) m_runStack.append(t);
}

QVector<Task *> TaskManager::getDownstreamCleanTasks(Task *t) const {
  QVector<Task *> tasks;
  for (auto it{m_taskQueue.rbegin()}; it != m_taskQueue.rend(); ++it) {
    if ((*it)->type() == TaskType::Clean) tasks.append(*it);
    if (*it == t) break;
  }
  return tasks;
}

}  // namespace FOEDAG
