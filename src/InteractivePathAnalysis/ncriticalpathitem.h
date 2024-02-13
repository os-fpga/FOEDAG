#pragma once

#include <QVariant>
#include <QVector>

class NCriticalPathItem
{
public:
    explicit NCriticalPathItem(bool isPath, const QVector<QVariant>& data, bool isSelectable, NCriticalPathItem* parentItem = nullptr);
    ~NCriticalPathItem();

    const QString& startPointLine() const { return m_startPointLine; }
    const QString& endPointLine() const { return m_endPointLine; }

    void deleteChildItems();

    void appendChild(NCriticalPathItem* child);

    bool isPath() const { return m_isPath; }
    bool isSelectable() const { return m_isSelectable; }
    NCriticalPathItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    NCriticalPathItem* parentItem();

private:
    bool m_isPath = false;
    bool m_isSelectable = false;
    QVector<NCriticalPathItem*> m_childItems;
    QVector<QVariant> m_itemData;
    NCriticalPathItem* m_parentItem = nullptr;
    QString m_startPointLine;
    QString m_endPointLine;
};


