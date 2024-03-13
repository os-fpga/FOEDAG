/**
  * @file NCriticalPathItem.cpp
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

#include "NCriticalPathItem.h"

#include "SimpleLogger.h"

namespace FOEDAG {

NCriticalPathItem::NCriticalPathItem() {
  m_itemData.resize(Column::END);
  m_itemData[Column::DATA] = "";
  m_itemData[Column::VAL1] = "";
  m_itemData[Column::VAL2] = "";
}

NCriticalPathItem::NCriticalPathItem(const QString& data, const QString& val1,
                                     const QString& val2, Type type, int id,
                                     int pathId, bool isSelectable,
                                     NCriticalPathItem* parent)
    : m_id(id),
      m_pathId(pathId),
      m_type(type),
      m_isSelectable(isSelectable),
      m_parentItem(parent) {
  m_itemData.resize(Column::END);
  m_itemData[Column::DATA] = data;
  m_itemData[Column::VAL1] = val1;
  m_itemData[Column::VAL2] = val2;

#ifdef DEBUG_NCRITICAL_PATH_ITEM_PROPERTIES
  m_itemData[Column::TYPE] = type;
  m_itemData[Column::ID] = id;
  m_itemData[Column::PATH_ID] = pathId;
  m_itemData[Column::IS_SELECTABLE] = isSelectable;
#endif

  // SimpleLogger::instance().debug("added",  m_itemData);

  if (isPath()) {
    QList<QString> d = data.split('\n');
    for (const QString& e : std::as_const(d)) {
      if (e.startsWith("Startpoint")) {
        m_startPointLine = e;
      } else if (e.startsWith("Endpoint")) {
        m_endPointLine = e;
      }
    }
  }
}

NCriticalPathItem::~NCriticalPathItem() { deleteChildItems(); }

void NCriticalPathItem::deleteChildItems() {
  if (!m_childItems.isEmpty()) {
    qDeleteAll(m_childItems);
    m_childItems.clear();
  }
}

void NCriticalPathItem::appendChild(NCriticalPathItem* item) {
  m_childItems.append(item);
}

NCriticalPathItem* NCriticalPathItem::child(int row) {
  if ((row < 0) || (row >= m_childItems.size())) {
    return nullptr;
  }
  return m_childItems.at(row);
}

int NCriticalPathItem::childCount() const { return m_childItems.count(); }

int NCriticalPathItem::columnCount() const { return m_itemData.count(); }

QVariant NCriticalPathItem::data(int column) const {
  if ((column < 0) || (column >= m_itemData.size())) {
    return QVariant();
  }
  return m_itemData.at(column);
}

NCriticalPathItem* NCriticalPathItem::parentItem() { return m_parentItem; }

int NCriticalPathItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(
        const_cast<NCriticalPathItem*>(this));
  }

  return 0;
}

}  // namespace FOEDAG