#include "PcfValidator.h"
#include "Utils/QtUtils.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QStringListModel>
#include <QDebug>

namespace FOEDAG {

PcfObserver::PcfObserver(QObject* parent, const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel)
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

void PcfObserver::check()
{
  QFileInfo fi(m_filePath);
  if (!fi.exists()) {
    return;
  }

  QDateTime lastModified = fi.lastModified();
  if (lastModified != m_lastModified) {
    m_errors.clear();

    parsePcfFile();

    checkLineStructure();
    checkPortsAndPinsAvailability();
    checkPortsAndPinsDuplication();

    m_lastModified = lastModified;

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
    QList<QString> lines = QtUtils::StringSplit(QString{file.readAll()}, '\n');
    int lineCount = 1;
    for (const QString& line: lines) {
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
  const QSet<QString> availablePorts = QSet<QString>::fromList(m_portsModel->stringList());
  const QSet<QString> availablePins = QSet<QString>::fromList(m_pinsModel->stringList());

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

} // namespace FOEDAG
