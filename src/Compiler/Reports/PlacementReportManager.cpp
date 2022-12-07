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
#include <set>

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"

namespace {
static constexpr const char *REPORT_NAME{
    "Post placement - Report Resource Utilization"};

static const QRegExp FIND_TIMINGS{
    "Placement estimated (critical|setup).*[0-9].*"};

// Messages regexps
static const QRegExp LOAD_PACKING_REGEXP{"# Load packing"};
static const QRegExp DEVICE_UTIL_REGEXP{"Device Utilization.*"};
static const QRegExp TILEABLE_GRAPH_REGEXP{
    "Build tileable routing resource graph"};
static const QRegExp INIT_PLACEMENT_HISTOGRAM_REGEXP{
    "Initial placement estimated setup slack histogram"};
static const QRegExp PLACEMENT_HISTOGRAM_REGEXP{
    "Placement estimated setup slack histogram"};
static const QRegExp PLACEMENT_RESOURCE_REGEXP{"Placement resource usage"};

static const QString CREATE_DEVICE_SECTION{"# Create Device"};
static const QString PLACEMENT_SECTION{"# Placement"};

}  // namespace

namespace FOEDAG {

PlacementReportManager::PlacementReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {
  m_createDeviceKeys = {AbstractReportManager::FIND_RESOURCES,
                        DEVICE_UTIL_REGEXP, TILEABLE_GRAPH_REGEXP};

  m_placementKeys = {INIT_PLACEMENT_HISTOGRAM_REGEXP,
                     PLACEMENT_HISTOGRAM_REGEXP, PLACEMENT_RESOURCE_REGEXP};
}

QStringList PlacementReportManager::getAvailableReportIds() const {
  return {QString(REPORT_NAME)};
}

std::unique_ptr<ITaskReport> PlacementReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  emit reportCreated(QString(REPORT_NAME));

  return std::make_unique<TableReport>(m_reportColumns, m_stats, REPORT_NAME);
}

const ITaskReportManager::Messages &PlacementReportManager::getMessages() {
  if (!isFileParsed()) parseLogFile();
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

void PlacementReportManager::parseLogFile() {
  auto logFile = createLogFile(QString(PLACEMENT_LOG));
  if (!logFile) return;

  auto timings = QStringList{};
  m_stats.clear();
  m_messages.clear();
  m_reportColumns.clear();

  auto in = QTextStream(logFile.get());
  QString line;
  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    if (FIND_TIMINGS.indexIn(line) != -1)
      timings << line + "\n";
    else if (FIND_RESOURCES.indexIn(line) != -1)
      m_stats = parseResourceUsage(in, m_reportColumns);
    else if (LOAD_PACKING_REGEXP.exactMatch(line))
      m_messages.insert(lineNr,
                        TaskMessage{lineNr,
                                    TaskMessage::MessageSeverity::INFO_MESSAGE,
                                    line.remove('#').simplified(),
                                    {}});
    else if (line.startsWith(CREATE_DEVICE_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, CREATE_DEVICE_SECTION,
                                        m_createDeviceKeys);
    else if (line.startsWith(PLACEMENT_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, PLACEMENT_SECTION,
                                        m_placementKeys);
    ++lineNr;
  }
  logFile->close();

  createTimingReport(timings);
}

}  // namespace FOEDAG
