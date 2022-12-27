#include "AbstractReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <set>

#include "Compiler/TaskManager.h"
#include "NewProject/ProjectManager/project.h"

namespace {
static constexpr const char *RESOURCES_SPLIT{"blocks of type:"};
static constexpr const char *BLOCKS_COL{"Blocks"};

static const QRegExp FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};
static const QRegExp WARNING_REGEXP("Warning [0-9].*:");
static const QRegExp ERROR_REGEXP("Error [0-9].*:");

static const QRegularExpression SPLIT_HISTOGRAM{
    "((([0-9]*[.])?[0-9]+)e?[+-]?%?)+|\\*.*"};
}  // namespace

namespace FOEDAG {

const QRegExp AbstractReportManager::FIND_RESOURCES{"Resource usage.*"};
const QRegExp AbstractReportManager::FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};

AbstractReportManager::AbstractReportManager(const TaskManager &taskManager) {
  // Log files should be re-parsed after starting new compilation
  connect(&taskManager, &TaskManager::started,
          [this]() { setFileParsed(false); });
  m_timingColumns = {ReportColumn{"Statistics"},
                     ReportColumn{"Value", Qt::AlignCenter}};
  m_histogramColumns = {ReportColumn{"From"}, ReportColumn{"To"},
                        ReportColumn{"Value", Qt::AlignCenter},
                        ReportColumn{"%", Qt::AlignCenter},
                        ReportColumn{"Histogram"}};
}

void AbstractReportManager::parseResourceUsage(QTextStream &in, int &lineNr) {
  m_resourceColumns.clear();

  m_resourceColumns.push_back(ReportColumn{QString(BLOCKS_COL)});

  int childSum{0};     // Total number, assumulated within a single parent
  int columnIndex{0};  // Index of a column we fill the value for

  // Lambda setting given value for a certain row. Modifies existing row, if
  // any, or adds a new one.
  auto setValue = [&](const QString &row, const QString &value) {
    auto findIt = std::find_if(
        m_resourceData.begin(), m_resourceData.end(),
        [&row](const auto &lineValues) { return lineValues[0] == row; });
    if (findIt == m_resourceData.end()) {
      auto lineValues = IDataReport::LineValues{row, {}, {}};
      lineValues[columnIndex] = value;
      m_resourceData.push_back(std::move(lineValues));
    } else {
      // The resource has been added before - just update the corresponding
      // column
      (*findIt)[columnIndex] = value;
    }
  };

  QString lineStr, resourceName;
  auto currentColumn = ReportColumn{{}, Qt::AlignCenter};
  while (in.readLineInto(&lineStr)) {
    ++lineNr;
    auto line = lineStr.simplified();
    if (line.isEmpty()) break;

    auto lineStrs = line.split(QString(RESOURCES_SPLIT));
    // Column values are expected in a following format: Value RESOURCES_SPLIT
    // ResourceName
    if (lineStrs.size() == 2) {
      columnIndex = m_resourceColumns.indexOf(currentColumn);
      resourceName = lineStrs[1];

      setValue(resourceName, lineStrs[0]);
      childSum += lineStrs[0].toInt();
    } else {
      auto resourceNameSplit = resourceName.split("_");
      // in case resource name is of 'parent_resource' format, get parent name
      // and set accumulated number to it
      if (resourceNameSplit.size() > 1)
        setValue(resourceNameSplit.first(), QString::number(childSum));
      currentColumn.m_name = line;
      // We can't use set because columns order has to be kept.
      if (!m_resourceColumns.contains(currentColumn))
        m_resourceColumns << currentColumn;
      childSum = 0;  // New parent - reset the SUM
    }
  }
}

IDataReport::TableData AbstractReportManager::parseCircuitStats(QTextStream &in,
                                                                int &lineNr) {
  auto circuitData = IDataReport::TableData{};

  auto isTotalLine = [](QString &line) -> bool {
    return !line.startsWith(
        "    ");  // child items have more space at the beginning
  };
  QStringList totalLine{};
  QString line;

  while (in.readLineInto(&line)) {
    ++lineNr;
    auto simplifiedLine = line.simplified();
    auto lineData = simplifiedLine.split(":");
    if (lineData.size() != 2)  // expected format is: "block : value";
      continue;

    if (isTotalLine(line)) {
      // We are only interested in first section(total with parents).
      // Second total line breaks the loop and ends parsing
      if (totalLine.isEmpty())
        totalLine << QString("Total") << lineData[1];
      else
        break;
    } else {
      circuitData.push_back(lineData);
    }
  }

  return circuitData;
}

std::unique_ptr<QFile> AbstractReportManager::createLogFile(
    const QString &fileName) const {
  auto projectPath = Project::Instance()->projectPath().toStdString();
  auto logFilePath =
      (std::filesystem::path(projectPath) / fileName.toStdString()).string();

  auto logFile = std::make_unique<QFile>(QString::fromStdString(logFilePath));
  if (!logFile->open(QIODevice::ExistingOnly | QIODevice::ReadOnly |
                     QIODevice::Text))
    return nullptr;

  return logFile;
}

