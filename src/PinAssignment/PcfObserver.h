#pragma once

// #include "ErrorFrame.h"

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QSet>
#include <QHash>
#include <QVector>

class QStringListModel;

namespace FOEDAG {

class PortsModel;
class PackagePinsModel;

struct PcfLineFrame {
  int lineNum = -1;
  QString line;
  QString cmd;
  QString port;
  QString pin;
};

class PcfObserver : public QObject {
  Q_OBJECT

  const int PCF_FILE_CHECK_INTERVAL_MS = 1000;

  const QString WRONG_SYNTAX_ERROR_TEMPLATE = "Bad pcf syntax";
  const QString WRONG_CMD_ERROR_TEMPLATE = "Bad pcf command '%1', 'set_io' is expected";
  const QString WRONG_PORT_AND_PIN_ERROR_TEMPLATE = "Bad port '%1' and pin '%2'";
  const QString WRONG_PORT_ERROR_TEMPLATE = "Bad port '%1'";
  const QString WRONG_PIN_ERROR_TEMPLATE = "Bad pin '%1'";
  const QString DUPLICATED_PORT_ERROR_TEMPLATE = "Port '%1' is already being used on line '%2'";
  const QString DUPLICATED_PIN_ERROR_TEMPLATE = "Pin '%1' is already being used on line '%2'";
  const QString MIXING_INPUT_PORT_AND_OUTPUT_PIN_TEMPLATE = "Mixing input port '%1' and output pin '%2'";
  const QString MIXING_OUTPUT_PORT_AND_INPUT_PIN_TEMPLATE = "Mixing output port '%1' and input pin '%2'";

public:
  PcfObserver(QObject* parent, const QString& filePath, PortsModel* portsModel, PackagePinsModel* pinsModel);

  static QList<PcfLineFrame> parsePcfFile(const QString& filePath);
  const QList<PcfLineFrame>& lineFrames(bool update=true);
  const QVector<QVector<QString>>& errors() const { return m_errors; }
  bool hasErrors() const { return !m_errors.isEmpty(); }
  void forceNextCheck();

signals:
  void contentChecked(bool status);

private slots:
  void check();

private:
  bool m_forceNextCheck = false;
  QString m_filePath;
  PortsModel* m_portsModel{nullptr};
  PackagePinsModel* m_pinsModel{nullptr};
  QTimer m_checkTimer;
  QDateTime m_lastModified;

  QList<PcfLineFrame> m_lineFrames;
  QVector<QVector<QString>> m_errors;

  void parsePcfFile();

  void checkLineStructure();
  void checkPortsAndPinsAvailability();
  void checkPortsAndPinsDuplication();
  void checkInputOutputMix();

  void regError(int lineNum, const QString& line, const QString& errorMsg);
};

} // namespace FOEDAG