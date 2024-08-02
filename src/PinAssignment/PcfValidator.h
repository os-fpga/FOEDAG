#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QSet>

class QStringListModel;

namespace FOEDAG {

class PcfValidator : public QObject {
  Q_OBJECT
  const int PCF_FILE_CHECK_INTERVAL_MS = 1000;

  struct PcfError {
    int lineNum = -1;
    QString line;
    QString msg;
  };

  struct LineFrame {
    int lineNum = -1;
    QString line;
    QList<QString> elements;
  };

public:
  PcfValidator(const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel);

signals:
  void errorsChanged(const QList<QString>&);

private slots:
  void check();

private:
  QString m_filePath;
  QStringListModel* m_portsModel = nullptr;
  QStringListModel* m_pinsModel = nullptr;
  QTimer m_pcfFileCheckTimer;
  QDateTime m_lastModified;
  QList<PcfError> m_errors;

  QList<LineFrame> parsePcfFile();
  void checkLineStructure(const QList<LineFrame>& frames);
  void checkPortsAndPinsAvailability(const QList<LineFrame>& frames);
  void checkPortsAndPinsDuplication(const QList<LineFrame>& frames);
};

} // namespace FOEDAG