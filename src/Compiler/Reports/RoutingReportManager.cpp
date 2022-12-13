#include "RoutingReportManager.h"

#include <QFile>
#include <QTextStream>

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"

namespace {
static const QRegExp FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};
static const QRegExp FIND_INIT_ROUTER{"Initializing router criticalities"};
static const QRegExp FIND_NET_CONNECTION{
    "Final Net Connection Criticality Histogram"};
static const QRegExp ROUTING_SUMMARY{"Circuit successfully routed.*"};
static const QRegExp TIMING_INFO{"Final hold Worst Negative Slack.*"};

static constexpr const char *BLOCK_TYPE_COL{"Block type"};
static constexpr const char *NOF_BLOCKS_COL{"Number of blocks"};

static constexpr const char *CIRCUIT_REPORT_NAME{"Circuit Statistics Report"};
static constexpr const char *RESOURCE_REPORT_NAME{
    "Post routing - Report Resource Utilization "};

static const QString LOAD_PLACEMENT_SECTION{"# Load Placement"};
static const QString COMPUT_ROUTER_SECTION{"# Computing router lookahead map"};
static const QString ROUTING_SECTION{"# Routing"};
}  // namespace

namespace FOEDAG {

RoutingReportManager::RoutingReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {
  m_circuitColumns =
      QStringList{QString{BLOCK_TYPE_COL}, QString{NOF_BLOCKS_COL}};
  m_routingKeys = {FIND_INIT_ROUTER, FIND_NET_CONNECTION};
}

QStringList RoutingReportManager::getAvailableReportIds() const {
  return {QString(CIRCUIT_REPORT_NAME), QString(RESOURCE_REPORT_NAME)};
}

const ITaskReportManager::Messages &RoutingReportManager::getMessages() {
  if (!isFileParsed()) parseLogFile();
  return m_messages;
}

std::unique_ptr<ITaskReport> RoutingReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  auto report = std::unique_ptr<ITaskReport>{};

  if (reportId == QString(RESOURCE_REPORT_NAME))
    report = std::make_unique<TableReport>(m_resourceColumns, m_resourceData,
                                           RESOURCE_REPORT_NAME);
  else
    report = std::make_unique<TableReport>(m_circuitColumns, m_circuitData,
                                           CIRCUIT_REPORT_NAME);

  emit reportCreated(reportId);

  return report;
}

ITaskReport::TableData RoutingReportManager::parseCircuitStats(QTextStream &in,
                                                               int &lineNr) {
  auto circuitData = ITaskReport::TableData{};

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

  logFile->close();
  setFileParsed(true);
}

void RoutingReportManager::reset() {
  m_messages.clear();
  m_circuitData.clear();
}

}  // namespace FOEDAG
