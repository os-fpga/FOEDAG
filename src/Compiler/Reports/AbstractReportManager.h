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
#include <QVector>

#include "ITaskReport.h"
#include "ITaskReportManager.h"

class QFile;
class QRegExp;
class QTextStream;

namespace FOEDAG {

class TaskManager;

/* Abstract implementation holding common logic for report managers.
 *
 */
class AbstractReportManager : public QObject, public ITaskReportManager {
  Q_OBJECT
 public:
  AbstractReportManager(const TaskManager &taskManager);

 protected:
  // Parses in stream line by line till empty one occurs and creates table data.
  ITaskReport::TableData parseResourceUsage(QTextStream &in,
                                            QStringList &columns,
                                            int &lineNr) const;

  // Creates and opens log file instance. returns nullptr if file doesn't exist.
  std::unique_ptr<QFile> createLogFile(const QString &fileName) const;

  using SectionKeys = QVector<QRegExp>;
  int parseErrorWarningSection(QTextStream &in, int lineNr,
                               const QString &sectionLine, SectionKeys keys);

  // Keyword to recognize the start of resource usage section
  static const QRegExp FIND_RESOURCES;

  bool isFileParsed() const;
  void setFileParsed(bool parsed);

 signals:
  void reportCreated(QString reportName);

 protected:
  ITaskReport::TableData m_stats;
  Messages m_messages;

 private:
  bool m_fileParsed{false};
};

}  // namespace FOEDAG
