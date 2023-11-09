#pragma once

#include <QVariant>
#include <QVector>

class NCriticalPathItem
{
public:
    explicit NCriticalPathItem(const QVector<QVariant>& data, bool isSelectable, NCriticalPathItem* parentItem = nullptr);
    ~NCriticalPathItem();

    void deleteChildItems();

    void appendChild(NCriticalPathItem* child);

    bool isSelectable() const { return m_isSelectable; }
    NCriticalPathItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    NCriticalPathItem* parentItem();

private:
    bool m_isSelectable = false;
    QVector<NCriticalPathItem*> m_childItems;
    QVector<QVariant> m_itemData;
    NCriticalPathItem* m_parentItem = nullptr;
};


