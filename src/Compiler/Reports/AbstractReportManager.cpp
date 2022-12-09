#include "AbstractReportManager.h"

#include <QFile>
#include <QTextStream>
#include <set>

#include "Compiler/TaskManager.h"
#include "NewProject/ProjectManager/project.h"

namespace {
static constexpr const char *RESOURCES_SPLIT{"blocks of type:"};
static constexpr const char *BLOCKS_COL{"Blocks"};

static const QRegExp WARNING_REGEXP("Warning [0-9].*:");
static const QRegExp ERROR_REGEXP("Error [0-9].*:");
}  // namespace

namespace FOEDAG {

const QRegExp AbstractReportManager::FIND_RESOURCES{"Resource usage.*"};

AbstractReportManager::AbstractReportManager(const TaskManager &taskManager) {
  // Log files should be re-parsed after starting new compilation
  connect(&taskManager, &TaskManager::started,
          [this]() { setFileParsed(false); });
}

ITaskReport::TableData AbstractReportManager::parseResourceUsage(
    QTextStream &in, QStringList &columns, int &lineNr) const {
  columns.clear();
  columns << QString(BLOCKS_COL);

  int childSum{0};     // Total number, assumulated within a single parent
  int columnIndex{0};  // Index of a column we fill the value for

  auto result = ITaskReport::TableData{};

  // Lambda setting given value for a certain row. Modifies existing row, if
  // any, or adds a new one.
  auto setValue = [&](const QString &row, const QString &value) {
    auto findIt = std::find_if(
        result.begin(), result.end(),
        [&row](const auto &lineValues) { return lineValues[0] == row; });
    if (findIt == result.end()) {
      auto lineValues = ITaskReport::LineValues{row, {}, {}};
      lineValues[columnIndex] = value;
      result.push_back(std::move(lineValues));
    } else {
      // The resource has been added before - just update the corresponding
      // column
      (*findIt)[columnIndex] = value;
    }
  };

  QString lineStr, columnName, resourceName;

  while (in.readLineInto(&lineStr)) {
    ++lineNr;
    auto line = lineStr.simplified();
    if (line.isEmpty()) break;

    auto lineStrs = line.split(QString(RESOURCES_SPLIT));
    // Column values are expected in a following format: Value RESOURCES_SPLIT
    // ResourceName
    if (lineStrs.size() == 2) {
      columnIndex = columns.indexOf(columnName);
      resourceName = lineStrs[1];

      setValue(resourceName, lineStrs[0]);
      childSum += lineStrs[0].toInt();
    } else {
      auto resourceNameSplit = resourceName.split("_");
      // in case resource name is of 'parent_resource' format, get parent name
      // and set accumulated number to it
      if (resourceNameSplit.size() > 1)
        setValue(resourceNameSplit.first(), QString::number(childSum));
      columnName = line;
      // We can't use set because columns order has to be kept.
      if (!columns.contains(columnName)) columns << columnName;
      childSum = 0;  // New parent - reset the SUM
    }
  }

  return result;
}

std::unique_ptr<QFile> AbstractReportManager::createLogFile(
    const QString &fileName) const {
  auto projectPath = Project::Instance()->projectPath();
  auto logFilePath = QString("%1/%2").arg(projectPath, fileName);

  auto logFile = std::make_unique<QFile>(logFilePath);
  if (!logFile->open(QIODevice::ExistingOnly | QIODevice::ReadOnly |
                     QIODevice::Text))
    return nullptr;

  return logFile;
}

int AbstractReportManager::parseErrorWarningSection(QTextStream &in, int lineNr,
                                                    const QString &sectionLine,
                                                    SectionKeys keys) {
  auto sectionName = sectionLine;
  sectionName = sectionName.remove('#').simplified();
  auto sectionMsg = TaskMessage{lineNr, MessageSeverity::NONE, sectionName, {}};

  QString line;
  // Store line numbers in set to always know which one was first
  std::set<int> wrnLines, errLines;
  while (in.readLineInto(&line)) {
    ++lineNr;
    // We reached the end of section
    if (line.startsWith(sectionLine)) break;

    if (line.contains(WARNING_REGEXP))  // count warnings
      wrnLines.insert(lineNr);
    else if (line.contains(ERROR_REGEXP))  // count errors
      errLines.insert(lineNr);
    // check whether section has any 'interesting' lines
    for (auto &keyRegExp : keys) {
      if (keyRegExp.indexIn(line) != -1) {
        auto tm = TaskMessage{
            lineNr, MessageSeverity::INFO_MESSAGE, keyRegExp.cap(), {}};
        sectionMsg.m_childMessages.insert(lineNr, std::move(tm));
        // remove the key once found to save some performance
        keys.removeAll(keyRegExp);
        break;
      }
    }
  }
  // Link to the first warning/error line. Section beginning if nothing was
  // found
  auto warningLine = wrnLines.empty() ? sectionMsg.m_lineNr : *wrnLines.begin();
  auto warningMsg = TaskMessage{
      warningLine,
      MessageSeverity::WARNING_MESSAGE,
      QString("%1 warnings found").arg(QString::number(wrnLines.size())),
      {}};
  sectionMsg.m_childMessages.insert(warningLine, std::move(warningMsg));

  auto errorLine = errLines.empty() ? sectionMsg.m_lineNr : *errLines.begin();
  auto errorMsg = TaskMessage{
      errorLine,
      MessageSeverity::ERROR_MESSAGE,
      QString("%1 errors found").arg(QString::number(errLines.size())),
      {}};
  sectionMsg.m_childMessages.insert(errorLine, std::move(errorMsg));

  m_messages.insert(sectionMsg.m_lineNr, std::move(sectionMsg));
  return lineNr;
}

void AbstractReportManager::setFileParsed(bool parsed) {
  m_fileParsed = parsed;
}

bool AbstractReportManager::isFileParsed() const { return m_fileParsed; }

}  // namespace FOEDAG
