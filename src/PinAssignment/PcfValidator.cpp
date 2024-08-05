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

const QList<PcfLineFrame>& PcfValidator::lineFrames()
{
  check();
  return m_lineFrames;
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

    parsePcfFile();

    checkLineStructure();
    checkPortsAndPinsAvailability();
    checkPortsAndPinsDuplication();

    const QSet<QString> errorIds = QSet<QString>::fromList(m_errors.keys());
    if (m_prevErrorIds != errorIds) {
      QVector<QVector<QString>> errors = QVector<QVector<QString>>::fromList(m_errors.values());
      qInfo() << "~~~ changed errors=" << errors;
      m_prevErrorIds = errorIds;

      emit errorsChanged(errors);
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

void PcfValidator::parsePcfFile()
{
  m_lineFrames.clear();

  QFile file{m_filePath};
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

      m_lineFrames.append(frame);

      lineCount++;
    }

    file.close();
  }
}

void PcfValidator::checkLineStructure()
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

void PcfValidator::checkPortsAndPinsAvailability()
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

void PcfValidator::checkPortsAndPinsDuplication()
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
