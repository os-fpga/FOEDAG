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
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "TableReport.h"

namespace {
// Report strings
static constexpr const char *REPORT_NAME{"Synthesis report"};
static constexpr const char *MAX_LVL_STR{"Maximum logic level"};
static constexpr const char *AVG_LVL_STR{"Average logic level"};

// Messages regexp
static const QRegExp VERIFIC_ERR_REGEXP{"VERIFIC-ERROR.*"};
static const QRegExp VERIFIC_WARN_REGEXP{"VERIFIC-WARNING.*"};
static const QRegExp VERIFIC_INFO_REGEXP{
    "Executing synth_rs pass.*|Executing RS_DSP_MACC.*"};
}  // namespace

namespace FOEDAG {

SynthesisReportManager::SynthesisReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {}

QStringList SynthesisReportManager::getAvailableReportIds() const {
  return {QString(REPORT_NAME)};
}

ITaskReport::TableData SynthesisReportManager::getStatistics(
    const QString &statsStr) const {
  auto res = ITaskReport::TableData{};

  auto line = QString{};        // Unmodified line from the log file
  auto dataLine = QString{};    // Simplified line
  auto parentItem = QString{};  // Parent item for lines starting with tab

  auto statTable =
      QRegExp("Number.*");  // Drop the beginning and start with stats
  if (statTable.indexIn(statsStr) == -1) return res;

  QTextStream in(statTable.cap().toLatin1());

  while (in.readLineInto(&line)) {
    if (line.startsWith("     ") && parentItem.isEmpty())
      parentItem = dataLine.left(dataLine.indexOf(':'));

    dataLine = line.simplified();
    if (dataLine.isEmpty()) break;

    auto reportLine = QStringList{};
    auto lineStrs = dataLine.contains(":") ? dataLine.split(":")
                                           : dataLine.split(QChar::Space);

    for (auto i = 0; i < lineStrs.size(); ++i) {
      auto lineStr = lineStrs[i];
      // The first string represents statistic name. Parent item should only be
      // added there.
      if (!i && !parentItem.isEmpty())
        lineStr = QString("%1:%2").arg(parentItem, lineStr);
      reportLine << lineStr;
    }

    res.push_back(std::move(reportLine));
  }

  return res;
}

void SynthesisReportManager::fillLevels(const QString &line,
                                        ITaskReport::TableData &stats) const {
  const QRegularExpression findLvls{
      "^DE:.*Max Lvl =\\s*(([0-9]*[.])?[0-9]+)\\s*Avg Lvl "
      "=\\s*(([0-9]*[.])?[0-9]+)"};

  auto match = findLvls.match(line);
  if (match.hasMatch()) {
    stats.push_back({MAX_LVL_STR, match.captured(1)});
    stats.push_back({AVG_LVL_STR, match.captured(3)});
  }
}

std::unique_ptr<ITaskReport> SynthesisReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  emit reportCreated(QString(REPORT_NAME));

  return std::make_unique<TableReport>(QStringList{"Statistics", "Value"},
                                       m_resourceData, "Synthesis report");
}

void SynthesisReportManager::parseLogFile() {
  auto logFile = createLogFile(QString(SYNTHESIS_LOG));
  if (!logFile) return;

  m_resourceData.clear();
  m_messages.clear();

  auto fileStr = QTextStream(logFile.get()).readAll();
  logFile->close();

  auto findStats = QRegExp("Printing statistics.*\n\n===.*===\n\n.*[^\n{2}]+");

  if (findStats.lastIndexIn(fileStr) != -1)
    m_resourceData = getStatistics(findStats.cap());

  auto findLvls = QRegExp{"DE:([^\n]+)"};
  if (findLvls.lastIndexIn(fileStr) != -1) {
    fillLevels(findLvls.cap(), m_resourceData);
  }

  auto line = QString{};
  QTextStream in(fileStr.toLatin1());
  auto lineNr = 0;
  AbstractReportManager::MessagesLines warnings, errors;
  while (in.readLineInto(&line)) {
    if (VERIFIC_INFO_REGEXP.indexIn(line) != -1)
      m_messages.insert(lineNr,
                        TaskMessage{lineNr,
                                    MessageSeverity::INFO_MESSAGE,
                                    VERIFIC_INFO_REGEXP.cap().simplified(),
                                    {}});
    else if (VERIFIC_ERR_REGEXP.indexIn(line) != -1)
      errors.emplace(lineNr, VERIFIC_ERR_REGEXP.cap().simplified());
    else if (VERIFIC_WARN_REGEXP.indexIn(line) != -1)
      warnings.emplace(lineNr, VERIFIC_WARN_REGEXP.cap().simplified());
    ++lineNr;
  }
  if (!warnings.empty()) {
    auto warningsItem =
        createWarningErrorItem(MessageSeverity::WARNING_MESSAGE, warnings);
    m_messages.insert(warningsItem.m_lineNr, warningsItem);
  }
  if (!errors.empty()) {
    auto errorsItem =
        createWarningErrorItem(MessageSeverity::ERROR_MESSAGE, errors);
    m_messages.insert(errorsItem.m_lineNr, errorsItem);
  }
  setFileParsed(true);
}

const ITaskReportManager::Messages &SynthesisReportManager::getMessages() {
  if (!isFileParsed()) parseLogFile();

  return m_messages;
}

}  // namespace FOEDAG
