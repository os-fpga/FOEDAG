#include "RoutingReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"

namespace {
static const QRegExp FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};
static const QRegExp FIND_INIT_ROUTER{"Initializing router criticalities"};
static const QRegExp FIND_NET_CONNECTION{
    "Final Net Connection Criticality Histogram"};
static const QRegExp FIND_ROUTING_TIMING{"Final.*(Slack|MHz).*"};
static const QRegExp ROUTING_SUMMARY{"Circuit successfully routed.*"};
static const QRegExp TIMING_INFO{"Final hold Worst Negative Slack.*"};

static constexpr const char *CIRCUIT_REPORT_NAME{"Circuit Statistics Report"};
static constexpr const char *RESOURCE_REPORT_NAME{
    "Post routing - Report Resource Utilization "};
static constexpr const char *TIMING_REPORT_NAME{
    "Post routing - Report Static Timing"};

static const QString LOAD_PLACEMENT_SECTION{"# Load Placement"};
static const QString COMPUT_ROUTER_SECTION{"# Computing router lookahead map"};
static const QString ROUTING_SECTION{"# Routing"};

static const QRegExp FIND_HISTOGRAM{"Final.*histogram:"};

static const QRegularExpression SPLIT_STAT_TIMING{
    "([-]?(([0-9]*[.])?[0-9]+) (ns?(?=,)|.*|MHz))"};

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
  m_circuitColumns = {ReportColumn{"Block type"},
                      ReportColumn{"Number of blocks", Qt::AlignCenter}};

  m_routingKeys = {QRegExp("Circuit Statistics:.*"),
                   QRegExp("Final Net Connection Criticality Histogram")};
}

QStringList RoutingReportManager::getAvailableReportIds() const {
  return {QString(CIRCUIT_REPORT_NAME), QString(RESOURCE_REPORT_NAME),
          QString(TIMING_REPORT_NAME)};
}

const ITaskReportManager::Messages &RoutingReportManager::getMessages() {
  if (!isFileParsed()) parseLogFile();
  return m_messages;
}

std::unique_ptr<ITaskReport> RoutingReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  ITaskReport::DataReports dataReports;

  if (reportId == QString(RESOURCE_REPORT_NAME))
    dataReports.push_back(std::make_unique<TableReport>(
        m_resourceColumns, m_resourceData, RESOURCE_REPORT_NAME));
  else if (reportId == QString(CIRCUIT_REPORT_NAME))
    dataReports.push_back(std::make_unique<TableReport>(
        m_circuitColumns, m_circuitData, CIRCUIT_REPORT_NAME));
  else {
    dataReports.push_back(std::make_unique<TableReport>(
        m_timingColumns, m_timingData, "Statistical timing:"));
    for (auto &hgrm : m_histograms)
      dataReports.push_back(std::make_unique<TableReport>(
          m_histogramColumns, hgrm.second, hgrm.first));
  }
  emit reportCreated(reportId);

  return std::make_unique<DefaultTaskReport>(std::move(dataReports), reportId);
}

IDataReport::TableData RoutingReportManager::parseCircuitStats(QTextStream &in,
                                                               int &lineNr) {
  auto circuitData = IDataReport::TableData{};

  auto isTotalLine = [](QString &line) -> bool {
    return !line.startsWith(
        "    ");  // child items have more space at the beginning
  };
  QStringList totalLine{};
  QString line;

  while (in.readLineInto(&line)) {
    ++lineNr;
    auto simplifiedLine = line.simplified();
    auto lineData = simplifiedLine.split(":");
    if (lineData.size() != 2)  // expected format is: "block : value";
      continue;

    if (isTotalLine(line)) {
      // We are only interested in first section(total with parents).
      // Second total line breaks the loop and ends parsing
      if (totalLine.isEmpty())
        totalLine << QString("Total") << lineData[1];
      else
        break;
    } else {
      circuitData.push_back(lineData);
    }
  }

  return circuitData;
}

void RoutingReportManager::parseLogFile() {
  auto logFile = createLogFile(QString(ROUTING_LOG));
  if (!logFile) return;

  reset();

  QTextStream in(logFile.get());

  auto timings = QStringList{};
  auto lineNr = 0;
  QString line;
  while (in.readLineInto(&line)) {
    if (FIND_RESOURCES.indexIn(line) != -1)
      parseResourceUsage(in, lineNr);
    else if (FIND_CIRCUIT_STAT.indexIn(line) != -1)
      m_circuitData = parseCircuitStats(in, lineNr);
    else if (line.startsWith(LOAD_PLACEMENT_SECTION))
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
    ++lineNr;
  }
  if (!timings.isEmpty()) fillTimingData(timings);

  logFile->close();
  setFileParsed(true);
}

QString RoutingReportManager::getTimingLogFileName() const {
  return QString(ROUTING_TIMING_LOG);
}

void RoutingReportManager::reset() {
  m_messages.clear();
  m_circuitData.clear();
  m_histograms.clear();
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
