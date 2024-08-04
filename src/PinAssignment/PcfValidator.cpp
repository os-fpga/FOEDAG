#include "PcfValidator.h"
#include "Utils/QtUtils.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QStringListModel>
#include <QDebug>

namespace FOEDAG {

PcfValidator::PcfValidator(QObject* parent, const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel)
    : QObject(parent)
    , m_filePath(filePath)
    , m_portsModel(portsModel)
    , m_pinsModel(pinsModel)
{
  m_pcfFileCheckTimer.setInterval(PCF_FILE_CHECK_INTERVAL_MS);

  connect(&m_pcfFileCheckTimer, &QTimer::timeout, this, &PcfValidator::check);

  m_pcfFileCheckTimer.start();
}

void PcfValidator::check()
{
  //qInfo() << "~~~ PcfValidator::check()";
  QFileInfo fi(m_filePath);
  if (!fi.exists()) {
    return;
  }

  QDateTime lastModified = fi.lastModified();
  if (lastModified != m_lastModified) {
    qInfo() << "~~~ file change detected, checking...";

    m_errors.clear();

    QList<LineFrame> frames = parsePcfFile();

    checkLineStructure(frames);
    checkPortsAndPinsAvailability(frames);
    checkPortsAndPinsDuplication(frames);

    const QSet<QString> errorIds = QSet<QString>::fromList(m_errors.keys());
    if (m_prevErrorIds != errorIds) {
      QVector<QVector<QString>> errors = QVector<QVector<QString>>::fromList(m_errors.values());
      emit errorsChanged(errors);
      qInfo() << "~~~ changed errors=" << errors;
      m_prevErrorIds = errorIds;
    }

    m_lastModified = lastModified;
  }
}

void PcfValidator::regError(int lineNum, const QString& line, const QString& errorMsg)
{
  auto generateUniqueIdFn = [](const QVector<QString>& row)->QString {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const auto &element: row) {
      hash.addData(element.toUtf8());
    }
    return hash.result().toHex();
  };

  QVector<QString> errorFrame{errorMsg, QString::number(lineNum), line};

  m_errors[generateUniqueIdFn(errorFrame)] = errorFrame;
}

QList<PcfValidator::LineFrame> PcfValidator::parsePcfFile()
{
  QList<LineFrame> frames;

  QFile file{m_filePath};
  if (file.open(QFile::ReadOnly)) {
    QList<QString> lines = QtUtils::StringSplit(QString{file.readAll()}, '\n');
    int lineCount = 1;
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
        regError(frame.lineNum, frame.line, WRONG_CMD_ERROR_TEMPLATE.arg(cmd));
      }
    } else {
      regError(frame.lineNum, frame.line, WRONG_SYNTAX_ERROR_TEMPLATE);
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
        regError(frame.lineNum, frame.line, WRONG_PORT_AND_PIN_ERROR_TEMPLATE.arg(port).arg(pin));
      } else if (!isPortAvailable) {
        regError(frame.lineNum, frame.line, WRONG_PORT_ERROR_TEMPLATE.arg(port));
      } else if (!isPinAvailable) {
        regError(frame.lineNum, frame.line, WRONG_PIN_ERROR_TEMPLATE.arg(pin));
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
        regError(frame.lineNum, frame.line, DUPLICATED_PORT_ERROR_TEMPLATE.arg(port).arg(busyPorts.value(port)));
      } else {
        busyPorts[port] = frame.lineNum;
      }

      if (busyPins.contains(pin)) {
        regError(frame.lineNum, frame.line, DUPLICATED_PIN_ERROR_TEMPLATE.arg(pin).arg(busyPins.value(pin)));
      } else {
        busyPins[pin] = frame.lineNum;
      }
    }
  }
}

} // namespace FOEDAG
