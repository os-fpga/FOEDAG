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

#include <QObject>

#include "ITaskReportManager.h"
#include "TableReport.h"

class QTextStream;

namespace FOEDAG {

/* Placement-specific report manager. It triggers 'placement.rpt' log file
 * parsing and creates two reports:
 * - Report Resource Utilization, shown as table in application editor area;
 * - Report Static Timing, placed into post_place_timing.rpt file.
 */
class PlacementReportManager final : public QObject, public ITaskReportManager {
  Q_OBJECT

  std::vector<std::string> getAvailableReportIds() const override;
  std::unique_ptr<ITaskReport> createReport(
      const std::string &reportId) override;
  std::map<size_t, std::string> getMessages() override;
  // Creates a file in given projectPath and fills it with timingData
  void createTimingReport(const QString &projectPath,
                          const QStringList &timingData);

  // Parses in stream line by line till empty one occurs and creates table data
  ITaskReport::TableData parseResources(QTextStream &in,
                                        const QStringList &columns) const;

 signals:
  void reportCreated(QString reportName);
};

}  // namespace FOEDAG
