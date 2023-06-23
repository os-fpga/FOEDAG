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
#include "JsonReportGenerator.h"

#include <QDir>

#include "Compiler/Reports/IDataReport.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;
namespace FOEDAG {

JsonReportGenerator::JsonReportGenerator(const ITaskReport &report,
                                         const QString &name,
                                         const QString &outDir)
    : ReportGenerator(report), m_dir(outDir), m_name(name) {}

void JsonReportGenerator::Generate() {
  json jUtils = json::array();
  for (auto &report : m_report.getDataReports()) {
    json obj = json::object();
    std::vector<StringVector> data;
    StringVector line;
    for (const auto &col : report->getColumns()) {
      line.push_back(col.m_name.toStdString());
    }
    obj[report->getName().toStdString()]["header"] = line;
    for (const auto &tableLine : report->getData()) {
      StringVector line;
      for (const auto &cell : tableLine) {
        line.push_back(cell.toStdString());
      }
      data.push_back(line);
    }
    if (data.empty()) continue;
    obj[report->getName().toStdString()]["data"] = data;
    jUtils += obj;
  }

  if (!jUtils.empty()) {
    const QString reportsFolder{"reports"};
    QDir outputDir{m_dir};
    outputDir.mkdir(reportsFolder);
    outputDir.cd(reportsFolder);
    QFile jsonFile{QString{"%1%2%3.json"}.arg(outputDir.absolutePath(),
                                              QDir::separator(), m_name)};
    if (jsonFile.open(QFile::WriteOnly | QFile::Text)) {
      jsonFile.write(QByteArray::fromStdString(jUtils.dump(4)));
      jsonFile.close();
    }
  }
}

}  // namespace FOEDAG
