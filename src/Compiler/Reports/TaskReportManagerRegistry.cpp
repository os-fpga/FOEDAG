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

#include "TaskReportManagerRegistry.h"

namespace FOEDAG {

bool TaskReportManagerRegistry::registerReportManager(uint type,
                                                      ReportManager manager) {
  return m_managers.emplace(type, std::move(manager)).second;
}

ITaskReportManager *TaskReportManagerRegistry::getReportManager(
    uint type) const {
  auto findIt = m_managers.find(type);
  if (findIt != m_managers.cend()) return findIt->second.get();

  return nullptr;
}

}  // namespace FOEDAG
