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

class PcfValidator : public QObject {
  Q_OBJECT

  const int PCF_FILE_CHECK_INTERVAL_MS = 1000;

  struct LineFrame {
    int lineNum = -1;
    QString line;
    QList<QString> elements;
  };

public:
  PcfValidator(QObject* parent, const QString& filePath, QStringListModel* portsModel, QStringListModel* pinsModel);

signals:
  void errorsChanged(const QVector<QVector<QString>>&);

private slots:
  void check();

private:
  QString m_filePath;
  QStringListModel* m_portsModel = nullptr;
  QStringListModel* m_pinsModel = nullptr;
  QTimer m_pcfFileCheckTimer;
  QDateTime m_lastModified;

  QSet<QString> m_prevErrorIds;
  QHash<QString, QVector<QString>> m_errors;

  QList<LineFrame> parsePcfFile();
  void checkLineStructure(const QList<LineFrame>& frames);
  void checkPortsAndPinsAvailability(const QList<LineFrame>& frames);
  void checkPortsAndPinsDuplication(const QList<LineFrame>& frames);

  void regError(int lineNum, const QString& line, const QString& errorMsg);
};

} // namespace FOEDAG