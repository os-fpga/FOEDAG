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
static constexpr const char *RESOURCE_REPORT_NAME{
    "Post placement - Report Resource Utilization"};
static constexpr const char *TIMING_REPORT_NAME{
    "Post placement - Report Static Timing"};

// Messages regexps
static const QRegExp FIND_PLACEMENT_TIMINGS{
    "Placement estimated (critical|setup).*[0-9].*"};
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
  return {QString(RESOURCE_REPORT_NAME), QString(TIMING_REPORT_NAME)};
}

std::unique_ptr<ITaskReport> PlacementReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  auto report = std::unique_ptr<ITaskReport>{};

  if (reportId == QString(RESOURCE_REPORT_NAME))
    report = std::make_unique<TableReport>(m_resourceColumns, m_resourceData,
                                           RESOURCE_REPORT_NAME);
  else
    report = std::make_unique<TableReport>(m_timingColumns, m_timingData,
                                           TIMING_REPORT_NAME);

  emit reportCreated(reportId);

  return report;
}

const ITaskReportManager::Messages &PlacementReportManager::getMessages() {
  if (!isFileParsed()) parseLogFile();
  return m_messages;
}

void PlacementReportManager::parseLogFile() {
  auto logFile = createLogFile(QString(PLACEMENT_LOG));
  if (!logFile) return;

  auto timings = QStringList{};
  m_messages.clear();

  auto in = QTextStream(logFile.get());
  QString line;
  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    if (FIND_RESOURCES.indexIn(line) != -1)
      parseResourceUsage(in, lineNr);
    else if (LOAD_PACKING_REGEXP.exactMatch(line))
      m_messages.insert(lineNr, TaskMessage{lineNr,
                                            MessageSeverity::INFO_MESSAGE,
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

  setFileParsed(true);
}

QString PlacementReportManager::getTimingLogFileName() const {
  return QString{PLACEMENT_TIMING_LOG};
}

bool PlacementReportManager::isStatisticalTimingLine(const QString &line) {
  return FIND_PLACEMENT_TIMINGS.indexIn(line) != -1;
}

}  // namespace FOEDAG
