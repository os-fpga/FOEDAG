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

#include "PowerAnalysisReportManager_ql.h"

#include <iostream>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
#include "TableReport.h"

namespace {
// Report strings
static constexpr const char *REPORT_NAME{"Power Analysis report"};
// static constexpr const char *MAX_LVL_STR{"Maximum logic level"};
// static constexpr const char *AVG_LVL_STR{"Average logic level"};

// // Messages regexp
// static const QRegExp VERIFIC_ERR_REGEXP{"VERIFIC-ERROR.*"};
// static const QRegExp VERIFIC_WARN_REGEXP{"VERIFIC-WARNING.*"};
// static const QRegExp VERIFIC_INFO_REGEXP{
//     "Executing synth_rs pass.*|Executing RS_DSP_MACC.*"};
}  // namespace

namespace FOEDAG {

PowerAnalysisReportManager::PowerAnalysisReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {}

QStringList PowerAnalysisReportManager::getAvailableReportIds() const {
  return {QString(REPORT_NAME)};
}


std::unique_ptr<ITaskReport> PowerAnalysisReportManager::createReport(
    const QString &reportId) {
  if (!isFileParsed()) parseLogFile();

  ITaskReport::DataReports dataReports;
  
  if(power_estimate_table) {
    dataReports.push_back(std::move(power_estimate_table));
  }
  if(power_debug_table) {
    dataReports.push_back(std::move(power_debug_table));
  }

  emit reportCreated(QString(REPORT_NAME));

  return std::make_unique<DefaultTaskReport>(std::move(dataReports),
                                             "Power Analysis report");
}

void PowerAnalysisReportManager::parseLogFile() {
  m_resourceData.clear();
  m_timingData.clear();
  m_messages.clear();

  // read power analysis rpt
  auto logFile = createLogFile(QString(POWER_ANALYSIS_LOG));
  if (!logFile) return;

  auto fileStr = QTextStream(logFile.get()).readAll();
  logFile->close();

  QRegExp dynamic_power_regex = QRegExp{"Dynamic Power\\s*=\\s*([+-]?[0-9]*\\.[0-9]*)\\s+[^\\s]+"};
  QString dynamic_power_value;
  if (dynamic_power_regex.lastIndexIn(fileStr) != -1) {
    dynamic_power_value = dynamic_power_regex.cap(1);
    // std::cout << dynamic_power_value.toStdString() << std::endl;
  }

  QRegExp leakage_power_regex = QRegExp{"Leakage Power\\s*=\\s*([+-]?[0-9]*\\.[0-9]*)\\s+[^\\s]+"};
  QString leakage_power_value;
  if (leakage_power_regex.lastIndexIn(fileStr) != -1) {
    leakage_power_value = leakage_power_regex.cap(1);
    // std::cout << leakage_power_value.toStdString() << std::endl;
  }


  QRegExp total_power_regex = QRegExp{"Total Power\\s*=\\s*([+-]?[0-9]*\\.[0-9]*)\\s+[^\\s]+"};
  QString total_power_value;
  if (total_power_regex.lastIndexIn(fileStr) != -1) {
    total_power_value = total_power_regex.cap(1);
    // std::cout << total_power_value.toStdString() << std::endl;
  }


  // create power analysis report
  IDataReport::ColumnValues power_estimate_cols;
  power_estimate_cols.push_back(ReportColumn{"Cateogory"});
  power_estimate_cols.push_back(ReportColumn{"Power (mW)", Qt::AlignCenter});

  IDataReport::TableData power_estimate_data = IDataReport::TableData{};
  power_estimate_data.push_back(QStringList{"Dynamic", dynamic_power_value});
  power_estimate_data.push_back(QStringList{"Leakage",leakage_power_value});
  power_estimate_data.push_back(QStringList{"Total",total_power_value});

  power_estimate_table = std::make_unique<TableReport>(power_estimate_cols,
                                                       power_estimate_data,
                                                       QString{"Power Estimates"});


  // read power analysis debug rpt
  auto logFile_debug = createLogFile(QString("power_analysis_debug.rpt"));
  if (!logFile_debug) return;

  auto fileStr_debug = QTextStream(logFile_debug.get()).readAll();
  logFile_debug->close();

  if(fileStr_debug.isEmpty()) return;

  // std::cout << "parsing debug report" << std::endl;

  QVector<QStringList> user_input_lines;
  // calculator_d6  : 4              [Array X]
  QRegExp user_inputs_regex("(\\S+)\\s+:\\s+(\\S+)\\s+\\[(.+)\\]");
  user_inputs_regex.setMinimal(true);
  int pos = 0;
  while ((pos = user_inputs_regex.indexIn(fileStr_debug, pos)) != -1) {
    QStringList user_input;

    // std::cout << "1: " << user_inputs_regex.cap(1).toStdString() << std::endl;
    // std::cout << "2: " << user_inputs_regex.cap(2).toStdString() << std::endl;
    // std::cout << "3: "<< user_inputs_regex.cap(3).toStdString() << std::endl;

    user_input << user_inputs_regex.cap(1);
    user_input << user_inputs_regex.cap(2);
    user_input << user_inputs_regex.cap(3);

    user_input_lines.push_back(std::move(user_input));

    pos += user_inputs_regex.matchedLength();
  }


  QRegExp user_inputs_def_regex("(\\S+)\\s+:\\s+(\\S+)\\s+(V|MHz|%)\\s+\\[(.+)\\]");
  user_inputs_def_regex.setMinimal(true);
  pos = 0;
  while ((pos = user_inputs_def_regex.indexIn(fileStr_debug, pos)) != -1) {
    QStringList user_input;

    // std::cout << "1: " << user_inputs_def_regex.cap(1).toStdString() << std::endl;
    // std::cout << "2: " << user_inputs_def_regex.cap(2).toStdString() << std::endl;
    // std::cout << "3: "<< user_inputs_def_regex.cap(3).toStdString() << std::endl;

    user_input << user_inputs_def_regex.cap(1);
    user_input << user_inputs_def_regex.cap(2) + " " + user_inputs_def_regex.cap(3);
    user_input << user_inputs_def_regex.cap(4);

    user_input_lines.push_back(std::move(user_input));

    pos += user_inputs_def_regex.matchedLength();
  }


  // create power analysis debug report
  IDataReport::ColumnValues power_debug_cols;
  power_debug_cols.push_back(ReportColumn{"Spreadsheet Cell"});
  power_debug_cols.push_back(ReportColumn{"Label", Qt::AlignCenter});
  power_debug_cols.push_back(ReportColumn{"Value"});

  IDataReport::TableData power_debug_data = IDataReport::TableData{};
  for (QStringList user_input : user_input_lines) {
    // std::cout << "data\n" << std::endl;
    // std::cout << user_input[0].toStdString() << std::endl;
    // std::cout << user_input[1].toStdString() << std::endl;
    // std::cout << user_input[2].toStdString() << std::endl;
    power_debug_data.push_back(std::move(user_input));
  }

  power_debug_table = std::make_unique<TableReport>(power_debug_cols,
                                                    power_debug_data,
                                                    QString{"Power Debug Inputs"});

  setFileParsed(true);
}

QString PowerAnalysisReportManager::getTimingLogFileName() const {
  // Current power analysis log implementation doesn't contain timing info
  return {};
}

void PowerAnalysisReportManager::splitTimingData(const QString &timingStr) {
  // Current power analysis log implementation doesn't contain timing info
}
}  // namespace FOEDAG
