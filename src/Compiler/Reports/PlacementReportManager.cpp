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

#include "PlacementReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"

namespace {
static constexpr const char *REPORT_NAME{
    "Post placement - Report Resource Utilization"};

static const QRegExp FIND_TIMINGS{
    "Placement estimated (critical|setup).*[0-9].*"};
}  // namespace

namespace FOEDAG {

PlacementReportManager::PlacementReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {}

QStringList PlacementReportManager::getAvailableReportIds() const {
  return {QString(REPORT_NAME)};
}

std::unique_ptr<ITaskReport> PlacementReportManager::createReport(
    const QString &reportId) {
  auto logFile = createLogFile(QString(PLACEMENT_LOG));
  if (!logFile) return nullptr;

  auto columnNames = QStringList{};

  auto timings = QStringList{};
  auto resourcesData = ITaskReport::TableData{};

  auto in = QTextStream(logFile.get());
  QString line;
  while (in.readLineInto(&line)) {
    if (FIND_TIMINGS.indexIn(line) != -1)
      timings << line + "\n";
    else if (FIND_RESOURCES.indexIn(line) != -1)
      resourcesData = parseResourceUsage(in, columnNames);
  }
  logFile->close();

  createTimingReport(timings);

  emit reportCreated(QString(REPORT_NAME));

  return std::make_unique<TableReport>(columnNames, resourcesData, REPORT_NAME);
}

const ITaskReportManager::Messages &PlacementReportManager::getMessages() {
  return m_messages;
}

void PlacementReportManager::createTimingReport(const QStringList &timingData) {
  auto projectPath = Project::Instance()->projectPath();
  auto logFilePath =
      QString("%1/%2").arg(projectPath, QString(PLACEMENT_TIMING_LOG));

  auto timingLogFile = QFile(logFilePath);

  if (!timingLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;
  timingLogFile.resize(0);  // clear all previous contents

  QTextStream out(&timingLogFile);
  for (auto &line : timingData) out << line;

  timingLogFile.close();
}

}  // namespace FOEDAG
