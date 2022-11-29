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
  std::vector<std::string> getAvailableReportIds() const override;
  std::unique_ptr<ITaskReport> createReport(
      const std::string &reportId) override;
  std::map<size_t, std::string> getMessages() override;

  // Retrieves maximum and average levels out of given line and fills into stats
  void fillLevels(const QString &line, ITaskReport::TableData &stats) const;
  // Parses input stream and gets all statistics with their values
  ITaskReport::TableData getStatistics(const QString &statsStr) const;
};

}  // namespace FOEDAG
