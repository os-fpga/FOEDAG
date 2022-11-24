#include "RoutingReportManager.h"

#include <QFile>
#include <QTextStream>

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"

namespace {
static const QRegExp FIND_CIRTUIT_STAT{"Circuit Statistics:.*"};

static constexpr const char *BLOCK_TYPE_COL{"Block type"};
static constexpr const char *NOF_BLOCKS_COL{"Number of blocks"};

static constexpr const char *CIRCUIT_REPORT_NAME{"Cirtuit Statistics Report"};
static constexpr const char *RESOURCE_REPORT_NAME{
    "Report Resource Utilization"};
}  // namespace

namespace FOEDAG {

std::vector<std::string> RoutingReportManager::getAvailableReportIds() const {
  return {CIRCUIT_REPORT_NAME, RESOURCE_REPORT_NAME};
}

std::map<size_t, std::string> RoutingReportManager::getMessages() { return {}; }

std::unique_ptr<ITaskReport> RoutingReportManager::createReport(
    const std::string &reportId) {
  auto logFile = createLogFile(QString(ROUTING_LOG));
  if (!logFile) return nullptr;

  auto report = reportId == RESOURCE_REPORT_NAME
                    ? createResourceReport(*logFile)
                    : createCircuitReport(*logFile);

  logFile->close();

  emit reportCreated(QString::fromStdString(reportId));

  return report;
}

std::unique_ptr<ITaskReport> RoutingReportManager::createResourceReport(
    QFile &logFile) {
  auto resourcesData = ITaskReport::TableData{};

  QStringList columnNames;
  QTextStream in(&logFile);

  QString line;
  while (in.readLineInto(&line)) {
    if (FIND_RESOURCES.indexIn(line) != -1) {
      resourcesData = parseResourceUsage(in, columnNames);
      break;
    }
  }
  return std::make_unique<TableReport>(
      std::move(columnNames), std::move(resourcesData), RESOURCE_REPORT_NAME);
}

std::unique_ptr<ITaskReport> RoutingReportManager::createCircuitReport(
    QFile &logFile) {
  auto circuitData = ITaskReport::TableData{};

  auto isTotalLine = [](QString &line) -> bool {
    return !line.startsWith(
        "    ");  // child items have more space at the beginning
  };
  QStringList totalLine{};

  auto statsFound = false;

  QString line;
  QTextStream in(&logFile);
  while (in.readLineInto(&line)) {
    if (statsFound) {
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

    } else if (FIND_CIRTUIT_STAT.indexIn(line) != -1) {
      statsFound = true;
    }
  }
  circuitData.push_back(totalLine);

  auto colNames = QStringList{QString{BLOCK_TYPE_COL}, QString{NOF_BLOCKS_COL}};
  return std::make_unique<TableReport>(
      std::move(colNames), std::move(circuitData), CIRCUIT_REPORT_NAME);
}

}  // namespace FOEDAG
