#pragma once

#include <QVariant>
#include <QVector>


class NCriticalPathItem
{
public:
    enum {
        DATA,
        VAL1,
        VAL2,
        TYPE,
        INDEX,
        PARENT_ID,
        IS_SELECTABLE
    };
    NCriticalPathItem();
    explicit NCriticalPathItem(
        const QString& data, 
        const QString& val1, 
        const QString& val2, 
        const QString& type, 
        int index,  
        const QString& parent,
        bool isSelectable,       
        NCriticalPathItem* parentItem = nullptr);

    ~NCriticalPathItem();

    const QString& startPointLine() const { return m_startPointLine; }
    const QString& endPointLine() const { return m_endPointLine; }

    void deleteChildItems();

    void appendChild(NCriticalPathItem* child);

    int id() const { return m_itemData[INDEX].toInt(); }
    QString type() const { return m_itemData[TYPE].toString(); }

    bool isPath() const { return type() == "p"; }
    bool isSelectable() const { return m_itemData[IS_SELECTABLE].toBool(); }

    NCriticalPathItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    NCriticalPathItem* parentItem();

private:
    QVector<NCriticalPathItem*> m_childItems;
    QVector<QVariant> m_itemData;
    NCriticalPathItem* m_parentItem = nullptr;
    QString m_startPointLine;
    QString m_endPointLine;
};


