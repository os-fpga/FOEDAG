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
#include "PerfomanceTracker.h"

#include <QHeaderView>
#include <QTableWidget>

#include "Compiler/CompilerDefines.h"
#include "Compiler/TaskManager.h"

namespace FOEDAG {

PerfomanceTracker::PerfomanceTracker(TaskManager *tManager)
    : m_taskManager(tManager) {
  m_view = new QTableWidget;
  m_view->setColumnCount(3);
  m_view->setHorizontalHeaderLabels({"Task", "Duration, s", "Utilization, MB"});
  m_view->verticalHeader()->hide();
  m_view->resizeColumnsToContents();
  m_view->setColumnWidth(0, 180);
  buildTable();
}

void PerfomanceTracker::update() { buildTable(); }

void PerfomanceTracker::setTaskManager(TaskManager *tManager) {
  m_taskManager = tManager;
}

QTableWidget *PerfomanceTracker::widget() { return m_view; }

void PerfomanceTracker::buildTable() {
  if (!m_taskManager) return;
  const auto taskIds = this->taskIds();

  if (m_view->rowCount() != taskIds.size()) {
    while (m_view->rowCount() != 0) m_view->removeRow(0);
    while (m_view->rowCount() < taskIds.size()) m_view->insertRow(0);
  }

  m_view->clearContents();
  int row = 0;
  for (auto taskId : taskIds) {
    auto task = m_taskManager->task(taskId);
    if (!task) continue;

    for (int i = 0; i < 3; i++) m_view->setItem(row, i, new QTableWidgetItem{});

    m_view->item(row, 0)->setText(task->title());
    m_view->item(row, 1)->setText(
        ToString(static_cast<double>(task->utilization().duration) / 1000));
    m_view->item(row, 2)->setText(
        ToString(static_cast<double>(task->utilization().utilization) / 1024));
    row++;
  }
}

QString PerfomanceTracker::ToString(double val) {
  if (val == 0) return "N/A";
  return QString::number(val);
}

QVector<uint> PerfomanceTracker::taskIds() const {
  static const auto taskIds = {ANALYSIS, SYNTHESIS,       PACKING,  PLACEMENT,
                               ROUTING,  TIMING_SIGN_OFF, BITSTREAM};
  static const auto taskIdsPostSynth = {PACKING, PLACEMENT, ROUTING,
                                        TIMING_SIGN_OFF, BITSTREAM};
  return isRtl() ? taskIds : taskIdsPostSynth;
}

bool PerfomanceTracker::isRtl() const { return m_isRtl; }

void PerfomanceTracker::setIsRtl(bool newIsRtl) {
  m_isRtl = newIsRtl;
  update();
}

}  // namespace FOEDAG
