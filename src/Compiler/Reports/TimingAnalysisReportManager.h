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
  bool isOpensta() const;
  void parseStatisticLine(const QString &line) override;
  QStringList getAvailableReportIds() const override;
  QString getReportIdByType(ReportIdType idType) const override;
  std::unique_ptr<ITaskReport> createReport(const QString &reportId) override;
  QString getTimingLogFileName() const override;
  bool isStatisticalTimingLine(const QString &line) override;
  bool isStatisticalTimingHistogram(const QString &line) override;
  void splitTimingData(const QString &timingStr) override;
  void parseLogFile() override;
  std::filesystem::path logFile() const override;
  void clean() override;
  void validateTimingReport();

  static QString ToString(double val);

  void parseOpenSTALog();
  IDataReport::TableData parseOpenSTATimingTable(QTextStream &in,
                                                 int &lineNr) const;
  IDataReport::TableData CreateTotalDesign() const;
  IDataReport::TableData CreateIntraClock() const;
  IDataReport::TableData CreateInterClock() const;

  template <class T>
  void Resize(int row, int col, QVector<QVector<T>> &vec) {
    vec.resize(row);
    for (int i = 0; i < row; i++) vec[i].resize(col);
  }

  void parseIntraSetupSection(const QString &line);
  void parseInterDomPathDelaysSection(const QString &line);
  void parseInterSetupSection(const QString &line);

  SectionKeys m_createDeviceKeys;
  IDataReport::ColumnValues m_openSTATimingColumns;

  IDataReport::ColumnValues m_circuitColumns;
  IDataReport::ColumnValues m_bramColumns;
  IDataReport::ColumnValues m_dspColumns;
  IDataReport::TableData m_circuitData;
  IDataReport::TableData m_bramData;
  IDataReport::TableData m_dspData;

  // timing report
  IDataReport::ColumnValues m_totalDesignColumn;
  IDataReport::ColumnValues m_intraClockColumn;
  IDataReport::ColumnValues m_interClockColumn;
  IDataReport::TableData m_totalDesignTable;
  IDataReport::TableData m_intraClockTable;
  IDataReport::TableData m_interClockTable;

  IDataReport::TableMetaData m_totalDesignMeta;
  IDataReport::TableMetaData m_intraClockMeta;
  IDataReport::TableMetaData m_interClockMeta;

  Compiler *m_compiler;
  TimingData m_timingSetup{};
  TimingData m_timingHold{};
  QVector<ClockData> m_clocksInter;
};

}  // namespace FOEDAG
