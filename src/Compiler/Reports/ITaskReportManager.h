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

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace FOEDAG {

class ITaskReport;

/* Manager for task reports. It has to be implemented per compilation
 * task, as reports are task-specific. It knows how many reports are
 * available per task and can create reports.
 */
class ITaskReportManager {
 public:
  // Returns all available reports per compilation task
  virtual std::vector<std::string> getAvailableReportIds() const = 0;
  // Creates the report. This call will most likely result in log file parsing
  // and potentially caching, so it's not const.
  virtual std::unique_ptr<ITaskReport> createReport(
      const std::string &reportId) = 0;
  // Returns retrieved from a log file messages per line number.
  virtual std::map<size_t, std::string> getMessages() = 0;
};

}  // namespace FOEDAG
