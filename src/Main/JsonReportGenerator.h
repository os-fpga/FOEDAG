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

#include "ReportGenerator.h"

namespace FOEDAG {

class JsonReportGenerator : public ReportGenerator {
 public:
  JsonReportGenerator(const ITaskReport &report, const QString &name,
                      const QString &outDir);
  void Generate() override;

 private:
  QString m_dir{};
  QString m_name{};
};

}  // namespace FOEDAG
