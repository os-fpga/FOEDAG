#include "NCriticalPathItem.h"
#include "SimpleLogger.h"

NCriticalPathItem::NCriticalPathItem()
{
    m_itemData.resize(7);
    m_itemData[DATA] = "";
    m_itemData[VAL1] = "";
    m_itemData[VAL2] = "";
    m_itemData[TYPE] = "";
    m_itemData[INDEX] = -1;
    m_itemData[PARENT_ID] = "";
    m_itemData[IS_SELECTABLE] = false;
}

NCriticalPathItem::NCriticalPathItem(const QString& data, 
        const QString& val1, 
        const QString& val2, 
        const QString& type, 
        int index,  
        const QString& parentId,
        bool isSelectable, 
        NCriticalPathItem* parent)
    : m_parentItem(parent)
{
    m_itemData.resize(7);
    m_itemData[DATA] = data;
    m_itemData[VAL1] = val1;
    m_itemData[VAL2] = val2;
    m_itemData[TYPE] = type;
    m_itemData[INDEX] = index;
    m_itemData[PARENT_ID] = parentId;
    m_itemData[IS_SELECTABLE] = isSelectable;

    //SimpleLogger::instance().debug("added",  m_itemData);

    if (isPath()) {
        QList<QString> d = data.split('\n');
        for (const QString& e: std::as_const(d)) {
            if (e.startsWith("Startpoint")) {
                m_startPointLine = e;
            } else if (e.startsWith("Endpoint")) {
                m_endPointLine = e;
            }
        }
    }
}

NCriticalPathItem::~NCriticalPathItem()
{
    deleteChildItems();
}

void NCriticalPathItem::deleteChildItems()
{
    if (!m_childItems.isEmpty()) {
        qDeleteAll(m_childItems);
        m_childItems.clear();
    }
}

void NCriticalPathItem::appendChild(NCriticalPathItem* item)
{
    m_childItems.append(item);
}

NCriticalPathItem* NCriticalPathItem::child(int row)
{
    if ((row < 0) || (row >= m_childItems.size())) {
        return nullptr;
    }
    return m_childItems.at(row);
}

int NCriticalPathItem::childCount() const
{
    return m_childItems.count();
}

int NCriticalPathItem::columnCount() const
{
    return m_itemData.count();
}

QVariant NCriticalPathItem::data(int column) const
{
    if ((column < 0) || (column >= m_itemData.size())) {
        return QVariant();
    }
    return m_itemData.at(column);
}

NCriticalPathItem* NCriticalPathItem::parentItem()
{
    return m_parentItem;
}

int NCriticalPathItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<NCriticalPathItem*>(this));
    }

    return 0;
}
