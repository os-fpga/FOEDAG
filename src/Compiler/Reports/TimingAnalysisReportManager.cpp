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

#include "TimingAnalysisReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "Compiler.h"
#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "TableReport.h"
#include "Utils/FileUtils.h"

#ifndef PRODUCTION_BUILD
#define TIMING_SUMMARY
#endif

namespace {
static constexpr const char *DESIGN_STAT_REPORT_NAME{"STA - Design statistics"};
static constexpr const char *RESOURCE_REPORT_NAME{"STA - Utilization report"};
#ifdef TIMING_SUMMARY
static constexpr const char *TIMING_REPORT{"Timing Summary"};
#endif

static const QString LOAD_ARCH_SECTION{"# Loading Architecture Description"};
static const QString BLOCK_GRAPH_BUILD_SECTION{
    "# Building complex block graph"};
static const QString LOAD_CIRCUIT_SECTION{"# Load circuit"};
static const QString LOAD_TIM_CONSTR{"# Load Timing Constraints"};
static const QString CREATE_DEVICE_SECTION{"# Create Device"};
static const QString LOAD_PLACEMENT_SECTION{"# Load Placement"};
static const QString LOAD_ROUTING_SECTION{"# Load Routing"};

static const QString INTER_DOMAIN_PATH_DELAYS_SECTION{
    "Final inter-domain critical path delays (CPDs):"};

static const QString INTRA_DOMAIN_SETUP_SLACK_SECTION{
    "Final intra-domain worst setup slacks per constraint:"};
static const QString INTER_DOMAIN_SETUP_SLACK_SECTION{
    "Final inter-domain worst setup slacks per constraint:"};

static const QRegExp VPR_ROUTING_OPT{
    "VPR was run with the following options.*"};

static const QString BUILD_TIM_GRAPH{"Build Timing Graph"};
static const QString LOAD_PACKING{"Load packing"};

static const QRegExp FIND_TA_TIMING{"Final.*(Slack|MHz).*"};
static const QRegExp FIND_HISTOGRAM{"Final.*histogram:"};

static const QRegularExpression SPLIT_STAT_TIMING{
    "([-]?(([0-9]*[.])?[0-9]+) (ns?(?=,)|.*|MHz))"};
static const QString STATISTIC_SECTION{"Pb types usage..."};

static const QStringList TIMING_FIELDS{"Hold WNS",
                                       "Hold TNS",
                                       "Critical path delay (least slack)",
                                       "FMax",
                                       "Setup WNS",
                                       "Setup TNS",
                                       "Intra-domain period",
                                       "Fanout-weighted intra-domain period"};

// OpenSTA stuff
static const QString READ_IN_DATA{
    "This program comes with ABSOLUTELY NO WARRANTY; for details type "
    "`show_warranty'"};

static const QRegularExpression OPENSTA_TIMING{"Delay.*Time.*Description"};
static const QRegularExpression SPLIT_OPENSTA_TIMING{
    "([-]?(([0-9]*[.])?[0-9]+) \\^?)|([a-z].*)"};
}  // namespace

