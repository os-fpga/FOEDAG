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

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "Utils/FileUtils.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace {
static constexpr const char *REPORT_NAME{"Report Resource Utilization"};
static constexpr const char *RESOURCES_SPLIT{"blocks of type:"};

static constexpr const char *BLOCKS_COL{"Blocks"};
static constexpr const char *NETLIST_COL{"Netlist"};
static constexpr const char *ARCHITECTURE_COL{"Architecture"};

static const QRegExp FIND_TIMINGS{
    "Placement estimated (critical|setup).*[0-9].*"};
static const QRegExp FIND_RESOURCES{"Resource usage.*"};
}  // namespace

namespace FOEDAG {

std::vector<std::string> PlacementReportManager::getAvailableReportIds() const {
  return {std::string(REPORT_NAME)};
}

std::unique_ptr<ITaskReport> PlacementReportManager::createReport(
    const std::string &reportId) {
  auto projectPath = Project::Instance()->projectPath().toStdString();
  auto logPath =
      std::filesystem::path(projectPath) / std::string(PLACEMENT_LOG);

  if (!FileUtils::FileExists(logPath)) return nullptr;

  auto logFile = QFile(QString::fromStdString(logPath.string()));
  if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) return nullptr;

  auto columnNames =
      std::vector<std::string>{BLOCKS_COL, NETLIST_COL, ARCHITECTURE_COL};

  auto timings = QStringList{};
  auto resourcesData = std::vector<ITaskReport::LineValues>{};

  auto in = QTextStream(&logFile);
  QString line;
  while (in.readLineInto(&line)) {
    if (FIND_TIMINGS.indexIn(line) != -1)
      timings << line + "\n";
    else if (FIND_RESOURCES.indexIn(line) != -1)
      resourcesData = parseResources(in, columnNames);
  }
  logFile.close();

  createTimingReport(projectPath, timings);

  emit reportCreated(QString(REPORT_NAME));

  return std::make_unique<TableReport>(
      std::move(columnNames), std::move(resourcesData), REPORT_NAME);
}

std::map<size_t, std::string> PlacementReportManager::getMessages() {
  return {};
}

void PlacementReportManager::createTimingReport(const std::string &projectPath,
                                                const QStringList &timingData) {
  auto timingLogPath =
      std::filesystem::path(projectPath) / std::string(PLACEMENT_TIMING_LOG);

  auto timingLogFile = QFile(QString::fromStdString(timingLogPath.string()));

  if (!timingLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;
  timingLogFile.resize(0);  // clear all previous contents

  QTextStream out(&timingLogFile);
  for (auto &line : timingData) out << line;

  timingLogFile.close();
}

std::vector<ITaskReport::LineValues> PlacementReportManager::parseResources(
    QTextStream &in, const std::vector<std::string> &columns) const {
  int childSum{0};     // Total number, assumulated within a single parent
  int columnIndex{0};  // Index of a column we fill the value for

  auto result = std::vector<ITaskReport::LineValues>{};

  // Lambda setting given value for a certain row. Modifies existing row, if
  // any, or adds a new one.
  auto setValue = [&](const std::string &row, const std::string &value) {
    auto findIt = std::find_if(
        result.begin(), result.end(),
        [&row](const auto &lineValues) { return lineValues[0] == row; });
    if (findIt == result.end()) {
      auto lineValues = ITaskReport::LineValues{row, {}, {}};
      lineValues[columnIndex] = value;
      result.push_back(std::move(lineValues));
    } else {
      // The resource has been added before - just update the corresponding
      // column
      (*findIt)[columnIndex] = value;
    }
  };

  QString lineStr, columnName;
  std::string resourceName, resourceValue;

  while (in.readLineInto(&lineStr)) {
    auto line = lineStr.simplified();
    if (line.isEmpty()) break;

    auto lineStrs = line.split(QString(RESOURCES_SPLIT), Qt::SkipEmptyParts);
    // Column values are expected in a following format: Value RESOURCES_SPLIT
    // ResourceName
    if (lineStrs.size() == 2) {
      columnIndex = std::find(columns.cbegin(), columns.cend(),
                              columnName.toStdString()) -
                    columns.cbegin();

      resourceName = lineStrs[1].toStdString();

      setValue(resourceName, lineStrs[0].toStdString());
      bool ok;
      childSum += lineStrs[0].toInt(&ok);
    } else {
      auto resourceNameSplit = QString::fromStdString(resourceName).split("_");
      // in case resource name is of 'parent_resource' format, get parent name
      // and set accumulated number to it
      if (resourceNameSplit.size() > 1)
        setValue(resourceNameSplit.first().toStdString(),
                 QString::number(childSum).toStdString());
      columnName = line;
      childSum = 0;  // New parent - reset the SUM
    }
  }

  return result;
}

}  // namespace FOEDAG
