#include "RoutingReportManager.h"

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
static const QRegExp FIND_INIT_ROUTER{"Initializing router criticalities"};
static const QRegExp FIND_NET_CONNECTION{
    "Final Net Connection Criticality Histogram"};
static const QRegExp FIND_ROUTING_TIMING{"Final.*(Slack|MHz).*"};
static const QRegExp ROUTING_SUMMARY{"Circuit successfully routed.*"};
static const QRegExp TIMING_INFO{"Final hold Worst Negative Slack.*"};

static constexpr const char *RESOURCE_REPORT_NAME{
    "Post routing - Utilization report"};
static constexpr const char *DESIGN_STAT_REPORT_NAME{
    "Post routing - Design statistics"};

static const QString LOAD_PLACEMENT_SECTION{"# Load Placement"};
static const QString COMPUT_ROUTER_SECTION{"# Computing router lookahead map"};
static const QString ROUTING_SECTION{"# Routing"};

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
}  // namespace

namespace FOEDAG {

RoutingReportManager::RoutingReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {
  m_circuitColumns = {ReportColumn{"Logic"},
                      ReportColumn{"Used", Qt::AlignCenter},
                      ReportColumn{"Available", Qt::AlignCenter},
                      ReportColumn{"%", Qt::AlignCenter}};
  m_bramColumns = m_circuitColumns;
  m_bramColumns[0].m_name = "Block RAM";
  m_dspColumns = m_circuitColumns;
  m_dspColumns[0].m_name = "DSP";

  m_routingKeys = {QRegExp("Circuit Statistics:.*"),
                   QRegExp("Final Net Connection Criticality Histogram")};
}

QStringList RoutingReportManager::getAvailableReportIds() const {
  return {QString(RESOURCE_REPORT_NAME), QString(DESIGN_STAT_REPORT_NAME)};
}

std::unique_ptr<ITaskReport> RoutingReportManager::createReport(
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

void RoutingReportManager::parseLogFile() {
  clean();
  auto logFile = createLogFile();
  if (!logFile) return;

  QTextStream in(logFile.get());
  if (in.atEnd()) return;
  auto timings = QStringList{};
  auto lineNr = 0;
  QString line;
  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (line.startsWith(LOAD_PLACEMENT_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_PLACEMENT_SECTION, {});
    else if (line.startsWith(COMPUT_ROUTER_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, COMPUT_ROUTER_SECTION, {});
    else if (isStatisticalTimingLine(line))
      timings << line + "\n";
    else if (isStatisticalTimingHistogram(line))
      m_histograms.push_back(qMakePair(line, parseHistogram(in, lineNr)));
    else if (line.startsWith(ROUTING_SECTION))
      lineNr =
          parseErrorWarningSection(in, lineNr, ROUTING_SECTION, m_routingKeys);
    else if (ROUTING_SUMMARY.indexIn(line) != -1)
      m_messages.insert(lineNr, TaskMessage{lineNr,
                                            MessageSeverity::INFO_MESSAGE,
                                            ROUTING_SUMMARY.cap(),
                                            {}});
    else if (TIMING_INFO.indexIn(line) != -1)
      m_messages.insert(
          lineNr,
          TaskMessage{
              lineNr, MessageSeverity::INFO_MESSAGE, TIMING_INFO.cap(), {}});
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

std::filesystem::path RoutingReportManager::logFile() const {
  if (m_compiler)
    return m_compiler->FilePath(Compiler::Action::Routing, ROUTING_LOG);
  return logFilePath(ROUTING_LOG);
}

void RoutingReportManager::clean() {
  AbstractReportManager::clean();
  m_messages.clear();
  m_circuitData.clear();
  m_histograms.clear();
  m_resourceData.clear();
  m_timingData.clear();
  m_bramData.clear();
  m_dspData.clear();
  m_ioData.clear();
  m_clockData.clear();
}

QString RoutingReportManager::getTimingLogFileName() const {
  return QString(ROUTING_TIMING_LOG);
}

bool RoutingReportManager::isStatisticalTimingHistogram(const QString &line) {
  return FIND_HISTOGRAM.indexIn(line) != -1;
}

bool RoutingReportManager::isStatisticalTimingLine(const QString &line) {
  return FIND_ROUTING_TIMING.indexIn(line) != -1;
}

void RoutingReportManager::splitTimingData(const QString &timingStr) {
  auto matchIt = SPLIT_STAT_TIMING.globalMatch(timingStr);
  auto valueIndex = 0;
  while (matchIt.hasNext() && valueIndex < TIMING_FIELDS.size()) {
    auto match = matchIt.next();
    m_timingData.push_back({TIMING_FIELDS[valueIndex++], match.captured()});
  }
}

}  // namespace FOEDAG
