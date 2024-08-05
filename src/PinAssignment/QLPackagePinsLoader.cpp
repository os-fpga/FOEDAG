#include "QLPackagePinsLoader.h"

#include <Utils/QtUtils.h>
#include "IODirection.h"

#include <QSet>
#include <QRegularExpression>

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

std::pair<bool, QString> QLPackagePinsLoader::load(const QString &fileName) {
  initHeader();

  const auto &[success, content] = getFileContent(fileName);
  if (!success) return std::make_pair(success, content);

  InternalPins &internalPins = m_model->internalPinsRef();

  QStringList lines = QtUtils::StringSplit(content, '\n');
  parseHeader(lines.takeFirst());
  PackagePinGroup group{};
  QSet<QString> uniquePins;
  for (const auto &line : lines) {
    QStringList data = line.split(",");
    if (!data.first().isEmpty()) {
      if (!group.name.isEmpty() && (group.name != data.first())) {
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
      group.name = data.first();
    }

    QStringList dataMod;
    for (int i=0; i<=InternalPinName; ++i) {
      dataMod.append("");
    }

    QString pinName;
    if (data.size() >= COLUMN_MAPPED_PIN) {
      pinName = data.at(COLUMN_MAPPED_PIN);
      dataMod[PinName] = pinName;
      dataMod[BallName] = pinName;
      dataMod[BallId] = pinName;
    }

    if (!pinName.isEmpty()) {
      if (uniquePins.contains(pinName)) {
        continue;
      }
      uniquePins.insert(pinName);

      QString netlistName;
      if (data.size() >= COLUMN_NETLIST_NAME) {
        netlistName = data.at(COLUMN_NETLIST_NAME);
        dataMod[InternalPinName] = netlistName;
      }

      QString dir;
      if (data.size() >= COLUMN_GPIO_TYPE) {
        dir = data.at(COLUMN_GPIO_TYPE);
      }

      // if direction field is not specified we try to detect it using other columns
      if (dir.isEmpty()) {
        // try detect direction using netlist_name
        if (!netlistName.isEmpty()) {
          if (netlistName.startsWith("A2F_")) {
            dir = IODirection::INPUT;
          } else if (netlistName.startsWith("F2A_")) {
            dir = IODirection::OUTPUT;
          }
        }

        // try detect direction using port_name
        if (dir.isEmpty()) {
          static const QRegularExpression inputPattern{"[A-Z0-9]+_A2F\\[\\d+\\]"};
          static const QRegularExpression outputPattern{"[A-Z0-9]+_F2A\\[\\d+\\]"};

          if (data.size() >= COLUMN_PORT_NAME) {
            QString portName = data.at(COLUMN_PORT_NAME);
            if (!portName.isEmpty()) {

              if (portName.contains(inputPattern)) {
                dir = IODirection::INPUT;
              } else if (portName.contains(outputPattern)) {
                dir = IODirection::OUTPUT;
              }
            }
          }
        }
      }

      if (!dir.isEmpty()) {
        dataMod[Direction] = dir;
      }

      group.pinData.append({dataMod});
    }
  }
  if (m_model->userGroups().contains(group.name)) {
    m_model->append(group);  // append last
  }
  m_model->initListModel();

  return std::make_pair(true, QString());
}

}  // namespace FOEDAG
