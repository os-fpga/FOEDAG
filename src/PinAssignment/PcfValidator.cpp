#include "PcfValidator.h"
#include "Utils/QtUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QStringListModel>
#include <QDebug>

namespace FOEDAG {

PcfValidator::PcfValidator(const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel)
    : m_filePath(filePath)
      , m_portsModel(portsModel)
      , m_pinsModel(pinsModel)
{
    m_pcfFileCheckTimer.setInterval(PCF_FILE_CHECK_INTERVAL_MS);

    connect(&m_pcfFileCheckTimer, &QTimer::timeout, this, &PcfValidator::check);
}

void PcfValidator::check()
{
    qInfo() << "~~~ PcfValidator::check()";
    QFileInfo fi(m_filePath);
    if (!fi.exists()) {
      return;
    }

    QDateTime lastModified = fi.lastModified();
    if (lastModified != m_lastModified) {
      qInfo() << "~~~ file change detected";

      m_errors.clear();

      QList<LineFrame> frames = parsePcfFile();

      checkLineStructure(frames);
      checkPortsAndPinsAvailability(frames);
      checkPortsAndPinsDuplication(frames);

      m_lastModified = lastModified;
    }
}

QList<PcfValidator::LineFrame> PcfValidator::parsePcfFile()
{
  QList<LineFrame> frames;

  QFile file{m_filePath};
  if (file.open(QFile::ReadOnly)) {
    QList<QString> lines = QtUtils::StringSplit(QString{file.readAll()}, '\n');
    int lineCount = 0;
    for (const QString& line: lines) {
      LineFrame frame;
      frame.lineNum = lineCount;
      frame.line = line;
      frame.elements = QtUtils::StringSplit(line, ' ');

      frames.append(frame);

      lineCount++;
    }

    file.close();
  }

  return frames;
}

void PcfValidator::checkLineStructure(const QList<LineFrame>& frames)
{
  for (const LineFrame& frame: frames) {
    if (frame.elements.size() == 3) {
      const QString cmd{frame.elements.at(0)};
      if (cmd != "set_io") {
        m_errors.append(PcfError{frame.lineNum, frame.line, "cmd != 'set_io'"});
      }
    } else {
      m_errors.append(PcfError{frame.lineNum, frame.line, "wrong syntax"});
    }
  }
}

void PcfValidator::checkPortsAndPinsAvailability(const QList<LineFrame>& frames)
{
  const QSet<QString> availablePorts = QSet<QString>::fromList(m_portsModel->stringList());
  const QSet<QString> availablePins = QSet<QString>::fromList(m_pinsModel->stringList());

  for (const LineFrame& frame: frames) {
    if (frame.elements.size() == 3) {
      const QString port{frame.elements.at(1)};
      const QString pin{frame.elements.at(2)};

      const bool isPortAvailable = availablePorts.contains(port);
      const bool isPinAvailable = availablePins.contains(pin);

      if (!isPortAvailable && !isPinAvailable) {
        m_errors.append(PcfError{frame.lineNum, frame.line, "!isPortAvailable && !isPinAvailable"});
      } else if (!isPortAvailable) {
        m_errors.append(PcfError{frame.lineNum, frame.line, "!isPortAvailable"});
      } else if (!isPinAvailable) {
        m_errors.append(PcfError{frame.lineNum, frame.line, "!isPinAvailable"});
      }
    }
  }
}

void PcfValidator::checkPortsAndPinsDuplication(const QList<LineFrame>& frames)
{
  QMap<QString, int> busyPorts;
  QMap<QString, int> busyPins;

  for (const LineFrame& frame: frames) {
    if (frame.elements.size() == 3) {
      const QString port{frame.elements.at(1)};
      const QString pin{frame.elements.at(2)};
      if (busyPorts.contains(port)) {
        m_errors.append(PcfError{frame.lineNum, frame.line, QString("duplicated port %1 on line %2, erlier is used on line %3").arg(port).arg(frame.lineNum).arg(busyPorts.value(port))});
      } else {
        busyPorts[port] = frame.lineNum;
      }

      if (busyPins.contains(pin)) {
        m_errors.append(PcfError{frame.lineNum, frame.line, QString("duplicated pin %1 on line %2, erlier is used on line %3").arg(pin).arg(frame.lineNum).arg(busyPins.value(pin))});
      } else {
        busyPins[pin] = frame.lineNum;
      }
    }
  }
}

} // namespace FOEDAG
