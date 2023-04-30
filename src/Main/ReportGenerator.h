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
#include <QString>
#include <memory>

#include "Compiler/Reports/ITaskReport.h"

class QBoxLayout;
namespace FOEDAG {

class ReportGenerator {
 public:
  ReportGenerator(const ITaskReport& report);
  virtual ~ReportGenerator();
  virtual void Generate() = 0;

 protected:
  const ITaskReport& m_report;
};

class TableReportGenerator : public ReportGenerator {
 public:
  TableReportGenerator(const ITaskReport& report, QBoxLayout* layoput);
  void Generate() override;

 protected:
  QBoxLayout* m_layout{nullptr};
};

class FileReportGenerator : public ReportGenerator {
 public:
  FileReportGenerator(const ITaskReport& report, const QString& fileName);
  void Generate() override;

 protected:
  QString m_fileName{};
};

template <typename Generator, typename... Args>
std::unique_ptr<ReportGenerator> CreateReportGenerator(
    const ITaskReport& report, Args&&... args) {
  return std::make_unique<Generator>(report, std::forward<Args>(args)...);
}

}  // namespace FOEDAG
