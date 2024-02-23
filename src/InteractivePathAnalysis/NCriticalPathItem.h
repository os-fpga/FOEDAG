#pragma once

#include <QVariant>
#include <QVector>
#include <QString>

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

    bool limitLineCharsNum(std::size_t);

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

    QString m_dataOrig;
    std::optional<std::size_t> m_appliedLineCharsMaxNumOpt;

    QVector<NCriticalPathItem*> m_childItems;
    QVector<QVariant> m_itemData;
    NCriticalPathItem* m_parentItem = nullptr;
    QString m_startPointLine;
    QString m_endPointLine;
};


