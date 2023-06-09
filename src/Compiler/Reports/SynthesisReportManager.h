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

#include <QSet>

#include "AbstractReportManager.h"

namespace FOEDAG {

/* Synthesis-specific report manager. It triggers 'synthesis.rpt' log file
 * parsing and creates a report, containing following data:
 * Number of wires
   Number of wire bits
   Number of public wires
   Number of public wire bits
   Number of memories
   Number of memory bits
   Number of processes
   Number of cells
   Average level
   Maximum level
 */
class SynthesisReportManager final : public AbstractReportManager {
 public:
  SynthesisReportManager(const TaskManager &taskManager);

 private:
  void parseLogLine(const QString &line) override;
  QStringList getAvailableReportIds() const override;
  std::unique_ptr<ITaskReport> createReport(const QString &reportId) override;
  QString getTimingLogFileName() const override;
  void splitTimingData(const QString &timingStr) override;
  std::filesystem::path logFile() const override;
  void clean() override;
  // Go through the log file and fills internal data collections (stats,
  // messages)
  void parseLogFile() override;

  // Retrieves maximum and average levels out of given line and fills into stats
  void fillLevels(const QString &line, IDataReport::TableData &stats) const;
  // Parses input stream and gets all statistics with their values
  IDataReport::TableData getStatistics(const QString &statsStr) const;

  IDataReport::ColumnValues m_circuitColumns;
  IDataReport::ColumnValues m_bramColumns;
  IDataReport::ColumnValues m_dspColumns;
  IDataReport::TableData m_circuitData;
  IDataReport::TableData m_bramData;
  IDataReport::TableData m_dspData;
};

}  // namespace FOEDAG
