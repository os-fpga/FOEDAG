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

#include <QVector>
#include <filesystem>
#include <map>

#include "IDataReport.h"
#include "ITaskReportManager.h"

class QFile;
class QRegExp;
class QTextStream;

namespace FOEDAG {

class TaskManager;

/* Abstract implementation holding common logic for report managers.
 *
 */
class AbstractReportManager : public ITaskReportManager {
  Q_OBJECT
 public:
  AbstractReportManager(const TaskManager &taskManager);

 protected:
  // Launches file parsing, if needed. Returns messages.
  const Messages &getMessages() override;
  // Parses corresponding log file
  virtual void parseLogFile() = 0;
  // Getter for timing report file name
  virtual QString getTimingLogFileName() const = 0;
  // Returns true if given line holds statistical timing info.
  virtual bool isStatisticalTimingLine(const QString &line);
  // Returns true if given line holds is a header of statistical histogram.
  virtual bool isStatisticalTimingHistogram(const QString &line);
  // Parses in stream line by line till empty one occurs and creates table data.
  // Fills parsed data into 'm_resourceColumns' and 'm_resourceData'
  virtual std::filesystem::path logFile() const = 0;
  virtual void clean();
  void parseResourceUsage(QTextStream &in, int &lineNr);
  void designStatistics();

  // Creates and opens log file instance. returns nullptr if file doesn't exist.
  std::unique_ptr<QFile> createLogFile() const;

  using SectionKeys = QVector<QRegExp>;
  int parseErrorWarningSection(QTextStream &in, int lineNr,
                               const QString &sectionLine, SectionKeys keys,
                               bool stopEmptyLine = false);

  int parseStatisticsSection(QTextStream &in, int lineNr);

  IDataReport::TableData parseCircuitStats(QTextStream &in, int &lineNr);
  IDataReport::TableData CreateLogicData(bool lut5_6 = true);
  IDataReport::TableData CreateBramData() const;
  IDataReport::TableData CreateDspData() const;
  IDataReport::TableData CreateIOData() const;
  IDataReport::TableData CreateClockData() const;
  virtual void parseLogLine(const QString &line);
  void parseStatisticLine(const QString &line);

  using MessagesLines = std::map<int, QString>;
  // Creates parent item for either warnings or messages. Clears msgs
  // afterwards.
  TaskMessage createWarningErrorItem(MessageSeverity severity,
                                     MessagesLines &msgs) const;

  // Fills the values from given 'timingData' to m_timingData.
  void fillTimingData(const QStringList &timingData);
  // Create the file with 'timingData' content.
  void createTimingDataFile(const QStringList &timingData);
  // Splits histogram lines into table data till reaching empty line.
  IDataReport::TableData parseHistogram(QTextStream &in, int &lineNr);

  // Timing data is task specific and can't be split on generic level
  virtual void splitTimingData(const QString &timingStr) = 0;

  // Keyword to recognize the start of resource usage section
  static const QRegExp FIND_RESOURCES;
  static const QRegExp FIND_CIRCUIT_STAT;

  bool isFileOutdated(const std::filesystem::path &file) const;
  void setFileTimeStamp(const std::filesystem::path &file);
  std::filesystem::path logFilePath(const std::string &file) const;

  bool isMessageSuppressed(const QString &message) const;

 signals:
  void reportCreated(QString reportName);

 protected:
  IDataReport::ColumnValues m_resourceColumns;
  IDataReport::TableData m_resourceData;

  IDataReport::TableData m_timingData;
  IDataReport::ColumnValues m_timingColumns;

  IDataReport::ColumnValues m_histogramColumns;
  QVector<QPair<QString, IDataReport::TableData>> m_histograms;
  IDataReport::TableData m_ioData;
  IDataReport::ColumnValues m_ioColumns;
  IDataReport::TableData m_clockData;
  IDataReport::ColumnValues m_clockColumns;

  Messages m_messages;

 private:
  time_t m_fileTimeStamp{-1};
  const QString SPACE{"       "};
  const QString D_SPACE{"              "};
};

}  // namespace FOEDAG
