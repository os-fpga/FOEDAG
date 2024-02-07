#include "RoutingReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"
#include "QLMetricsManager.h"

#define USE_QTREEVIEW (1)

namespace {
static const QRegExp FIND_INIT_ROUTER{"Initializing router criticalities"};
static const QRegExp FIND_NET_CONNECTION{
    "Final Net Connection Criticality Histogram"};
static const QRegExp FIND_ROUTING_TIMING{"Final.*(Slack|MHz).*"};
static const QRegExp ROUTING_SUMMARY{"Circuit successfully routed.*"};
static const QRegExp TIMING_INFO{"Final hold Worst Negative Slack.*"};

static constexpr const char *CIRCUIT_REPORT_NAME{
    "Post routing - Circuit Statistics Report"};
static constexpr const char *RESOURCE_REPORT_NAME{
    "Post routing - Report Resource Utilization"};
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

std::unique_ptr<ITaskReport> RoutingReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  ITaskReport::DataReports dataReports;

  if (reportId == QString(RESOURCE_REPORT_NAME)) {
    dataReports.push_back(std::make_unique<TableReport>(
        m_resourceColumns, m_resourceData, QString{"Resource Utilization"}));
    dataReports.push_back(std::make_unique<TableReport>(
        m_detailedUtilizationColumns, m_detailedUtilizationData, QString{"Detailed Resource Utilization"}));
  }
  else if (reportId == QString(CIRCUIT_REPORT_NAME))
    dataReports.push_back(std::make_unique<TableReport>(
        m_circuitColumns, m_circuitData, QString{"Circuit Statistics"}));
  else {
    dataReports.push_back(std::make_unique<TableReport>(
        m_timingColumns, m_timingData, "Static Timing"));
    for (auto &hgrm : m_histograms)
      dataReports.push_back(std::make_unique<TableReport>(
          m_histogramColumns, hgrm.second, hgrm.first));
  }
  emit reportCreated(reportId);

  return std::make_unique<DefaultTaskReport>(std::move(dataReports), reportId);
}

void RoutingReportManager::parseLogFile() {
  reset();

  auto logFile = createLogFile(QString(ROUTING_LOG));
  if (!logFile) return;

  QTextStream in(logFile.get());
  if (in.atEnd()) return;
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


#if USE_QTREEVIEW
  // push placeholder column and data row to ensure we are not reported empty.
  // the actual treeview is populated inside the Tasks_ql.cpp file directly.
  // To enable all data reports to be flexible, we should be using QStandardItemModel (or QAbstractItemModel)
  // based data model, and render according to the type of report, maybe table, tree, list etc.
  m_detailedUtilizationColumns.push_back(ReportColumn{""});
  m_detailedUtilizationData.push_back(QStringList{""});
#else // #if USE_QTREEVIEW

  // create report using detailed utilization data:
  // column headers : blank, as this is a hierarchy-based report
  m_detailedUtilizationColumns.push_back(ReportColumn{""});
  m_detailedUtilizationColumns.push_back(ReportColumn{""});
  m_detailedUtilizationColumns.push_back(ReportColumn{""});

  AuroraUtilization& util_p = QLMetricsManager::getInstance()->aurora_routing_utilization;
  m_detailedUtilizationData.push_back(std::move(QStringList{
          QString::number(util_p.clb) + " CLB used",
          "",
          ""}));
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "which contains " + QString::number(util_p.fle) + " FLE",
          "",
          ""}));
  m_detailedUtilizationData.push_back(std::move(QStringList{
          QString::number(util_p.clb_fle) + " FLE used",
          "",
          ""}));

  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.clb_lut) + " LUT used",
          ""}));
  if(util_p.clb_lut_lut6 > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_lut_lut6) + " as LUT6"}));
  if(util_p.clb_lut_lut6ff > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_lut_lut6ff) + " as LUT6+FF"}));
  if(util_p.clb_lut_lut5 > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_lut_lut5) + " as LUT5"}));
  if(util_p.clb_lut_lut5ff > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_lut_lut5ff) + " as LUT5+FF"}));
  if(util_p.clb_lut_lut4 > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_lut_lut4) + " as LUT4 (adder carry chain)"}));

  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.clb_ff) + " FF used",
          ""}));
  if(util_p.clb_ff_lut6ff > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_ff_lut6ff) + " as LUT6+FF"}));
  if(util_p.clb_ff_lut5ff > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_ff_lut5ff) + " as LUT5+FF"}));
  if(util_p.clb_ff_ff > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_ff_ff) + " as FF"}));
  if(util_p.clb_ff_shiftreg > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::number(util_p.clb_ff_shiftreg) + " as shift register"}));

  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          ""}));
  m_detailedUtilizationData.push_back(std::move(QStringList{
          QString::number(util_p.bram) + " BRAM used",
          "",
          ""}));
  if(util_p.bram_bram_nonsplit > 0) {
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.bram_bram_nonsplit) + " in nonsplit BRAM mode",
          ""}));
  for (const auto & [ key, value ] : util_p.bram_bram_nonsplit_map) {
    m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::fromStdString(key).trimmed() + " : " + QString::number(value)}));
  }
  }
  if(util_p.bram_bram_split) {
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.bram_bram_split) + " in split BRAM mode",
          ""}));
  for (const auto & [ key, value ] : util_p.bram_bram_split_map) {
    m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::fromStdString(key).trimmed() + " : " + QString::number(value)}));
  }
  }
  if(util_p.bram_fifo_nonsplit > 0) {
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.bram_fifo_nonsplit) + " in nonsplit FIFO mode",
          ""}));
  for (const auto & [ key, value ] : util_p.bram_fifo_nonsplit_map) {
    m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::fromStdString(key).trimmed() + " : " + QString::number(value)}));
  }
  }
  if(util_p.bram_fifo_split > 0) {
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.bram_fifo_split) + " in split FIFO mode",
          ""}));
  for (const auto & [ key, value ] : util_p.bram_fifo_split_map) {
    m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          QString::fromStdString(key).trimmed() + " : " + QString::number(value)}));
  }
  }

  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          ""}));
  m_detailedUtilizationData.push_back(std::move(QStringList{
          QString::number(util_p.dsp) + " DSP used",
          "",
          ""}));
  for (const auto & [ key, value ] : util_p.dsp_map) {
    m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::fromStdString(key).trimmed() + " : " + QString::number(value),
          ""}));
  }

  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          "",
          ""}));
  m_detailedUtilizationData.push_back(std::move(QStringList{
          QString::number(util_p.io) + " IO used",
          "",
          ""}));
  if(util_p.io_input > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.io_input) + " as input",
          ""}));
  if(util_p.io_output > 0)
  m_detailedUtilizationData.push_back(std::move(QStringList{
          "",
          QString::number(util_p.io_output) + " as output",
          ""}));
#endif // #if USE_QTREEVIEW

  setFileParsed(true);
}

QString RoutingReportManager::getTimingLogFileName() const {
  return QString(ROUTING_TIMING_LOG);
}

void RoutingReportManager::reset() {
  m_messages.clear();
  m_circuitData.clear();
  m_histograms.clear();
  m_resourceData.clear();
  m_timingData.clear();
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
