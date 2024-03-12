/**
  * @file NCriticalPathItem.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
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

#pragma once

#include <QVariant>
#include <QVector>
#include <QString>

namespace FOEDAG {

//#define DEBUG_NCRITICAL_PATH_ITEM_PROPERTIES

class NCriticalPathItem
{
public:
#ifndef DEBUG_NCRITICAL_PATH_ITEM_PROPERTIES
    enum Column {
        DATA,
        VAL1,
        VAL2,
        END
    };
#else
    enum Column {
        DATA,
        VAL1,
        VAL2,
        TYPE,
        ID,
        PATH_ID,
        IS_SELECTABLE,
        END
    };
#endif

    enum Type {
        PATH,
        PATH_ELEMENT,
        OTHER
    };

    NCriticalPathItem();
    explicit NCriticalPathItem(
        const QString& data, 
        const QString& val1, 
        const QString& val2, 
        Type type,
        int id,
        int pathId,
        bool isSelectable,
        NCriticalPathItem* parentItem = nullptr);

    ~NCriticalPathItem();

    const QString& startPointLine() const { return m_startPointLine; }
    const QString& endPointLine() const { return m_endPointLine; }

    void deleteChildItems();

    void appendChild(NCriticalPathItem* child);

    int id() const { return m_id; }
    int pathIndex() const { return m_pathId; }
    Type type() const { return m_type; }

    bool isPath() const { return m_type == Type::PATH; }
    bool isSelectable() const { return m_isSelectable; }

    NCriticalPathItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    NCriticalPathItem* parentItem();

private:
    int m_id = -1;
    int m_pathId = -1;
    Type m_type = Type::OTHER;
    bool m_isSelectable = false;

    QVector<NCriticalPathItem*> m_childItems;
    QVector<QVariant> m_itemData;
    NCriticalPathItem* m_parentItem = nullptr;
    QString m_startPointLine;
    QString m_endPointLine;
};

} // namespace FOEDAG