namespace FOEDAG {
TimingAnalysisReportManager::TimingAnalysisReportManager(
    const TaskManager &taskManager, Compiler *compiler)
    : AbstractReportManager(taskManager), m_compiler{compiler} {
  m_circuitColumns = {ReportColumn{"Logic"},
                      ReportColumn{"Used", Qt::AlignCenter},
                      ReportColumn{"Available", Qt::AlignCenter},
                      ReportColumn{"%", Qt::AlignCenter}};
  m_bramColumns = m_circuitColumns;
  m_bramColumns[0].m_name = "Block RAM";
  m_dspColumns = m_circuitColumns;
  m_dspColumns[0].m_name = "DSP";

  m_totalDesignColumn = {ReportColumn{"Total Design"}, ReportColumn{"WNS (ns)"},
                         ReportColumn{"TNS (ns)"}};

  m_intraClockColumn = {ReportColumn{"Intra-clock"},
                        ReportColumn{"Constraint (ns)"},
                        ReportColumn{"Path Delay (ns)"},
                        ReportColumn{"WNS (ns)"}, ReportColumn{"FMAX (MHz)"}};

  m_interClockColumn = m_intraClockColumn;
  m_interClockColumn[0] = ReportColumn{"Inter-clock"};

  m_openSTATimingColumns = {ReportColumn{"Delay", Qt::AlignCenter},
                            ReportColumn{"Time", Qt::AlignCenter},
                            ReportColumn{"Description"}};

  m_createDeviceKeys = {QRegExp("Device Utilization.*"),
                        QRegExp{"Build tileable routing resource graph"}};
}

void TimingAnalysisReportManager::parseStatisticLine(const QString &line) {
  AbstractReportManager::parseStatisticLine(line);
  static const QRegularExpression sWNS{
      QString{"Final setup Worst Negative Slack \\(sWNS\\): %1 ns"}.arg(
          FloatRegex())};
  auto match = sWNS.match(line);
  if (match.hasMatch()) {
    m_timingSetup.WNS = match.captured(1).toDouble();
    return;
  }
  static const QRegularExpression sTNS{
      QString{"Final setup Total Negative Slack \\(sTNS\\): "
              "%1 ns"}
          .arg(FloatRegex())};
  match = sTNS.match(line);
  if (match.hasMatch()) {
    m_timingSetup.TNS = match.captured(1).toDouble();
    return;
  }

  static const QRegularExpression hWNS{
      QString{"Final hold Worst Negative Slack \\(hWNS\\): %1 ns"}.arg(
          FloatRegex())};
  match = hWNS.match(line);
  if (match.hasMatch()) {
    m_timingHold.WNS = match.captured(1).toDouble();
    return;
  }
  static const QRegularExpression hTNS{
      QString{"Final hold Total Negative Slack \\(hTNS\\): %1 ns"}.arg(
          FloatRegex())};
  match = hTNS.match(line);
  if (match.hasMatch()) {
    m_timingHold.TNS = match.captured(1).toDouble();
    return;
  }
}

QStringList TimingAnalysisReportManager::getAvailableReportIds() const {
  return {RESOURCE_REPORT_NAME, DESIGN_STAT_REPORT_NAME
#ifdef TIMING_SUMMARY
          ,
          TIMING_REPORT
#endif
  };
}

std::unique_ptr<ITaskReport> TimingAnalysisReportManager::createReport(
    const QString &reportId) {
  if (isFileOutdated(logFile())) parseLogFile();
  if (!FileUtils::FileExists(logFile())) clean();

  ITaskReport::DataReports dataReports;

  if (reportId == QString(DESIGN_STAT_REPORT_NAME)) {
    dataReports.push_back(std::make_unique<TableReport>(
        m_resourceColumns, m_resourceData, QString{}));
  } else if (reportId == QString(RESOURCE_REPORT_NAME)) {
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
  } else {
    dataReports.push_back(std::make_unique<TableReport>(
        m_totalDesignColumn, m_totalDesignTable, QString{}, m_totalDesignMeta));
    dataReports.push_back(std::make_unique<TableReport>(
        m_intraClockColumn, m_intraClockTable, QString{}, m_intraClockMeta));
    if (!m_interClockTable.isEmpty())  // multi clock
      dataReports.push_back(std::make_unique<TableReport>(
          m_interClockColumn, m_interClockTable, QString{}, m_interClockMeta));
  }

  emit reportCreated(reportId);

  return std::make_unique<DefaultTaskReport>(std::move(dataReports), reportId);
}

QString TimingAnalysisReportManager::getTimingLogFileName() const {
  return QString(TA_TIMING_LOG);
}

bool TimingAnalysisReportManager::isStatisticalTimingLine(const QString &line) {
  if (m_compiler &&
      m_compiler->TimingAnalysisEngineOpt() == Compiler::STAEngineOpt::Opensta)
    return line.contains("wns") || line.contains("tns");

  return FIND_TA_TIMING.indexIn(line) != -1;
}

bool TimingAnalysisReportManager::isStatisticalTimingHistogram(
    const QString &line) {
  return FIND_HISTOGRAM.indexIn(line) != -1;
}

void TimingAnalysisReportManager::splitTimingData(const QString &timingStr) {
  if (m_compiler && m_compiler->TimingAnalysisEngineOpt() ==
                        Compiler::STAEngineOpt::Opensta) {
    auto timings = timingStr.simplified().split(" ");
    // TODO: This is ugly, but at this point there is no clear view on timings.
    // We expect it to consist of two KEY VALUE pairs.
    if (timings.size() == 4) {
      m_timingData.push_back({timings[0], timings[1]});
      m_timingData.push_back({timings[2], timings[3]});
    }
    return;
  }

  auto matchIt = SPLIT_STAT_TIMING.globalMatch(timingStr);
  auto valueIndex = 0;
  while (matchIt.hasNext() && valueIndex < TIMING_FIELDS.size()) {
    auto match = matchIt.next();
    m_timingData.push_back({TIMING_FIELDS[valueIndex++], match.captured()});
  }
}

void TimingAnalysisReportManager::parseLogFile() {
  clean();
  auto logFile = createLogFile();
  if (!logFile) return;

  auto timings = QStringList{};

  auto in = QTextStream(logFile.get());
  if (in.atEnd()) return;

  QString line;
  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (line.startsWith(LOAD_ARCH_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_ARCH_SECTION, {});
    else if (line.startsWith(BLOCK_GRAPH_BUILD_SECTION))
      lineNr =
          parseErrorWarningSection(in, lineNr, BLOCK_GRAPH_BUILD_SECTION, {});
    else if (line.startsWith(LOAD_CIRCUIT_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_CIRCUIT_SECTION, {});
    else if (line.startsWith(LOAD_TIM_CONSTR))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_TIM_CONSTR, {});
    else if (VPR_ROUTING_OPT.indexIn(line) != -1)
      m_messages.insert(lineNr, TaskMessage{lineNr,
                                            MessageSeverity::INFO_MESSAGE,
                                            VPR_ROUTING_OPT.cap(),
                                            {}});
    else if (line.endsWith(BUILD_TIM_GRAPH))
      m_messages.insert(
          lineNr,
          TaskMessage{
              lineNr, MessageSeverity::INFO_MESSAGE, BUILD_TIM_GRAPH, {}});
    else if (line.endsWith(LOAD_PACKING))
      m_messages.insert(
          lineNr,
          TaskMessage{lineNr, MessageSeverity::INFO_MESSAGE, LOAD_PACKING, {}});
    else if (line.startsWith(CREATE_DEVICE_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, CREATE_DEVICE_SECTION,
                                        m_createDeviceKeys);
    else if (line.startsWith(LOAD_PLACEMENT_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_PLACEMENT_SECTION, {});
    else if (line.startsWith(LOAD_ROUTING_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_ROUTING_SECTION, {});
    else if (isStatisticalTimingLine(line))
      timings << line + "\n";
    else if (isStatisticalTimingHistogram(line))
      m_histograms.push_back(qMakePair(line, parseHistogram(in, lineNr)));
    else if (line.startsWith(STATISTIC_SECTION))
      lineNr = parseStatisticsSection(in, lineNr);
    else if (line.startsWith(INTRA_DOMAIN_PATH_DELAYS_SECTION))
      lineNr = parseSection(in, lineNr, [this](const QString &line) {
        parseIntraDomPathDelaysSection(line);
      });
    else if (line.startsWith(INTRA_DOMAIN_SETUP_SLACK_SECTION))
      lineNr = parseSection(in, lineNr, [this](const QString &line) {
        parseIntraSetupSection(line);
      });
    else if (line.startsWith(INTER_DOMAIN_PATH_DELAYS_SECTION))
      lineNr = parseSection(in, lineNr, [this](const QString &line) {
        parseInterDomPathDelaysSection(line);
      });
    else if (line.startsWith(INTER_DOMAIN_SETUP_SLACK_SECTION))
      lineNr = parseSection(in, lineNr, [this](const QString &line) {
        parseInterSetupSection(line);
      });
    ++lineNr;
  }
  m_circuitData = CreateLogicData();
  m_bramData = CreateBramData();
  m_dspData = CreateDspData();
  m_ioData = CreateIOData();
  m_clockData = CreateClockData();
  m_totalDesignTable = CreateTotalDesign();
  m_intraClockTable = CreateIntraClock();
  m_interClockTable = CreateInterClock();
  designStatistics();
  validateTimingReport();

  logFile->close();

  setFileTimeStamp(this->logFile());
  emit logFileParsed();
}

std::filesystem::path TimingAnalysisReportManager::logFile() const {
  if (m_compiler)
    return m_compiler->FilePath(Compiler::Action::STA, TIMING_ANALYSIS_LOG);
  return logFilePath(TIMING_ANALYSIS_LOG);
}

void TimingAnalysisReportManager::clean() {
  AbstractReportManager::clean();
  m_messages.clear();
  m_histograms.clear();
  m_timingData.clear();
  m_resourceData.clear();
  m_circuitData.clear();
  m_bramData.clear();
  m_dspData.clear();
  m_ioData.clear();
  m_clockData.clear();
  m_totalDesignTable.clear();
  m_intraClockTable.clear();
  m_interClockTable.clear();
  m_clocksInter.clear();
  m_totalDesignMeta.clear();
  m_intraClockMeta.clear();
  m_interClockMeta.clear();
  m_timingSetup = {};
  m_timingHold = {};
}

void TimingAnalysisReportManager::validateTimingReport() {
  int colCount = m_totalDesignColumn.count();
  int rowCount = m_totalDesignTable.count();
  Resize(rowCount, colCount, m_totalDesignMeta);
  Resize(m_intraClockTable.count(), m_intraClockColumn.count(),
         m_intraClockMeta);
  Resize(m_interClockTable.count(), m_interClockColumn.count(),
         m_interClockMeta);
  if (m_timingSetup.WNS < 0) {
    for (int i = 0; i < colCount; i++)
      m_totalDesignMeta[0][i].forground = Qt::red;
  }
  if (m_timingHold.WNS < 0) {
    for (int i = 0; i < colCount; i++)
      m_totalDesignMeta[1][i].forground = Qt::red;
  }
  if (m_clocksIntra.count() == 1) {  // single clock
    if ((m_timingSetup.WNS < 0) || !m_clocksIntra.at(0).constrained) {
      for (int j = 0; j < m_intraClockMeta.at(0).count(); j++) {
        m_intraClockMeta[0][j].forground = Qt::red;
      }
    }
  } else {
    for (int i = 0; i < m_clocksIntra.count(); i++) {
      if ((m_clocksIntra.at(i).WNS < 0) || !m_clocksIntra.at(i).constrained) {
        for (int j = 0; j < m_intraClockMeta.at(i).count(); j++) {
          m_intraClockMeta[i][j].forground = Qt::red;
        }
      }
    }
  }
  for (int i = 0; i < m_clocksInter.count(); i++) {
    if (m_clocksInter.at(i).WNS < 0) {
      for (int j = 0; j < m_interClockMeta.at(i).count(); j++) {
        m_interClockMeta[i][j].forground = Qt::red;
      }
    }
  }
}

QString TimingAnalysisReportManager::ToString(double val) {
  if (val == 0) return "Met";
  return QString::number(val);
}

void TimingAnalysisReportManager::parseOpenSTALog() {
  auto logFile = createLogFile();
  if (!logFile) return;

  auto in = QTextStream(logFile.get());
  if (in.atEnd()) return;

  QString line;
  auto timings = QStringList{};

  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    if (line.contains(READ_IN_DATA))
      lineNr = parseErrorWarningSection(
          in, lineNr, READ_IN_DATA,
          {QRegExp("Startpoint:.*"), QRegExp("Endpoint:.*")}, true);
    else if (isStatisticalTimingLine(line))
      timings << line + "\n";
    else if (line.contains(OPENSTA_TIMING))
      m_histograms.push_back(qMakePair(QString("Timing table"),
                                       parseOpenSTATimingTable(in, lineNr)));
    ++lineNr;
  }

  logFile->close();

  setFileTimeStamp(this->logFile());
}

IDataReport::TableData TimingAnalysisReportManager::parseOpenSTATimingTable(
    QTextStream &in, int &lineNr) const {
  // Stop after second empty line in a row
  bool previousLineEmpty = false;
  IDataReport::TableData result;
  QString line;
  while (in.readLineInto(&line)) {
    ++lineNr;
    if (line.simplified().isEmpty()) {
      // Break if previous line was empty
      if (previousLineEmpty) break;
      // Remember that current line is empty
      previousLineEmpty = true;
      continue;
    }
    previousLineEmpty = false;
    auto match = SPLIT_OPENSTA_TIMING.globalMatch(line);
    QStringList tableLine;
    while (match.hasNext()) tableLine << match.next().captured();
    // Delay may be missing. In this case first value should be empty
    if (tableLine.size() == 2) tableLine.insert(0, {});
    // Table also contains irrelevant data - skip it
    if (tableLine.size() == 3) result.push_back(std::move(tableLine));
  }
  return result;
}

IDataReport::TableData TimingAnalysisReportManager::CreateTotalDesign() const {
  IDataReport::TableData data;
  data.push_back(
      {"Setup", ToString(m_timingSetup.WNS), ToString(m_timingSetup.TNS)});
  data.push_back(
      {"Hold", ToString(m_timingHold.WNS), ToString(m_timingHold.TNS)});
  return data;
}

IDataReport::TableData TimingAnalysisReportManager::CreateIntraClock() const {
  IDataReport::TableData data;
  if (m_clocksIntra.count() < 2) {
    const bool met = (m_timingSetup.WNS == 0);
    for (const auto &clock : m_clocksIntra) {
      if (!clock.constrained) {
        data.push_back({clock.clockName, "Unconstrained", {}, {}, {}});
      } else {
        data.push_back({clock.clockName,
                        ToString(met ? 0 : clock.pathDelay + m_timingSetup.WNS),
                        QString::number(clock.pathDelay),
                        ToString(m_timingSetup.WNS),
                        QString::number(clock.fMax)});
      }
    }
  } else if (m_clocksIntra.count() > 1) {
    for (const auto &clock : m_clocksIntra) {
      if (!clock.constrained) {
        data.push_back({clock.clockName, "Unconstrained", {}, {}, {}});
      } else {
        const bool met = (clock.WNS == 0);
        data.push_back({clock.clockName,
                        ToString(met ? 0 : clock.pathDelay + clock.WNS),
                        QString::number(clock.pathDelay), ToString(clock.WNS),
                        QString::number(clock.fMax)});
      }
    }
  }
  return data;
}

IDataReport::TableData TimingAnalysisReportManager::CreateInterClock() const {
  IDataReport::TableData data;
  for (const auto &clock : m_clocksInter) {
    data.push_back({clock.clockName,
                    QString::number(clock.pathDelay + clock.WNS),
                    QString::number(clock.pathDelay),
                    QString::number(clock.WNS), QString::number(clock.fMax)});
  }
  return data;
}

void TimingAnalysisReportManager::parseIntraSetupSection(const QString &line) {
  static const QRegularExpression lineRegex{
      QString{"(\\S+) .+worst setup slack: %1 ns"}.arg(FloatRegex())};
  auto match = lineRegex.match(line);
  if (match.hasMatch()) {
    auto clockName = match.captured(1);
    for (auto &clock : m_clocksIntra) {
      if (clock.clockName == clockName) {
        clock.WNS = match.captured(2).toDouble();
        break;
      }
    }
  }
}

void TimingAnalysisReportManager::parseInterDomPathDelaysSection(
    const QString &line) {
  static const QRegularExpression lineRegex{
      QString{"  (.+) CPD: %1 ns \\(%1 MHz\\)"}.arg(FloatRegex())};
  auto match = lineRegex.match(line);
  if (match.hasMatch()) {
    ClockData newClock;
    newClock.clockName = match.captured(1);
    newClock.pathDelay = match.captured(2).toDouble();
    newClock.fMax = match.captured(5).toDouble();
    m_clocksInter.push_back(newClock);
  }
}

void TimingAnalysisReportManager::parseInterSetupSection(const QString &line) {
  static const QRegularExpression lineRegex{
      QString{"  (.+) worst setup slack: %1 ns"}.arg(FloatRegex())};
  auto match = lineRegex.match(line);
  if (match.hasMatch()) {
    auto clockName = match.captured(1);
    for (auto &clock : m_clocksInter) {
      if (clock.clockName == clockName) {
        clock.WNS = match.captured(2).toDouble();
        break;
      }
    }
  }
}

}  // namespace FOEDAG
