#include "AbstractReportManager.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <set>

#include "Compiler/TaskManager.h"
#include "NewProject/ProjectManager/project.h"
#include "Utils/FileUtils.h"

namespace {
static constexpr const char *RESOURCES_SPLIT{"blocks of type:"};
static constexpr const char *BLOCKS_COL{"Blocks"};

static const QRegExp FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};
static const QRegExp WARNING_REGEXP("Warning( [0-9])?.*:");
static const QRegExp ERROR_REGEXP("Error( [0-9])?.*:");

static const QRegularExpression SPLIT_HISTOGRAM{
    "((([0-9]*[.])?[0-9]+)e?[+-]?%?)+|\\*.*"};
}  // namespace

namespace FOEDAG {

const QRegExp AbstractReportManager::FIND_RESOURCES{"Resource usage.*"};
const QRegExp AbstractReportManager::FIND_CIRCUIT_STAT{"Circuit Statistics:.*"};

AbstractReportManager::AbstractReportManager(const TaskManager &taskManager)
    : m_compiler(taskManager.GetCompiler()) {
  // Log files should be re-parsed after starting new compilation
  m_timingColumns = {ReportColumn{"Statistics"},
                     ReportColumn{"Value", Qt::AlignCenter}};
  m_histogramColumns = {ReportColumn{"From"}, ReportColumn{"To"},
                        ReportColumn{"Value", Qt::AlignCenter},
                        ReportColumn{"%", Qt::AlignCenter},
                        ReportColumn{"Histogram"}};
  m_ioColumns = {ReportColumn{"I/O"}, ReportColumn{"Used", Qt::AlignCenter},
                 ReportColumn{"Available", Qt::AlignCenter},
                 ReportColumn{"%", Qt::AlignCenter}};
  m_clockColumns = {ReportColumn{"Clock"},
                    ReportColumn{"Used", Qt::AlignCenter}};
}

const ITaskReportManager::Messages &AbstractReportManager::getMessages() {
  if (isFileOutdated(logFile())) parseLogFile();
  return m_messages;
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

void AbstractReportManager::designStatistics() {
  m_resourceColumns.clear();
  m_resourceColumns.push_back(ReportColumn{"Design statistics"});
  m_resourceColumns.push_back(ReportColumn{{}, Qt::AlignCenter});

  m_resourceData.clear();
  uint luts = m_usedRes.logic.lut5 + m_usedRes.logic.lut6;
  uint result =
      (m_usedRes.logic.clb == 0) ? 0 : luts / (m_usedRes.logic.clb * 8);
  m_resourceData.push_back(
      {"CLB LUT packing percentage", QString{"%1 %"}.arg(result)});
  uint registers = m_usedRes.logic.dff + m_usedRes.logic.latch;
  result =
      (m_usedRes.logic.clb == 0) ? 0 : registers / (m_usedRes.logic.clb * 16);
  m_resourceData.push_back(
      {"CLB Register packing percentage", QString{"%1 %"}.arg(result)});
  uint bram = m_usedRes.bram.bram_18k + m_usedRes.bram.bram_36k;
  result = (bram == 0)
               ? 0
               : m_usedRes.bram.bram_36k + (2 * m_usedRes.bram.bram_18k) / bram;
  m_resourceData.push_back(
      {"BRAM packing percentage", QString{"%1 %"}.arg(result)});
  m_resourceData.push_back({"Wires", QString{"%1"}.arg(m_usedRes.stat.wires)});
  m_resourceData.push_back(
      {"Max Fanout", QString{"%1"}.arg(m_usedRes.stat.maxFanout)});
  m_resourceData.push_back(
      {"Average Fanout", QString{"%1"}.arg(m_usedRes.stat.avgFanout)});

  m_resourceData.push_back(
      {"Maximum logic level", QString{"%1"}.arg(m_usedRes.stat.maxLogicLvel)});
  m_resourceData.push_back(
      {"Average logic level", QString{"%1"}.arg(m_usedRes.stat.avgLogicLvel)});
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

IDataReport::TableData AbstractReportManager::CreateLogicData(bool lut5_6) {
  auto circuitData = IDataReport::TableData{};
  Logic uLogic = m_usedRes.logic;
  Logic aLogic = m_availRes.logic;
  uint result = (aLogic.clb == 0) ? 0 : uLogic.clb * 100 / aLogic.clb;
  circuitData.push_back({"CLB", QString::number(uLogic.clb),
                         QString::number(aLogic.clb), QString::number(result)});

  uint usedLuts = uLogic.lut5 + uLogic.lut6;
  if (lut5_6) usedLuts = (uLogic.lut5 / 2) + uLogic.lut6;

  result = (aLogic.lut6 == 0) ? 0 : usedLuts * 100 / aLogic.lut6;
  circuitData.push_back({SPACE + "LUTs", QString::number(usedLuts),
                         QString::number(aLogic.lut6),
                         QString::number(result)});

  if (lut5_6) {
    result = (aLogic.lut5 == 0) ? 0 : uLogic.lut5 * 100 / aLogic.lut5;
    circuitData.push_back({D_SPACE + "LUT5", QString::number(uLogic.lut5),
                           QString::number(aLogic.lut5),
                           QString::number(result)});

    result = (aLogic.lut6 == 0) ? 0 : uLogic.lut6 * 100 / aLogic.lut6;
    circuitData.push_back({D_SPACE + "LUT6", QString::number(uLogic.lut6),
                           QString::number(aLogic.lut6),
                           QString::number(result)});
  }

  uint usedRegs = uLogic.dff + uLogic.latch;
  result = (aLogic.dff == 0) ? 0 : usedRegs * 100 / aLogic.dff;
  circuitData.push_back({SPACE + "Registers", QString::number(usedRegs),
                         QString::number(aLogic.dff), QString::number(result)});

  result = (aLogic.dff == 0) ? 0 : uLogic.dff * 100 / aLogic.dff;
  circuitData.push_back({D_SPACE + "Flip Flop", QString::number(uLogic.dff),
                         QString::number(aLogic.dff), QString::number(result)});

  // NOTE temporary removed since latches removed from arch but may be added
  //  result = (aLogic.latch == 0) ? 0 : uLogic.latch * 100 / aLogic.latch;
  //  circuitData.push_back({D_SPACE + "Latch", QString::number(uLogic.latch),
  //                         QString::number(aLogic.latch),
  //                         QString::number(result)});

  result = (aLogic.fa2Bits == 0) ? 0 : uLogic.fa2Bits * 100 / aLogic.fa2Bits;
  circuitData.push_back({SPACE + "Adder Carry", QString::number(uLogic.fa2Bits),
                         QString::number(aLogic.fa2Bits),
                         QString::number(result)});
  return circuitData;
}

IDataReport::TableData AbstractReportManager::CreateBramData() const {
  auto bramData = IDataReport::TableData{};
  Bram uBram = m_usedRes.bram;
  Bram aBram = m_availRes.bram;
  uint usedBram = uBram.bram_18k + uBram.bram_36k;
  uint availBram = aBram.bram_36k;
  uint result = (availBram == 0)
                    ? 0
                    : ((uBram.bram_18k / 2) + uBram.bram_36k) * 100 / availBram;
  bramData.push_back({"BRAM", QString::number(usedBram),
                      QString::number(availBram), QString::number(result)});

  // TODO not supported yet
  //  result = (aBram.bram_18k == 0) ? 0 : uBram.bram_18k * 100 /
  //  aBram.bram_18k; bramData.push_back({SPACE + "18k",
  //  QString::number(uBram.bram_18k),
  //                      QString::number(aBram.bram_18k),
  //                      QString::number(result)});

  result = (aBram.bram_36k == 0) ? 0 : uBram.bram_36k * 100 / aBram.bram_36k;
  bramData.push_back({SPACE + "36k", QString::number(uBram.bram_36k),
                      QString::number(aBram.bram_36k),
                      QString::number(result)});

  return bramData;
}

IDataReport::TableData AbstractReportManager::CreateDspData() const {
  auto dspData = IDataReport::TableData{};
  DSP uDsp = m_usedRes.dsp;
  DSP aDsp = m_availRes.dsp;
  uint usedDsp = uDsp.dsp_9_10 + uDsp.dsp_18_20;
  uint availDsp = aDsp.dsp_9_10;
  uint result = (availDsp == 0)
                    ? 0
                    : (uDsp.dsp_9_10 + (uDsp.dsp_18_20 / 2)) * 100 / availDsp;
  dspData.push_back({"DSP Block", QString::number(usedDsp),
                     QString::number(availDsp), QString::number(result)});

  result = (aDsp.dsp_9_10 == 0) ? 0 : uDsp.dsp_9_10 * 100 / aDsp.dsp_9_10;
  dspData.push_back({SPACE + "9x10", QString::number(uDsp.dsp_9_10),
                     QString::number(aDsp.dsp_9_10), QString::number(result)});

  result = (aDsp.dsp_18_20 == 0)
               ? 0
               : m_usedRes.bram.bram_36k * 100 / aDsp.dsp_18_20;
  dspData.push_back({SPACE + "18x20", QString::number(uDsp.dsp_18_20),
                     QString::number(aDsp.dsp_18_20), QString::number(result)});
  return dspData;
}

IDataReport::TableData AbstractReportManager::CreateIOData() const {
  auto ioData = IDataReport::TableData{};
  IO uIO = m_usedRes.inouts;
  IO aIO = m_availRes.inouts;
  uint result = (aIO.io == 0) ? 0 : uIO.io * 100 / aIO.io;
  ioData.push_back({"I/O", QString::number(uIO.io), QString::number(aIO.io),
                    QString::number(result)});

  result = (aIO.inputs == 0) ? 0 : uIO.inputs * 100 / aIO.inputs;
  ioData.push_back({SPACE + "Inputs", QString::number(uIO.inputs),
                    QString::number(aIO.inputs), QString::number(result)});

  result = (aIO.outputs == 0) ? 0 : uIO.outputs * 100 / aIO.outputs;
  ioData.push_back({SPACE + "Outputs", QString::number(uIO.outputs),
                    QString::number(aIO.outputs), QString::number(result)});
  return ioData;
}

IDataReport::TableData AbstractReportManager::CreateClockData() const {
  auto clockData = IDataReport::TableData{};
  clockData.push_back({"Clock", QString::number(m_usedRes.clocks.clock_num)});
  return clockData;
}

void AbstractReportManager::parseLogLine(const QString &line) {
  static const QRegularExpression clb{"^ +clb\\D+(\\d+)",
                                      QRegularExpression::MultilineOption};
  auto clbMatch = clb.match(line);
  if (clbMatch.hasMatch()) {
    m_usedRes.logic.clb = clbMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression lut5{"^ +lut5\\D+(\\d+)",
                                       QRegularExpression::MultilineOption};
  auto lut5Match = lut5.match(line);
  if (lut5Match.hasMatch()) {
    m_usedRes.logic.lut5 += lut5Match.captured(1).toUInt();
    return;
  }
  static const QRegularExpression lut6{"^ +lut6\\D+(\\d+)",
                                       QRegularExpression::MultilineOption};
  auto lut6Match = lut6.match(line);
  if (lut6Match.hasMatch()) {
    m_usedRes.logic.lut6 += lut6Match.captured(1).toUInt();
    return;
  }
  static const QRegularExpression lut6_{"^ +6-LUT\\D+(\\d+)",
                                        QRegularExpression::MultilineOption};
  auto lut6_Match = lut6_.match(line);
  if (lut6_Match.hasMatch()) {
    m_usedRes.logic.lut6 += lut6_Match.captured(1).toUInt();
    return;
  }

  static const QRegularExpression dff{
      "^ +(\\S*dff\\S*)\\D+(\\d+)",
      QRegularExpression::MultilineOption |
          QRegularExpression::CaseInsensitiveOption};
  auto dffMatch = dff.match(line);
  if (dffMatch.hasMatch()) {
    if (!dffMatch.captured(1).contains("SDFFRE", Qt::CaseInsensitive))
      m_usedRes.logic.dff += dffMatch.captured(2).toUInt();
    return;
  }
  static const QRegularExpression latch{
      "^ +latch\\D+(\\d+)", QRegularExpression::MultilineOption |
                                QRegularExpression::CaseInsensitiveOption};
  auto latchMatch = latch.match(line);
  if (latchMatch.hasMatch()) {
    m_usedRes.logic.latch += latchMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression carry2{"^ +adder_carry\\D+(\\d+)",
                                         QRegularExpression::MultilineOption};
  auto carry2Match = carry2.match(line);
  if (carry2Match.hasMatch()) {
    m_usedRes.logic.fa2Bits = carry2Match.captured(1).toUInt();
    return;
  }
  static const QRegularExpression bram36k{"^ +mem_36K\\D+(\\d+)",
                                          QRegularExpression::MultilineOption};
  auto bram36kMatch = bram36k.match(line);
  if (bram36kMatch.hasMatch()) {
    m_usedRes.bram.bram_36k = bram36kMatch.captured(1).toUInt();
    return;
  }
  // TODO not supported yet
  //  static const QRegularExpression bram18k{"^ +RS_TDP18K\\D+(\\d+)",
  //                                          QRegularExpression::MultilineOption};
  //  auto bram18kMatch = bram18k.match(line);
  //  if (bram18kMatch.hasMatch()) {
  //    m_usedRes.bram.bram_18k = bram18kMatch.captured(1).toUInt();
  //    return;
  //  }
  static const QRegularExpression dsp_18_20{
      "^ +RS_DSP_MULT\\D+(\\d+)", QRegularExpression::MultilineOption};
  auto dsp_18_20Match = dsp_18_20.match(line);
  if (dsp_18_20Match.hasMatch()) {
    m_usedRes.dsp.dsp_18_20 += dsp_18_20Match.captured(1).toUInt();
    return;
  }
  // TODO pattern TBD
  //  static const QRegularExpression dsp_9_10{"^ +RS_DSP_MULT\\D+(\\d+)",
  //                                            QRegularExpression::MultilineOption};
  //  auto dsp_9_10Match = dsp_9_10.match(line);
  //  if (dsp_9_10Match.hasMatch()) {
  //    m_usedResources.dsp.dsp_9_10 = dsp_9_10Match.captured(1).toUInt();
  //    return;
  //  }

  static const QRegularExpression io{"^ +io \\D+(\\d+)"};
  auto ioMatch = io.match(line);
  if (ioMatch.hasMatch()) {
    m_usedRes.inouts.io = ioMatch.captured(1).toUInt();
    return;
  }

  static const QRegularExpression inpad{"^ +inpad \\D+(\\d+)"};
  auto inpadMatch = inpad.match(line);
  if (inpadMatch.hasMatch()) {
    m_usedRes.inouts.inputs = inpadMatch.captured(1).toUInt();
    return;
  }

  static const QRegularExpression outpad{"^ +outpad \\D+(\\d+)"};
  auto outpadMatch = outpad.match(line);
  if (outpadMatch.hasMatch()) {
    m_usedRes.inouts.outputs = outpadMatch.captured(1).toUInt();
    return;
  }
}

void AbstractReportManager::parseStatisticLine(const QString &line) {
  static const QRegularExpression nets{"^ +Nets\\D+(\\d+)",
                                       QRegularExpression::MultilineOption};
  auto netsMatch = nets.match(line);
  if (netsMatch.hasMatch()) {
    m_usedRes.stat.wires = netsMatch.captured(1).toUInt();
    return;
  }
  static const QRegularExpression avgFanout{
      "^ +Avg Fanout\\D+([+-]?[[0-9]*[.]]?[0-9]+)",
      QRegularExpression::MultilineOption};
  auto avgFanoutMatch = avgFanout.match(line);
  if (avgFanoutMatch.hasMatch()) {
    m_usedRes.stat.avgFanout = avgFanoutMatch.captured(1).toDouble();
    return;
  }
  static const QRegularExpression maxFanout{
      "^ +Max Fanout\\D+([+-]?[[0-9]*[.]]?[0-9]+)",
      QRegularExpression::MultilineOption};
  auto maxFanoutMatch = maxFanout.match(line);
  if (maxFanoutMatch.hasMatch()) {
    m_usedRes.stat.maxFanout = maxFanoutMatch.captured(1).toDouble();
    return;
  }

  static const QRegularExpression findLvls{
      "^DE:.*Max Lvl =\\s*(([0-9]*[.])?[0-9]+)\\s*Avg Lvl "
      "=\\s*(([0-9]*[.])?[0-9]+)"};

  auto match = findLvls.match(line);
  if (match.hasMatch()) {
    m_usedRes.stat.maxLogicLvel = match.captured(1).toDouble();
    m_usedRes.stat.avgLogicLvel = match.captured(3).toDouble();
    return;
  }

  static const QRegularExpression fmax{"^.+Fmax:\\D+([+-]?[[0-9]*[.]]?[0-9]+)"};
  auto fmaxMatch = fmax.match(line);
  if (fmaxMatch.hasMatch()) {
    m_usedRes.stat.fmax = fmaxMatch.captured(1).toDouble();
    return;
  }
  static const QRegularExpression clock{"^ +Netlist Clocks\\D+(\\d+)"};
  auto clockMatch = clock.match(line);
  if (clockMatch.hasMatch()) {
    m_usedRes.clocks.clock_num = clockMatch.captured(1).toUInt();
    return;
  }
}

std::unique_ptr<QFile> AbstractReportManager::createLogFile() const {
  auto logFile =
      std::make_unique<QFile>(QString::fromStdString(this->logFile().string()));
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
                                                    SectionKeys keys,
                                                    bool stopEmptyLine) {
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
    parseStatisticLine(line);
    ++lineNr;
    // We reached the end of section
    if (line.startsWith(sectionLine)) break;
    if (stopEmptyLine && line.isEmpty()) break;

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

    if (line.contains(WARNING_REGEXP) &&
        !isMessageSuppressed(line)) {  // group warnings
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

  m_messages.insert(sectionMsg.m_lineNr, std::move(sectionMsg));
  return lineNr;
}

int AbstractReportManager::parseStatisticsSection(QTextStream &in, int lineNr) {
  QString line{};
  while (in.readLineInto(&line)) {
    ++lineNr;
    if (line.isEmpty()) break;  // end of section
    parseLogLine(line);
  }
  return lineNr;
}

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

void AbstractReportManager::setFileTimeStamp(
    const std::filesystem::path &file) {
  m_fileTimeStamp = FileUtils::Mtime(file);
}

std::filesystem::path AbstractReportManager::logFilePath(
    const std::string &file) const {
  auto projectPath = Project::Instance()->projectPath().toStdString();
  return std::filesystem::path(projectPath) / file;
}

bool AbstractReportManager::isMessageSuppressed(const QString &message) const {
  const auto suppressList = this->suppressList();
  for (const auto &sup : suppressList)
    if (message.contains(sup)) return true;
  return false;
}

bool AbstractReportManager::isFileOutdated(
    const std::filesystem::path &file) const {
  auto ts = FileUtils::Mtime(file);
  return ts > m_fileTimeStamp;
}

bool AbstractReportManager::isStatisticalTimingLine(const QString &line) {
  return false;
}

bool AbstractReportManager::isStatisticalTimingHistogram(const QString &line) {
  return false;
}

void AbstractReportManager::clean() { m_usedRes = {}; }

}  // namespace FOEDAG
