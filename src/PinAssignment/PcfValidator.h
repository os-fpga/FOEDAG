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

  struct PcfLineFrame {
    int lineNum = -1;
    QString line;
    QString cmd;
    QString port;
    QString pin;
  };

class PcfValidator : public QObject {
  Q_OBJECT

  const int PCF_FILE_CHECK_INTERVAL_MS = 1000;

  const QString WRONG_SYNTAX_ERROR_TEMPLATE = "Bad pcf syntax";
  const QString WRONG_CMD_ERROR_TEMPLATE = "Bad pcf command '%1', 'set_io' is expected";
  const QString WRONG_PORT_AND_PIN_ERROR_TEMPLATE = "Bad port '%1' and pin '%2'";
  const QString WRONG_PORT_ERROR_TEMPLATE = "Bad port '%1'";
  const QString WRONG_PIN_ERROR_TEMPLATE = "Bad pin '%1'";
  const QString DUPLICATED_PORT_ERROR_TEMPLATE = "Port '%1' is already being used on line '%2'";
  const QString DUPLICATED_PIN_ERROR_TEMPLATE = "Pin '%1' is already being used on line '%2'";

public:
  PcfValidator(QObject* parent, const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel);

  static QList<PcfLineFrame> parsePcfFile(const QString& filePath);
  const QList<PcfLineFrame>& lineFrames(bool update=true);
  bool hasErrors() const { return !m_errors.isEmpty(); }

signals:
  void errorsChanged(QVector<QVector<QString>>);

private slots:
  void check();

private:
  QString m_filePath;
  QStringListModel* m_portsModel = nullptr;
  QStringListModel* m_pinsModel = nullptr;
  QTimer m_pcfFileCheckTimer;
  QDateTime m_lastModified;

  QList<PcfLineFrame> m_lineFrames;
  QSet<QString> m_prevErrorIds;
  QHash<QString, QVector<QString>> m_errors;

  void parsePcfFile();

  void checkLineStructure();
  void checkPortsAndPinsAvailability();
  void checkPortsAndPinsDuplication();

  void regError(int lineNum, const QString& line, const QString& errorMsg);
};

} // namespace FOEDAG