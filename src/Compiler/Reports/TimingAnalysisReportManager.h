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
#pragma once

#include "AbstractReportManager.h"

namespace FOEDAG {
class Compiler;

/* Report manager for timing analysis. It works with 'timing_analysis.rpt' log
 * file. As there are two timing engines supported: Tatum (Default) and OpenSTA,
 * this manager is responsible for recognizing messages of both.
 */
class TimingAnalysisReportManager final : public AbstractReportManager {
 public:
  TimingAnalysisReportManager(const TaskManager &taskManager,
                              Compiler *compiler);

 private:
  QStringList getAvailableReportIds() const override;
  std::unique_ptr<ITaskReport> createReport(const QString &reportId) override;
  QString getTimingLogFileName() const override;
  bool isStatisticalTimingLine(const QString &line) override;
  bool isStatisticalTimingHistogram(const QString &line) override;
  void splitTimingData(const QString &timingStr) override;
  void parseLogFile() override;
  std::filesystem::path logFile() const override;
  void clean() override;

  void parseOpenSTALog();
  IDataReport::TableData parseOpenSTATimingTable(QTextStream &in,
                                                 int &lineNr) const;

  SectionKeys m_createDeviceKeys;
  IDataReport::ColumnValues m_openSTATimingColumns;

  IDataReport::ColumnValues m_circuitColumns;
  IDataReport::ColumnValues m_bramColumns;
  IDataReport::ColumnValues m_dspColumns;
  IDataReport::TableData m_circuitData;
  IDataReport::TableData m_bramData;
  IDataReport::TableData m_dspData;

  Compiler *m_compiler;
};

}  // namespace FOEDAG
