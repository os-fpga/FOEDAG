#include "QLPackagePinsLoader.h"

#include <Utils/QtUtils.h>
#include "IODirection.h"

#include <QSet>
#include <QRegularExpression>
#include <QDebug>

namespace FOEDAG {

QLPackagePinsLoader::QLPackagePinsLoader(PackagePinsModel *model, QObject *parent)
    : PackagePinsLoader(model, parent) {
}

void QLPackagePinsLoader::initHeader()
{
  if (!m_model) {
    return;
  }

  if (!m_model->header().empty()) {
    return;
  }

  bool visible = true;

  int id = 0;

  m_model->appendHeaderData(HeaderData{"Package pin", "Package pin", id++, true});
  m_model->appendHeaderData(HeaderData{"Available", "How many pins are in the group", id++, true});
  m_model->appendHeaderData(HeaderData{"Ports", "User defined ports", id++, true});
}

void QLPackagePinsLoader::parseHeader(const QString &header)
{
  m_header.clear();
  const QStringList columns = header.split(",");
  for (const QString& column: columns) {
    m_header[column.toLower()] = m_header.size();
  }
}

std::pair<bool, QString> QLPackagePinsLoader::load(const QString &fileName) {
  initHeader();

  const auto &[success, content] = getFileContent(fileName);
  if (!success) return std::make_pair(success, content);

#ifdef UPSTREAM_PINPLANNER
  InternalPins &internalPins = m_model->internalPinsRef();
#endif

  QStringList lines = QtUtils::StringSplit(content, '\n');
  parseHeader(lines.takeFirst());

  if (!m_header.contains(COLUMN_ORIENTATION)) {
    return std::make_pair(false, QString("column %1 is missing, abort loading %2").arg(COLUMN_ORIENTATION).arg(fileName));
  }
  if (!m_header.contains(COLUMN_MAPPED_PIN)) {
    return std::make_pair(false, QString("column %1 is missing, abort loading %2").arg(COLUMN_MAPPED_PIN).arg(fileName));
  }
  if (!m_header.contains(COLUMN_PORT_NAME)) {
    return std::make_pair(false, QString("column %1 is missing, abort loading %2").arg(COLUMN_PORT_NAME).arg(fileName));
  }

  const int columnOrientationIndex = m_header.value(COLUMN_ORIENTATION);
  const int columnMappedPinIndex = m_header.value(COLUMN_MAPPED_PIN);
  const int columnPortNameIndex = m_header.value(COLUMN_PORT_NAME);

  PackagePinGroup group{};
  QSet<QString> uniquePins;
  for (const auto &line: lines) {
    QStringList data = line.split(",");
    if (data.size() >= columnOrientationIndex) {
      QString orientation{data.at(columnOrientationIndex)};
      if (!orientation.isEmpty()) {
        if (!group.name.isEmpty() && (group.name != orientation)) {
          bool acceptGroup = false;
          if (!m_model->userGroups().empty()) {
            acceptGroup = m_model->userGroups().contains(group.name);
          } else {
            acceptGroup = true;
          }

          if (acceptGroup) {
            m_model->append(group);
          }
          group.pinData.clear();
          uniquePins.clear();
        }
        group.name = orientation;
      }
    } else {
      qCritical() << QString("it looks like line [%1] doesn't contain column [%2]").arg(line).arg(COLUMN_ORIENTATION);
    }

    QStringList dataMod;
    for (int i=0; i<=InternalPinName; ++i) {
      dataMod.append("");
    }

    QString pinName;

    if (data.size() >= columnMappedPinIndex) {
      pinName = data.at(columnMappedPinIndex);
      dataMod[PinName] = pinName;
      dataMod[BallName] = pinName;
      dataMod[BallId] = pinName;
    } else {
      qCritical() << QString("it looks like line [%1] doesn't contain column [%2]").arg(line).arg(COLUMN_MAPPED_PIN);
    }

    if (!pinName.isEmpty()) {
      if (uniquePins.contains(pinName)) {
        continue;
      }
      uniquePins.insert(pinName);

      static const QRegularExpression inputPattern{"[A-Z0-9]+_A2F\\[\\d+\\]"};
      static const QRegularExpression outputPattern{"[A-Z0-9]+_F2A\\[\\d+\\]"};

      QString dir;
      if (data.size() >= columnPortNameIndex) {
        QString portName = data.at(columnPortNameIndex);
        if (!portName.isEmpty()) {
          if (portName.contains(inputPattern)) {
            dir = IODirection::INPUT;
          } else if (portName.contains(outputPattern)) {
            dir = IODirection::OUTPUT;
          }
        }
      } else {
        qCritical() << QString("it looks like line [%1] doesn't contain column [%2]").arg(line).arg(COLUMN_PORT_NAME);
      }

      if (!dir.isEmpty()) {
        dataMod[Direction] = dir;
      }

      group.pinData.append({dataMod});
    }
  }
  if (!m_model->userGroups().contains(group.name)) {
    m_model->append(group);  // append last
  }
  m_model->initListModel();

  return std::make_pair(true, QString{});
}

}  // namespace FOEDAG
