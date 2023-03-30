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

#include <qnamespace.h>

#include <memory>
#include <unordered_map>

#include "ITaskReportManager.h"

namespace FOEDAG {

class TaskReportManagerRegistry final {
 public:
  using ReportManagerPtr = std::shared_ptr<ITaskReportManager>;

  // Register given manager under the task type. Retuns false if manager has
  // been registered before
  bool registerReportManager(uint type, ReportManagerPtr manager);
  // Returns report manager the task type.
  ReportManagerPtr getReportManager(uint type) const;
  std::vector<uint> ids() const;

  void setSuppressList(const QStringList &s);

 private:
  std::unordered_map<uint, ReportManagerPtr> m_managers;
};

}  // namespace FOEDAG
