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

#include "Compiler.h"
#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"
#include "Utils/FileUtils.h"

namespace {
static constexpr const char *RESOURCE_REPORT_NAME{
    "Post placement - Utilization report"};
static constexpr const char *DESIGN_STAT_REPORT_NAME{
    "Post placement - Design statistics"};

// Messages regexps
static const QRegExp FIND_PLACEMENT_TIMINGS{
    "Placement estimated.*(Slack|MHz).*"};
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

static const QRegularExpression FIND_STAT_TIMING{
    "([-]?(([0-9]*[.])?[0-9]+) (ns?(?=,)|.*|MHz))"};
static const QString STATISTIC_SECTION{"Pb types usage..."};

static const QStringList TIMING_FIELDS{"Critical path delay (least slack)",
                                       "FMax",
                                       "Setup WNS",
                                       "Setup TNS",
                                       "Intra-domain period",
                                       "Fanout-weighted intra-domain period"};
}  // namespace

namespace FOEDAG {

PlacementReportManager::PlacementReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {
  m_createDeviceKeys = {AbstractReportManager::FIND_RESOURCES,
                        DEVICE_UTIL_REGEXP, TILEABLE_GRAPH_REGEXP};

  m_placementKeys = {INIT_PLACEMENT_HISTOGRAM_REGEXP,
                     PLACEMENT_HISTOGRAM_REGEXP, PLACEMENT_RESOURCE_REGEXP};

  m_circuitColumns = {ReportColumn{"Logic"},
                      ReportColumn{"Used", Qt::AlignCenter},
                      ReportColumn{"Available", Qt::AlignCenter},
                      ReportColumn{"%", Qt::AlignCenter}};
  m_bramColumns = m_circuitColumns;
  m_bramColumns[0].m_name = "Block RAM";
  m_dspColumns = m_circuitColumns;
  m_dspColumns[0].m_name = "DSP";
}

QStringList PlacementReportManager::getAvailableReportIds() const {
  return {QString(RESOURCE_REPORT_NAME), QString(DESIGN_STAT_REPORT_NAME)};
}

std::unique_ptr<ITaskReport> PlacementReportManager::createReport(
    const QString &reportId) {
  if (isFileOutdated(logFile())) parseLogFile();
  if (!FileUtils::FileExists(logFile())) clean();

  ITaskReport::DataReports dataReports;

  if (reportId == QString(DESIGN_STAT_REPORT_NAME)) {
    dataReports.push_back(std::make_unique<TableReport>(
        m_resourceColumns, m_resourceData, QString{}));
  } else {
    dataReports.push_back(std::make_unique<TableReport>(
        m_circuitColumns, m_circuitData, QString{}));
    dataReports.push_back(
        std::make_unique<TableReport>(m_bramColumns, m_bramData, QString{}));
    dataReports.push_back(
        std::make_unique<TableReport>(m_dspColumns, m_dspData, QString{}));
    dataReports.push_back(
        std::make_unique<TableReport>(m_ioColumns, m_ioData, QString{}));
    dataReports.push_back(
        std::make_unique<TableReport>(m_clockColumns, m_clockData, QString{}));
  }

  emit reportCreated(reportId);

  return std::make_unique<DefaultTaskReport>(std::move(dataReports), reportId);
}

void PlacementReportManager::parseLogFile() {
  clean();
  auto logFile = createLogFile();
  if (!logFile) return;

  auto in = QTextStream(logFile.get());
  if (in.atEnd()) return;

  QString line;
  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (LOAD_PACKING_REGEXP.exactMatch(line))
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
    else if (line.startsWith(STATISTIC_SECTION))
      lineNr = parseStatisticsSection(in, lineNr);
    ++lineNr;
  }
  m_circuitData = CreateLogicData();
  m_bramData = CreateBramData();
  m_dspData = CreateDspData();
  m_ioData = CreateIOData();
  m_clockData = CreateClockData();
  designStatistics();

  logFile->close();

  setFileTimeStamp(this->logFile());
}

std::filesystem::path PlacementReportManager::logFile() const {
  if (m_compiler)
    return m_compiler->FilePath(Compiler::Action::Detailed, PLACEMENT_LOG);
  return logFilePath(PLACEMENT_LOG);
}

void PlacementReportManager::clean() {
  AbstractReportManager::clean();
  m_messages.clear();
  m_histograms.clear();
  m_resourceData.clear();
  m_timingData.clear();
  m_circuitData.clear();
  m_bramData.clear();
  m_dspData.clear();
  m_ioData.clear();
  m_clockData.clear();
}

QString PlacementReportManager::getTimingLogFileName() const {
  return QString{PLACEMENT_TIMING_LOG};
}

bool PlacementReportManager::isStatisticalTimingLine(const QString &line) {
  return FIND_PLACEMENT_TIMINGS.indexIn(line) != -1;
}

bool PlacementReportManager::isStatisticalTimingHistogram(const QString &line) {
  return PLACEMENT_HISTOGRAM_REGEXP.indexIn(line) != -1;
}

void PlacementReportManager::splitTimingData(const QString &timingStr) {
  auto matchIt = FIND_STAT_TIMING.globalMatch(timingStr);
  auto valueIndex = 0;
  while (matchIt.hasNext() && valueIndex < TIMING_FIELDS.size()) {
    auto match = matchIt.next();
    m_timingData.push_back({TIMING_FIELDS[valueIndex++], match.captured()});
  }
}

}  // namespace FOEDAG
