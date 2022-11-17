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

#include <string>
#include <vector>

namespace FOEDAG {

/* Given interface represents a report for the compilation task.
 * It returns row and column data as strings. For simplicity, only
 * standard table data is supported, no parent items. It can be
 * extended if needed.
 */
class ITaskReport {
 public:
  virtual ~ITaskReport() = default;

  using LineValues = std::vector<std::string>;
  // Returns report column names
  virtual const LineValues &getColumns() const = 0;
  // Returns report data - rows of values
  virtual const std::vector<LineValues> &getData() const = 0;
  // Returns report name
  virtual const std::string &getName() const = 0;
};
}  // namespace FOEDAG
