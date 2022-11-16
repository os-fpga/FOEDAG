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

#include "ITaskReport.h"

namespace FOEDAG {

// Default report implementation, representing table-like data.
class TableReport : public ITaskReport {
 public:
  TableReport(LineValues &&columnNames, std::vector<LineValues> &&linesData,
              const std::string &name);
  virtual ~TableReport() = default;

 private:
  const LineValues &getColumns() const override;
  // Returns report data - rows of values
  const std::vector<LineValues> &getData() const override;
  // Returns report name
  const std::string &getName() const override;

  LineValues m_columnNames;
  std::vector<LineValues> m_linesData;
  std::string m_name;
};

}  // namespace FOEDAG
