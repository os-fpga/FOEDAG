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

#include "SynthesisReportManager.h"

#include <QFile>
#include <QTextStream>

#include "CompilerDefines.h"
#include "NewProject/ProjectManager/project.h"
#include "TableReport.h"
#include "Utils/FileUtils.h"

namespace {
static constexpr const char *STAT_STR{"Printing statistics"};
static constexpr const char *DE_STR{"DE:"};
static constexpr const char *MAX_LVL_STR{"Maximum logic level"};
static constexpr const char *AVG_LVL_STR{"Average logic level"};
}  // namespace

namespace FOEDAG {

std::vector<std::string> SynthesisReportManager::getAvailableReportIds() const {
  return {"Synthesis report"};
}

SynthesisReportManager::LinesData SynthesisReportManager::getStatistics(
    QTextStream &in) const {
  auto res = LinesData{};

  auto line = QString{};        // Unmodified line from the log file
  auto dataLine = QString{};    // Simplified line
  auto parentItem = QString{};  // Parent item for lines starting with tab

  in.readLineInto(&line);  // skip empty line
  in.readLineInto(&line);  // skip top level module
  in.readLineInto(&line);  // skip empty line

  while (in.readLineInto(&line)) {
    if (line.startsWith("     ") && parentItem.isEmpty())
      parentItem = dataLine.left(dataLine.indexOf(':'));

    dataLine = line.simplified();
    if (dataLine.isEmpty()) break;

    auto reportLine = std::vector<std::string>{};
    auto lineStrs = dataLine.contains(":") ? dataLine.split(":")
                                           : dataLine.split(QChar::Space);

    for (auto i = 0; i < lineStrs.size(); ++i) {
      auto lineStr = lineStrs[i];
      // The first string represents statistic name. Parent item should only be
      // added there.
      if (!i && !parentItem.isEmpty())
        lineStr = QString("%1:%2").arg(parentItem).arg(lineStr);
      reportLine.push_back(lineStr.toStdString());
    }

    res.push_back(std::move(reportLine));
  }

  return res;
}

SynthesisReportManager::LinesData SynthesisReportManager::getLevels(
    const QString &line) const {
  auto res = LinesData{};

  auto lineData = line.split(" ");
  // #TODO: Replace it with RegExp. We expect 21 strings to be in DE: line.
  // Otherwise under 10 and 14 we may get totally different numbers.
  if (lineData.size() != 21) return res;
  res.push_back({MAX_LVL_STR, lineData[10].toStdString()});
  res.push_back({AVG_LVL_STR, lineData[14].toStdString()});

  return res;
}

std::unique_ptr<ITaskReport> SynthesisReportManager::createReport(
    const std::string &reportId) {
  auto logPath =
      std::filesystem::path(Project::Instance()->projectPath().toStdString()) /
      std::string(SYNTHESIS_LOG);

  if (!FileUtils::FileExists(logPath)) return nullptr;

  auto pathString = QString::fromStdString(logPath.string());
  auto logFile = QFile(pathString);
  if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) return nullptr;

  // To save the last report statistics
  auto stats = LinesData{};
  auto levels = LinesData{};

  QTextStream in(&logFile);
  QString line;
  while (in.readLineInto(&line)) {
    auto dataLine = line.simplified();
    if (dataLine.isEmpty()) continue;
    if (dataLine.contains(QString(STAT_STR)))
      stats = getStatistics(in);
    else if (dataLine.startsWith(QString(DE_STR)))
      levels = getLevels(dataLine);
  }
  logFile.close();

  auto result = LinesData{};
  std::merge(levels.begin(), levels.end(), stats.begin(), stats.end(),
             std::back_inserter(result));

  auto columnNames = std::vector<std::string>{"Statistics", "Value"};
  return std::make_unique<TableReport>(std::move(columnNames),
                                       std::move(result), "Synthesis report");
}

std::map<size_t, std::string> SynthesisReportManager::getMessages() {
  return {};
}

}  // namespace FOEDAG
