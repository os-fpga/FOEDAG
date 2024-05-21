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
#include "BitstreamReportManager.h"

#include <QFile>
#include <QTextStream>
#include <memory>

#include "Compiler.h"
#include "CompilerDefines.h"
#include "DefaultTaskReport.h"
static const QString STATISTIC_SECTION{"Pb types usage..."};

namespace FOEDAG {

BitstreamReportManager::BitstreamReportManager(const TaskManager &taskManager)
    : AbstractReportManager(taskManager) {}

QString BitstreamReportManager::getReportIdByType(ReportIdType idType) const {
  Q_UNUSED(idType)
  return {};
}

std::unique_ptr<ITaskReport> BitstreamReportManager::createReport(
    const QString &reportId) {
  return nullptr;
}

QString BitstreamReportManager::getTimingLogFileName() const {
  return QString{};
}

bool BitstreamReportManager::isStatisticalTimingLine(const QString &line) {
  return false;
}

bool BitstreamReportManager::isStatisticalTimingHistogram(const QString &line) {
  return false;
}

void BitstreamReportManager::splitTimingData(const QString &timingStr) {}

void BitstreamReportManager::parseLogFile() {
  auto logFile = createLogFile();
  if (!logFile) return;

  auto in = QTextStream(logFile.get());
  if (in.atEnd()) return;

  QString line;
  while (in.readLineInto(&line)) {
    parseStatisticLine(line);
    if (line.startsWith(STATISTIC_SECTION))
      parseStatisticsSection(in, -1);
    else if (line.startsWith(INTRA_DOMAIN_PATH_DELAYS_SECTION))
      parseSection(in, -1, [this](const QString &line) {
        parseIntraDomPathDelaysSection(line);
      });
  }
  CreateLogicData();
  CreateBramData();
  CreateDspData();
  CreateIOData();
  designStatistics();

  logFile->close();

  setFileTimeStamp(this->logFile());
  emit logFileParsed();
}

std::filesystem::path BitstreamReportManager::logFile() const {
  if (m_compiler)
    return m_compiler->FilePath(Compiler::Action::Bitstream, BITSTREAM_LOG);
  return logFilePath(BITSTREAM_LOG);
}

}  // namespace FOEDAG
