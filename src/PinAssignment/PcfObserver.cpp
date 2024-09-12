#include "PcfObserver.h"
#include "PackagePinsModel.h"
#include "PortsModel.h"
#include "IODirection.h"

#include "Utils/QtUtils.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace FOEDAG {

PcfObserver::PcfObserver(QObject* parent, const QString& filePath, PortsModel* portsModel, PackagePinsModel* pinsModel)
    : QObject(parent)
    , m_filePath(filePath)
    , m_portsModel(portsModel)
    , m_pinsModel(pinsModel)
{
  m_checkTimer.setInterval(PCF_FILE_CHECK_INTERVAL_MS);

  connect(&m_checkTimer, &QTimer::timeout, this, &PcfObserver::check);

  m_checkTimer.start();
}

const QList<PcfLineFrame>& PcfObserver::lineFrames(bool update)
{
  if (update) {
    check();
  }
  return m_lineFrames;
}

void PcfObserver::forceNextCheck()
{
  m_checkTimer.stop();
  m_checkTimer.start();
  m_forceNextCheck = true;
}

void PcfObserver::check()
{
  QFileInfo fi(m_filePath);
  if (!fi.exists()) {
    return;
  }

  QDateTime lastModified = fi.lastModified();
  if ((lastModified != m_lastModified) || m_forceNextCheck) {
    m_errors.clear();

    parsePcfFile();

    checkLineStructure();
    checkPortsAndPinsAvailability();
    checkPortsAndPinsDuplication();
    checkInputOutputMix();

    m_lastModified = lastModified;
    if (m_forceNextCheck) {
      m_forceNextCheck = false;
    }

    emit contentChecked(m_errors.isEmpty());
  }
}

void PcfObserver::regError(int lineNum, const QString& line, const QString& errorMsg)
{
  m_errors.append({errorMsg, QString::number(lineNum), line});
}

QList<PcfLineFrame> PcfObserver::parsePcfFile(const QString& filePath)
{
  QList<PcfLineFrame> lineFrames;

  QFile file{filePath};
  if (file.open(QFile::ReadOnly)) {
    QTextStream in(&file);
    int lineCount = 1;
    while (!in.atEnd()) {
      QString line{in.readLine()};

      if (!line.isEmpty()) {
        PcfLineFrame frame;
        frame.lineNum = lineCount;
        frame.line = line;
        QList<QString> elements = QtUtils::StringSplit(line, ' ');
        if (elements.size() > 0) {
          frame.cmd = elements.at(0);
        }
        if (elements.size() > 1) {
          frame.port = elements.at(1);
        }
        if (elements.size() > 2) {
          frame.pin = elements.at(2);
        }

        lineFrames.append(frame);
      }

      lineCount++;
    }

    file.close();
  }

  return lineFrames;
}

void PcfObserver::parsePcfFile()
{
  m_lineFrames = parsePcfFile(m_filePath);
}

void PcfObserver::checkLineStructure()
{
  for (const PcfLineFrame& frame: m_lineFrames) {
    if (!frame.cmd.isEmpty() && !frame.port.isEmpty() && !frame.pin.isEmpty() ) {
      if (frame.cmd != "set_io") {
        regError(frame.lineNum, frame.line, WRONG_CMD_ERROR_TEMPLATE.arg(frame.cmd));
      }
    } else {
      regError(frame.lineNum, frame.line, WRONG_SYNTAX_ERROR_TEMPLATE);
    }
  }
}

void PcfObserver::checkPortsAndPinsAvailability()
{
  const QSet<QString> availablePorts = QtUtils::convertToSet(m_portsModel->listModel()->stringList());
  const QSet<QString> availablePins = QtUtils::convertToSet(m_pinsModel->listModel()->stringList());

  for (const PcfLineFrame& frame: m_lineFrames) {
    const bool isPortAvailable = availablePorts.contains(frame.port);
    const bool isPinAvailable = availablePins.contains(frame.pin);

    if (!isPortAvailable && !isPinAvailable) {
      regError(frame.lineNum, frame.line, WRONG_PORT_AND_PIN_ERROR_TEMPLATE.arg(frame.port).arg(frame.pin));
    } else if (!isPortAvailable) {
      regError(frame.lineNum, frame.line, WRONG_PORT_ERROR_TEMPLATE.arg(frame.port));
    } else if (!isPinAvailable) {
      regError(frame.lineNum, frame.line, WRONG_PIN_ERROR_TEMPLATE.arg(frame.pin));
    }
  }
}

void PcfObserver::checkPortsAndPinsDuplication()
{
  QMap<QString, int> busyPorts;
  QMap<QString, int> busyPins;

  for (const PcfLineFrame& frame: m_lineFrames) {
    if (busyPorts.contains(frame.port)) {
      regError(frame.lineNum, frame.line, DUPLICATED_PORT_ERROR_TEMPLATE.arg(frame.port).arg(busyPorts.value(frame.port)));
    } else {
      busyPorts[frame.port] = frame.lineNum;
    }

    if (busyPins.contains(frame.pin)) {
      regError(frame.lineNum, frame.line, DUPLICATED_PIN_ERROR_TEMPLATE.arg(frame.pin).arg(busyPins.value(frame.pin)));
    } else {
      busyPins[frame.pin] = frame.lineNum;
    }
  }
}

void PcfObserver::checkInputOutputMix()
{
  QSet<QString> inputPorts = QtUtils::convertToSet(m_portsModel->listModel(IODirection::INPUT)->stringList());
  QSet<QString> inputPins = QtUtils::convertToSet(m_pinsModel->listModel(IODirection::INPUT)->stringList());
  QSet<QString> outputPorts = QtUtils::convertToSet(m_portsModel->listModel(IODirection::OUTPUT)->stringList());
  QSet<QString> outputPins = QtUtils::convertToSet(m_pinsModel->listModel(IODirection::OUTPUT)->stringList());

  inputPorts.remove("");
  inputPins.remove("");
  outputPorts.remove("");
  outputPins.remove("");

  for (const PcfLineFrame& frame: m_lineFrames) {
    if (inputPorts.contains(frame.port) && outputPins.contains(frame.pin)) {
      regError(frame.lineNum, frame.line, MIXING_INPUT_PORT_AND_OUTPUT_PIN_TEMPLATE.arg(frame.port).arg(frame.pin));
    }
    if (outputPorts.contains(frame.port) && inputPins.contains(frame.pin)) {
      regError(frame.lineNum, frame.line, MIXING_OUTPUT_PORT_AND_INPUT_PIN_TEMPLATE.arg(frame.port).arg(frame.pin));
    }
  }
}

} // namespace FOEDAG
