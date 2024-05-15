/**
  * @file NCriticalPathModel.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathModel.h"

#include "NCriticalPathItem.h"
#include "SimpleLogger.h"

namespace FOEDAG {

NCriticalPathModel::NCriticalPathModel(QObject* parent)
    : QAbstractItemModel(parent) {
  qRegisterMetaType<ItemsHelperStructPtr>("std::shared_ptr<FOEDAG::ItemsHelperStruct>");
  m_rootItem = new NCriticalPathItem;

  m_lineLimiterTimer.setInterval(LINE_LIMITER_FILTER_TIME_MS);
  m_lineLimiterTimer.setSingleShot(true);
  connect(&m_lineLimiterTimer, &QTimer::timeout, this,
          &NCriticalPathModel::applyLineCharsNum);
}

NCriticalPathModel::~NCriticalPathModel() { delete m_rootItem; }

void NCriticalPathModel::clear() {
  SimpleLogger::instance().debug("clear path model");
  beginResetModel();
  m_rootItem->deleteChildItems();
  endResetModel();

  m_inputNodes.clear();
  m_outputNodes.clear();

  emit cleared();
}

int NCriticalPathModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return static_cast<NCriticalPathItem*>(parent.internalPointer())
        ->columnCount();
  }
  return m_rootItem->columnCount();
}

QVariant NCriticalPathModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  NCriticalPathItem* item =
      static_cast<NCriticalPathItem*>(index.internalPointer());
  if (item) {
    switch (role) {
      case Qt::DisplayRole:
        return item->data(index.column());
      case Qt::DecorationRole:
        return item->isSelectable();
    }
  }

  return QVariant();
}

bool NCriticalPathModel::isSelectable(const QModelIndex& index) const {
  if (!index.isValid()) {
    return false;
  }
  NCriticalPathItem* item =
      static_cast<NCriticalPathItem*>(index.internalPointer());
  return item->isSelectable();
}

Qt::ItemFlags NCriticalPathModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

  // Check if you want to make this item not selectable
  if (!isSelectable(index)) {
    return defaultFlags & ~Qt::ItemIsSelectable;
  }

  return defaultFlags;
}

QVariant NCriticalPathModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return m_rootItem->data(section);
  }

  return QVariant();
}

QModelIndex NCriticalPathModel::index(int row, int column,
                                      const QModelIndex& parentIndex) const {
  if (!hasIndex(row, column, parentIndex)) {
    return QModelIndex();
  }

  NCriticalPathItem* parentItem = nullptr;

  if (!parentIndex.isValid()) {
    parentItem = m_rootItem;
  } else {
    parentItem = static_cast<NCriticalPathItem*>(parentIndex.internalPointer());
  }

  NCriticalPathItem* childItem = parentItem->child(row);
  if (childItem) {
    return createIndex(row, column, childItem);
  }
  return QModelIndex();
}

QModelIndex NCriticalPathModel::parent(const QModelIndex& index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }

  NCriticalPathItem* childItem =
      static_cast<NCriticalPathItem*>(index.internalPointer());
  NCriticalPathItem* parentItem = childItem->parentItem();

  if (parentItem == m_rootItem) {
    return QModelIndex();
  }

  return createIndex(parentItem->row(), 0, parentItem);
}

int NCriticalPathModel::rowCount(const QModelIndex& parent) const {
  NCriticalPathItem* parentItem = nullptr;
  if (parent.column() > 0) {
    return 0;
  }

  if (!parent.isValid()) {
    parentItem = m_rootItem;
  } else {
    parentItem = static_cast<NCriticalPathItem*>(parent.internalPointer());
  }

  return parentItem->childCount();
}

void NCriticalPathModel::loadFromString(QString rawData) {
  NCriticalPathModelLoader* modelLoader =
      new NCriticalPathModelLoader(std::move(rawData));
  connect(modelLoader, &NCriticalPathModelLoader::itemsReady, this,
          &NCriticalPathModel::loadItems);
  connect(modelLoader, &QThread::finished, modelLoader, &QThread::deleteLater);
  modelLoader->start();
}

void NCriticalPathModel::loadItems(
    const ItemsHelperStructPtr& itemsHelperStructPtr) {
  clear();

  for (const auto& [item, rootItem] : itemsHelperStructPtr->items) {
    insertNewItem(rootItem ? rootItem : m_rootItem, item);
  }

  m_inputNodes = std::move(itemsHelperStructPtr->inputNodes);
  m_outputNodes = std::move(itemsHelperStructPtr->outputNodes);
  itemsHelperStructPtr->inputNodes.clear();
  itemsHelperStructPtr->outputNodes.clear();

  emit loadFinished();
  SimpleLogger::instance().debug("load model finished");
}

int NCriticalPathModel::findRow(NCriticalPathItem* item) const {
  NCriticalPathItem* parentItem = item->parentItem();
  if (item && parentItem) {
    for (int row = 0; row < parentItem->childCount(); ++row) {
      if (parentItem->child(row) == item) {
        return row;
      }
    }
  }
  return -1;  // Return -1 if item is the root or not found
}

int NCriticalPathModel::findColumn(NCriticalPathItem*) const {
  return 0;  // assuming a single-column tree structure
}

void NCriticalPathModel::insertNewItem(NCriticalPathItem* parentItem,
                                       NCriticalPathItem* newItem) {
  newItem->setParent(parentItem);
  int row = parentItem->childCount();
  QModelIndex parentIndex;
  if (parentItem != m_rootItem) {
    int parentRow = findRow(parentItem);
    int parentColumn = findColumn(parentItem);
    parentIndex = createIndex(parentRow, parentColumn,
                              const_cast<NCriticalPathItem*>(parentItem));
  }

  beginInsertRows(parentIndex, row, row);
  parentItem->appendChild(newItem);
  endInsertRows();
}

void NCriticalPathModel::limitLineCharsNum(std::size_t lineCharsMaxNum) {
  if (lineCharsMaxNum < LINE_CHAR_NUM_MIN) {
    lineCharsMaxNum = LINE_CHAR_NUM_MIN;
  }
  if (m_lineCharsMaxNum != lineCharsMaxNum) {
    m_lineCharsMaxNum = lineCharsMaxNum;
    if (m_lineLimiterTimer.isActive()) {
      m_lineLimiterTimer.stop();
    }
    m_lineLimiterTimer.start();
  }
}

void NCriticalPathModel::applyLineCharsNum() {
  bool hasChanges = false;
  if (m_rootItem) {
    for (int pRow = 0; pRow < m_rootItem->childCount(); ++pRow) {
      NCriticalPathItem* pathItem = m_rootItem->child(pRow);
      if (pathItem->limitLineCharsNum(m_lineCharsMaxNum)) {
        hasChanges = true;
      }
      for (int eRow = 0; eRow < pathItem->childCount(); ++eRow) {
        NCriticalPathItem* elementItem = pathItem->child(eRow);
        if (elementItem->limitLineCharsNum(m_lineCharsMaxNum)) {
          hasChanges = true;
        }
      }
    }
  }

  if (hasChanges) {
    // notify viewer that data has changed
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
  }
}

}  // namespace FOEDAG