// Given function groups errors/warnings, coming one after another, into a
// single item. In case some irrelevant data is in-between, it's ignored and
// errors/warnings group is kept.
int AbstractReportManager::parseErrorWarningSection(QTextStream &in, int lineNr,
                                                    const QString &sectionLine,
                                                    SectionKeys keys) {
  auto sectionName = sectionLine;
  sectionName = sectionName.remove('#').simplified();
  auto sectionMsg = TaskMessage{lineNr, MessageSeverity::NONE, sectionName, {}};

  QString line;
  // Store line numbers in set to always know which one was first
  std::map<int, QString> warnings, errors;
  auto fillErrorsWarnings = [&warnings, &errors, &sectionMsg, this]() {
    if (!warnings.empty()) {
      auto wrnItem =
          createWarningErrorItem(MessageSeverity::WARNING_MESSAGE, warnings);
      sectionMsg.m_childMessages.insert(wrnItem.m_lineNr, wrnItem);
    }
    if (!errors.empty()) {
      auto errorsItem =
          createWarningErrorItem(MessageSeverity::ERROR_MESSAGE, errors);
      sectionMsg.m_childMessages.insert(errorsItem.m_lineNr, errorsItem);
    }
  };

  auto timings = QStringList{};
  while (in.readLineInto(&line)) {
    ++lineNr;
    // We reached the end of section
    if (line.startsWith(sectionLine)) break;

    // check whether current line is among desired keys
    for (auto &keyRegExp : keys) {
      if (keyRegExp.indexIn(line) != -1) {
        auto tm = TaskMessage{
            lineNr, MessageSeverity::INFO_MESSAGE, keyRegExp.cap(), {}};
        sectionMsg.m_childMessages.insert(lineNr, std::move(tm));
        // remove the key once found to save some performance
        keys.removeAll(keyRegExp);

        // Info message was found and errors/warnings group is finished, if any.
        fillErrorsWarnings();

        break;
      }
    }

    if (line.contains(WARNING_REGEXP)) {  // group warnings
      warnings.emplace(lineNr, line.simplified());
      // Warning means errors group is finished and we can create an item
      if (!errors.empty()) {
        auto errorsItem =
            createWarningErrorItem(MessageSeverity::ERROR_MESSAGE, errors);
        sectionMsg.m_childMessages.insert(errorsItem.m_lineNr, errorsItem);
      }
    } else if (line.contains(ERROR_REGEXP)) {  // group errors
      errors.emplace(lineNr, line.simplified());
      if (!warnings.empty()) {
        auto wrnItem =
            createWarningErrorItem(MessageSeverity::WARNING_MESSAGE, warnings);
        sectionMsg.m_childMessages.insert(wrnItem.m_lineNr, wrnItem);
      }
    } else if (FIND_RESOURCES.indexIn(line) != -1) {
      parseResourceUsage(in, lineNr);
    } else if (isStatisticalTimingLine(line)) {
      timings << line + "\n";
    } else if (isStatisticalTimingHistogram(line)) {
      m_histograms.push_back(qMakePair(line, parseHistogram(in, lineNr)));
    }
  }

  fillErrorsWarnings();

  if (!timings.isEmpty()) fillTimingData(timings);

  m_messages.insert(sectionMsg.m_lineNr, std::move(sectionMsg));
  return lineNr;
}  // namespace FOEDAG

TaskMessage AbstractReportManager::createWarningErrorItem(
    MessageSeverity severity, MessagesLines &msgs) const {
  auto itemName = severity == MessageSeverity::ERROR_MESSAGE
                      ? QString("Errors")
                      : QString("Warnings");
  auto itemLine = msgs.begin()->first;
  if (msgs.size() == 1) {
    auto result = TaskMessage{itemLine, severity, msgs.begin()->second, {}};
    msgs.clear();
    return result;
  }

  auto itemMsg =
      QString("%1 %2 found").arg(QString::number(msgs.size()), itemName);
  auto item = TaskMessage{itemLine, severity, itemMsg, {}};
  for (auto &msg : msgs) {
    auto msgItem =
        TaskMessage{msg.first, MessageSeverity::NONE, msg.second, {}};
    item.m_childMessages.insert(msg.first, std::move(msgItem));
  }

  msgs.clear();
  return item;
}

void AbstractReportManager::fillTimingData(const QStringList &timingData) {
  m_timingData.clear();

  createTimingDataFile(timingData);

  splitTimingData(timingData.join(' '));
}

void AbstractReportManager::createTimingDataFile(
    const QStringList &timingData) {
  auto logFileName = getTimingLogFileName();
  if (logFileName.isEmpty()) return;

  auto projectPath = Project::Instance()->projectPath();
  auto logFilePath = QString("%1/%2").arg(projectPath, logFileName);

  auto timingLogFile = QFile(logFilePath);

  if (!timingLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;
  timingLogFile.resize(0);  // clear all previous contents

  QTextStream out(&timingLogFile);
  for (auto &line : timingData) out << line;

  timingLogFile.close();
}

IDataReport::TableData AbstractReportManager::parseHistogram(QTextStream &in,
                                                             int &lineNr) {
  IDataReport::TableData result;
  QString line;
  while (in.readLineInto(&line)) {
    ++lineNr;
    if (line.simplified().isEmpty()) break;
    auto match = SPLIT_HISTOGRAM.globalMatch(line);
    QStringList tableLine;
    while (match.hasNext()) tableLine << match.next().captured();
    result.push_back(std::move(tableLine));
  }
  return result;
}

void AbstractReportManager::setFileParsed(bool parsed) {
  m_fileParsed = parsed;
}

bool AbstractReportManager::isFileParsed() const { return m_fileParsed; }

bool AbstractReportManager::isStatisticalTimingLine(const QString &line) {
  return false;
}

bool AbstractReportManager::isStatisticalTimingHistogram(const QString &line) {
  return false;
}

}  // namespace FOEDAG
