#include "PackingReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "TableReport.h"
#include "Utils/FileUtils.h"

namespace {
// report names
static constexpr const char *RESOURCE_REPORT_NAME{
    "Packing - Utilization report"};
static constexpr const char *DESIGN_STAT_REPORT_NAME{
    "Packing - Design statistics"};

// Messages
static const QRegExp VPR_ROUTING_OPT{
    "VPR was run with the following options.*"};
static const QString BLOCK_GRAPH_BUILD_SECTION{
    "# Building complex block graph"};
static const QString LOAD_CIRCUIT_SECTION{"# Load circuit"};
static const QString LOAD_ARCH_SECTION{"# Loading Architecture Description"};
static const QString BUILD_TIM_GRAPH{"Build Timing Graph"};
static const QString LOAD_TIM_CONSTR{"# Load Timing Constraints"};
static const QString PACKING_SECTION{"# Packing"};
static const QString STATISTIC_SECTION{"Pb types usage..."};
}  // namespace

namespace FOEDAG {

PackingReportManager::PackingReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {
  m_circuitColumns = {ReportColumn{"Logic"},
                      ReportColumn{"Used", Qt::AlignCenter},
                      ReportColumn{"Available", Qt::AlignCenter},
                      ReportColumn{"%", Qt::AlignCenter}};
  m_bramColumns = m_circuitColumns;
  m_bramColumns[0].m_name = "Block RAM";
  m_dspColumns = m_circuitColumns;
  m_dspColumns[0].m_name = "DSP";
}

QStringList PackingReportManager::getAvailableReportIds() const {
  return {RESOURCE_REPORT_NAME, DESIGN_STAT_REPORT_NAME};
}

std::unique_ptr<ITaskReport> PackingReportManager::createReport(
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

QString PackingReportManager::getTimingLogFileName() const {
  // Packing has no timing information so far
  return QString{};
}

bool PackingReportManager::isStatisticalTimingLine(const QString &line) {
  // Packing has no timing information so far
  return false;
}

bool PackingReportManager::isStatisticalTimingHistogram(const QString &line) {
  // Packing log has no histograms
  return false;
}

void PackingReportManager::splitTimingData(const QString &timingStr) {
  // Packing has no timing information so far
}

void PackingReportManager::parseLogFile() {
  clean();
  auto logFile = createLogFile();
  if (!logFile) return;

  auto in = QTextStream(logFile.get());
  if (in.atEnd()) return;

  QString line;
  auto lineNr = 0;
  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (line.startsWith(LOAD_ARCH_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_ARCH_SECTION, {});
    else if (VPR_ROUTING_OPT.indexIn(line) != -1)
      m_messages.insert(lineNr, TaskMessage{lineNr,
                                            MessageSeverity::INFO_MESSAGE,
                                            VPR_ROUTING_OPT.cap(),
                                            {}});
    else if (line.startsWith(BLOCK_GRAPH_BUILD_SECTION))
      lineNr =
          parseErrorWarningSection(in, lineNr, BLOCK_GRAPH_BUILD_SECTION, {});
    else if (line.startsWith(LOAD_CIRCUIT_SECTION))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_CIRCUIT_SECTION, {});
    else if (line.endsWith(BUILD_TIM_GRAPH))
      m_messages.insert(
          lineNr,
          TaskMessage{
              lineNr, MessageSeverity::INFO_MESSAGE, BUILD_TIM_GRAPH, {}});
    else if (line.startsWith(LOAD_TIM_CONSTR))
      lineNr = parseErrorWarningSection(in, lineNr, LOAD_TIM_CONSTR, {});
    else if (line.startsWith(STATISTIC_SECTION))
      lineNr = parseStatisticsSection(in, lineNr);
    else if (line.startsWith(PACKING_SECTION))
      lineNr =
          parseErrorWarningSection(in, lineNr, PACKING_SECTION,
                                   {QRegExp("Final Clustering Statistics")});

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

std::filesystem::path PackingReportManager::logFile() const {
  return logFilePath(PACKING_LOG);
}

void PackingReportManager::clean() {
  AbstractReportManager::clean();
  m_messages.clear();
  m_resourceData.clear();
  m_circuitData.clear();
  m_bramData.clear();
  m_dspData.clear();
  m_ioData.clear();
  m_clockData.clear();
}

}  // namespace FOEDAG
