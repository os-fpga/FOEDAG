#include "NCriticalPathModelLoader.h"

#include "NCriticalPathItem.h"
#include "SimpleLogger.h"

//#define DEBUG_DUMP_RECEIVED_CRIT_PATH_TO_FILE

#ifdef DEBUG_DUMP_RECEIVED_CRIT_PATH_TO_FILE
#include <QFile>
#include <QList>
#endif

#include <QRegularExpression>

void NCriticalPathModelLoader::run() {
#ifdef DEBUG_DUMP_RECEIVED_CRIT_PATH_TO_FILE
  QFile file("received.report.dump.txt");
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << data;
    file.close();
  } else {
    qWarning() << "cannot open file for writing";
  }
#endif

  QList<QString> lines_ = m_rawData.split("\n");
  std::vector<std::string> lines;
  lines.reserve(lines_.size());
  for (const QString& line : qAsConst(lines_)) {
    lines.push_back(line.toStdString());
  }

  std::vector<GroupPtr> groups = NCriticalPathReportParser::parseReport(lines);

  std::map<int, std::pair<int, int>> metadata;
  NCriticalPathReportParser::parseMetaData(lines, metadata);

  createItems(groups, metadata);
}

void NCriticalPathModelLoader::createItems(
    const std::vector<GroupPtr>& groups,
    const std::map<int, std::pair<int, int>>& metadata) {
  ItemsHelperStructPtr itemsHelperStructPtr =
      std::make_shared<ItemsHelperStruct>();

  NCriticalPathItem* currentPathItem = nullptr;

  for (const GroupPtr& group : groups) {
    if (group->isPath()) {
      int selectableSegmentCounter = 0;
      int segmentCounter = 0;
      for (const auto& element : group->elements) {
        QList<QString> itemColumn1Data;
        QList<QString> itemColumn2Data;
        QList<QString> itemColumn3Data;
        int role = -1;
        for (const Line& line : element->lines) {
          // qInfo() << "process line[" << line.line.c_str() << "], role=" <<
          // line.role;
          if (role == -1) {
            // init role
            role = line.role;
          }

          if (role != line.role) {
            qCritical() << "bad role in line" << line.line.c_str() << line.role
                        << "where role expected" << role;
          }
          if (line.isMultiColumn) {
            auto [data, val1, val2] = extractRow(line.line.c_str());
            itemColumn1Data.append(data);
            itemColumn2Data.append(val1);
            itemColumn3Data.append(val2);
          } else {
            itemColumn1Data.append(line.line.c_str());
            itemColumn2Data.append("");
            itemColumn3Data.append("");
          }
        }

        QString data{itemColumn1Data.join("\n")};
        QString val1{itemColumn2Data.join("\n")};
        QString val2{itemColumn3Data.join("\n")};

        // qInfo() << "process" << data << val1 << val2;

        if (role == PATH) {
          NCriticalPathItem::Type type{NCriticalPathItem::PATH};
          int id =
              group->pathInfo.index -
              1;  // -1 here is because the path index starts from 1, not from 0
          int pathId = -1;
          bool isSelectable = true;

          currentPathItem = new NCriticalPathItem(data, val1, val2, type, id,
                                                  pathId, isSelectable);
          itemsHelperStructPtr->items.emplace_back(
              std::make_pair(currentPathItem, nullptr));
        } else if (role == SEGMENT) {
          if (currentPathItem) {
            NCriticalPathItem::Type type{NCriticalPathItem::PATH_ELEMENT};
            int pathId = currentPathItem->id();

            bool isSelectable = false;

            int pathIndex = pathId;
            auto it = metadata.find(pathIndex);
            if (it != metadata.end()) {
              const auto& [offset, num] = it->second;
              if ((segmentCounter > offset) &&
                  (segmentCounter < (offset + num))) {
                isSelectable = true;
                selectableSegmentCounter++;
              }
            }

            int id = isSelectable ? selectableSegmentCounter : -1;

            NCriticalPathItem* newItem = new NCriticalPathItem(
                data, val1, val2, type, id, pathId, isSelectable);
            itemsHelperStructPtr->items.emplace_back(
                std::make_pair(newItem, currentPathItem));

            segmentCounter++;
          } else {
            qCritical() << "path item is null";
          }
        } else if (role == OTHER) {
          if (currentPathItem) {
            NCriticalPathItem::Type type{NCriticalPathItem::OTHER};
            int id = -1;
            int pathId = currentPathItem->id();
            bool isSelectable = false;

            NCriticalPathItem* newItem = new NCriticalPathItem(
                data, val1, val2, type, id, pathId, isSelectable);
            itemsHelperStructPtr->items.emplace_back(
                std::make_pair(newItem, currentPathItem));
          } else {
            qCritical() << "path item is null";
          }
        }
      }

      // handle input
      QString input{group->pathInfo.start.c_str()};
      if (itemsHelperStructPtr->inputNodes.find(input) ==
          itemsHelperStructPtr->inputNodes.end()) {
        itemsHelperStructPtr->inputNodes[input] = 1;
      } else {
        itemsHelperStructPtr->inputNodes[input]++;
      }

      // handle output
      QString output{group->pathInfo.end.c_str()};
      if (itemsHelperStructPtr->outputNodes.find(output) ==
          itemsHelperStructPtr->outputNodes.end()) {
        itemsHelperStructPtr->outputNodes[output] = 1;
      } else {
        itemsHelperStructPtr->outputNodes[output]++;
      }
    } else {
      // process items not belong to path
      for (const auto& element : group->elements) {
        for (const Line& line : element->lines) {
          QString data{line.line.c_str()};
          QString val1{""};
          QString val2{""};
          NCriticalPathItem::Type type{NCriticalPathItem::OTHER};
          int id = -1;      // not used
          int pathId = -1;  // not used
          bool isSelectable = false;

          NCriticalPathItem* newItem = new NCriticalPathItem(
              data, val1, val2, type, id, pathId, isSelectable);
          itemsHelperStructPtr->items.emplace_back(
              std::make_pair(newItem, nullptr));
        }
      }
    }
  }

  emit itemsReady(itemsHelperStructPtr);
}

std::tuple<QString, QString, QString> NCriticalPathModelLoader::extractRow(
    QString l) const {
  l = l.simplified();

  QList<QString> data = l.split(" ");

  QString column2;
  QString column3;

  for (auto it = data.rbegin(); it != data.rend(); ++it) {
    QString el = *it;
    el = el.trimmed();
    if (el == "Path") {
      column3 = el;
      continue;
    }
    if (el == "Incr") {
      column2 = el;
      continue;
    }

    bool ok;
    el.toDouble(&ok);
    if (ok) {
      if (column3.isEmpty()) {
        column3 = el;
        continue;
      }
      if (column2.isEmpty()) {
        column2 = el;
        continue;
      }
    } else {
      break;
    }
  }

  if (!column3.isEmpty() && (data.last() == column3)) {
    data.pop_back();
  }
  if (!column2.isEmpty() && (data.last() == column2)) {
    data.pop_back();
  }

  QString column1{data.join(" ")};
  return {column1, column2, column3};
}
