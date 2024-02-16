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

#include "Compiler.h"
#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "TableReport.h"
#include "Utils/FileUtils.h"

namespace {
// Report strings
static constexpr const char *MAX_LVL_STR{"Maximum logic level"};
static constexpr const char *AVG_LVL_STR{"Average logic level"};

static constexpr const char *RESOURCE_REPORT_NAME{
    "Synthesis - Utilization report"};
static constexpr const char *DESIGN_STAT_REPORT_NAME{
    "Synthesis - Design statistics"};

// Messages regexp
static const QRegularExpression VERIFIC_ERR_REGEXP{"VERIFIC-ERROR.*"};
static const QRegularExpression VERIFIC_WARN_REGEXP{"VERIFIC-WARNING.*"};
static const QRegularExpression VERIFIC_INFO_REGEXP{
    "Executing synth_rs pass.*|Executing RS_DSP_MACC.*"};
static const QString STATISTIC_SECTION{"Number of wires"};
}  // namespace

namespace FOEDAG {

SynthesisReportManager::SynthesisReportManager(const TaskManager &taskManager)
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

void SynthesisReportManager::parseLogLine(const QString &line) {
  AbstractReportManager::parseLogLine(line);
  static const QRegularExpression lut{"\\$lut +(\\d+)",
                                      QRegularExpression::MultilineOption};
  auto lutMatch = lut.match(line);
  if (lutMatch.hasMatch()) {
    m_usedRes.logic.lut5 = m_usedRes.logic.lut6 = 0;
    m_usedRes.logic.lut5 = lutMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression bram36k{"^ +TDP_RAM36K\\D+(\\d+)",
                                          QRegularExpression::MultilineOption};
  auto bram36kMatch = bram36k.match(line);
  if (bram36kMatch.hasMatch()) {
    m_usedRes.bram.bram_36k = bram36kMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression bram18k{"^ +TDP_RAM18KX2\\D+(\\d+)",
                                          QRegularExpression::MultilineOption};
  auto bram18kMatch = bram18k.match(line);
  if (bram18kMatch.hasMatch()) {
    m_usedRes.bram.bram_18k = bram18kMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression dsp_18_20{
      "^ +DSP38\\D+(\\d+)", QRegularExpression::MultilineOption};
  auto dsp_18_20Match = dsp_18_20.match(line);
  if (dsp_18_20Match.hasMatch()) {
    m_usedRes.dsp.dsp_18_20 += dsp_18_20Match.captured(1).toUInt();
    return;
  }
  static const QRegularExpression dsp_9_10{"^ +DSP19X2\\D+(\\d+)",
                                           QRegularExpression::MultilineOption};
  auto dsp_9_10Match = dsp_9_10.match(line);
  if (dsp_9_10Match.hasMatch()) {
    m_usedRes.dsp.dsp_9_10 = dsp_9_10Match.captured(1).toUInt();
    return;
  }
}

QStringList SynthesisReportManager::getAvailableReportIds() const {
  return {QString(RESOURCE_REPORT_NAME), QString(DESIGN_STAT_REPORT_NAME)};
}

IDataReport::TableData SynthesisReportManager::getStatistics(
    const QString &statsStr) const {
  auto res = IDataReport::TableData{};

  auto line = QString{};        // Unmodified line from the log file
  auto dataLine = QString{};    // Simplified line
  auto parentItem = QString{};  // Parent item for lines starting with tab

  auto statTable = QRegularExpression(
      "Number.*");  // Drop the beginning and start with stats
  auto match = statTable.match(statsStr);
  if (!match.hasMatch()) return res;

  QTextStream in(match.captured().toLatin1());

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
                                        IDataReport::TableData &stats) const {
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

void SynthesisReportManager::parseLogFile() {
  clean();
  auto logFile = createLogFile();
  if (!logFile) return;

  auto fileStr = QTextStream(logFile.get()).readAll();
  logFile->close();

  auto line = QString{};
  QTextStream in(fileStr.toLatin1());
  auto lineNr = 0;
  AbstractReportManager::MessagesLines warnings, errors;
  auto fillErrorsWarnings = [&warnings, &errors, this]() {
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
  };

  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (auto match = VERIFIC_INFO_REGEXP.match(line); match.hasMatch()) {
      m_messages.insert(lineNr, TaskMessage{lineNr,
                                            MessageSeverity::INFO_MESSAGE,
                                            match.captured().simplified(),
                                            {}});
      fillErrorsWarnings();
    } else if (auto match = VERIFIC_ERR_REGEXP.match(line); match.hasMatch()) {
      errors.emplace(lineNr, match.captured().simplified());
      if (!warnings.empty()) {
        auto warningsItem =
            createWarningErrorItem(MessageSeverity::WARNING_MESSAGE, warnings);
        m_messages.insert(warningsItem.m_lineNr, warningsItem);
      }
    } else if (auto match = VERIFIC_WARN_REGEXP.match(line); match.hasMatch()) {
      warnings.emplace(lineNr, match.captured().simplified());

      if (!errors.empty()) {
        auto errorsItem =
            createWarningErrorItem(MessageSeverity::ERROR_MESSAGE, errors);
        m_messages.insert(errorsItem.m_lineNr, errorsItem);
      }
    } else if (line.contains(STATISTIC_SECTION)) {
      m_usedRes.dsp = DSP{};
      m_usedRes.bram = Bram{};
      m_usedRes.logic.dff = 0;
      lineNr = parseStatisticsSection(in, lineNr);
    } else if (line.startsWith(INTRA_DOMAIN_PATH_DELAYS_SECTION)) {
      lineNr = parseSection(in, lineNr, [this](const QString &line) {
        parseIntraDomPathDelaysSection(line);
      });
    }
    ++lineNr;
  }
  m_circuitData = CreateLogicData(false);
  m_bramData = CreateBramData();
  m_dspData = CreateDspData();
  m_ioData = CreateIOData();
  m_clockData = CreateClockData();
  designStatistics();

  fillErrorsWarnings();

  setFileTimeStamp(this->logFile());
  emit logFileParsed();
}

bool SynthesisReportManager::supportBram18k() const { return true; }

bool SynthesisReportManager::supportDsp9x10() const { return true; }

QString SynthesisReportManager::getTimingLogFileName() const {
  // Current synthesis log implementation doesn't contain timing info
  return {};
}

void SynthesisReportManager::splitTimingData(const QString &timingStr) {
  // Current synthesis log implementation doesn't contain timing info
}

std::filesystem::path SynthesisReportManager::logFile() const {
  if (m_compiler)
    return m_compiler->FilePath(Compiler::Action::Synthesis, SYNTHESIS_LOG);
  return logFilePath(SYNTHESIS_LOG);
}

void SynthesisReportManager::clean() {
  AbstractReportManager::clean();
  m_resourceData.clear();
  m_messages.clear();
  m_circuitData.clear();
  m_bramData.clear();
  m_dspData.clear();
  m_ioData.clear();
  m_clockData.clear();
}

}  // namespace FOEDAG
