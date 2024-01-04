#include "NCriticalPathItem.h"
#include "SimpleLogger.h"

NCriticalPathItem::NCriticalPathItem(bool isPath, const QVector<QVariant>& data, bool isSelectable, NCriticalPathItem* parent)
    : m_isPath(isPath), m_isSelectable(isSelectable), m_itemData(data), m_parentItem(parent)
{
    //SimpleLogger::instance().debug("added",  m_itemData,  m_isSelectable);
    if (isPath) {
        QList<QString> d = data[0].toString().split('\n');
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
