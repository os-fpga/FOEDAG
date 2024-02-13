#include "ncriticalpathmodel.h"
#include "ncriticalpathitem.h"
#include "simplelogger.h"

#include <QFile>
#include <QList>
#include <QRegularExpression>

//#define DEBUG_DUMP_RECEIVED_CRIT_PATH_TO_FILE

NCriticalPathModel::NCriticalPathModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new NCriticalPathItem(false, {"", "", ""}, false);
}

NCriticalPathModel::~NCriticalPathModel()
{
    delete m_rootItem;
}

void NCriticalPathModel::clear()
{
    SimpleLogger::instance().debug("clear model start");
    beginResetModel();
    m_rootItem->deleteChildItems();
    endResetModel();
    SimpleLogger::instance().debug("clear model finished");

    m_inputNodes.clear();
    m_outputNodes.clear();

    emit cleared();
}

void NCriticalPathModel::loadFromString(const QString& data)
{
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

    QList<QString> lines_ = data.split("\n");
    std::vector<std::string> lines;
    lines.reserve(lines_.size());
    for (const QString& line: qAsConst(lines_)) {
        lines.push_back(line.toStdString());
    }
    load(lines);
}

void NCriticalPathModel::load(const std::vector<std::string>& lines)
{
    clear();

    std::vector<BlockPtr> blocks = parsePathData(lines);
    setupModelData(blocks);

    emit loadFinished();
    SimpleLogger::instance().debug("finish model setup");
}

int NCriticalPathModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return static_cast<NCriticalPathItem*>(parent.internalPointer())->columnCount();
    }
    return m_rootItem->columnCount();
}

QVariant NCriticalPathModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
    if (item) {
        switch (role) {
        case Qt::DisplayRole: return item->data(index.column());
        case Qt::DecorationRole: return item->isSelectable();
        }
    }

    return QVariant();
}

bool NCriticalPathModel::isSelectable(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return false;
    }
    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
    return item->isSelectable();
}

Qt::ItemFlags NCriticalPathModel::flags(const QModelIndex& index) const
{
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

QVariant NCriticalPathModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_rootItem->data(section);
    }

    return QVariant();
}

QModelIndex NCriticalPathModel::index(int row, int column, const QModelIndex& parentIndex) const
{
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

QModelIndex NCriticalPathModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    NCriticalPathItem* childItem = static_cast<NCriticalPathItem*>(index.internalPointer());
    NCriticalPathItem* parentItem = childItem->parentItem();

    if (parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int NCriticalPathModel::rowCount(const QModelIndex& parent) const
{
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

QModelIndex NCriticalPathModel::findPathIndex(const QString& pathId)
{
    for (int row = 0; row < rowCount(); ++row) {
        for (int col = 0; col < columnCount(); ++col) {
            QModelIndex index_ = index(row, col);
            QVariant data_ = data(index_, Qt::DisplayRole);
            if (data_.isValid()) {
                if (pathId == data_.toString()) {
                    SimpleLogger::instance().debug("Index:", index_.row(), index_.column(), "Data:", data_.toString());
                    return index_;
                }
            }
        }
    }
    return QModelIndex();
}

void NCriticalPathModel::setupModelData(const std::vector<BlockPtr>& blocks)
{
    assert(m_rootItem);

    m_inputNodes.clear();
    m_outputNodes.clear();

    NCriticalPathItem* pathItem = nullptr;

    for (const BlockPtr& block: blocks) {
        if (block->isPath()) {
            QList<QString> pathParts;
            QList<Line> segments;
            for (const Line& line: block->lines) {
                if (line.role == Role::PATH) {
                    pathParts.append(line.line.c_str());
                } else {
                    segments.append(line);
                }
            }

            // path item
            pathItem = new NCriticalPathItem(true, {pathParts.join("\n"), "", ""}, true, m_rootItem);
            insertNewItem(m_rootItem, pathItem);

            //segment items
            for (const Line& segment: segments) {
                QString l(segment.line.c_str());
                l = l.trimmed();
                auto segmentRow = extractRow(l);
#ifdef ALLOW_CRITICAL_PATH_SEGMENS_SELECTION
                bool isSelectable = (segment.role == Role::SEGMENT);
#else
                bool isSelectable = false;
#endif
                NCriticalPathItem* newItem = new NCriticalPathItem(false, segmentRow, isSelectable, pathItem);
                insertNewItem(pathItem, newItem);
            }

            // handle input
            QString input{block->pathInfo.start.c_str()};
            if (m_inputNodes.find(input) == m_inputNodes.end()) {
                m_inputNodes[input] = 1;
            } else {
                m_inputNodes[input]++;
            }

            // handle output
            QString output{block->pathInfo.end.c_str()};
            if (m_outputNodes.find(output) == m_outputNodes.end()) {
                m_outputNodes[output] = 1;
            } else {
                m_outputNodes[output]++;
            }

        } else {
            for (const Line& line: block->lines) {
                NCriticalPathItem* newItem = new NCriticalPathItem(false, {line.line.c_str(), "", ""}, false, m_rootItem);
                insertNewItem(m_rootItem, newItem);
            }
        }
    }
}

int NCriticalPathModel::findRow(NCriticalPathItem* item) const
{
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

int NCriticalPathModel::findColumn(NCriticalPathItem*) const
{
    return 0;  // assuming a single-column tree structure
}

void NCriticalPathModel::insertNewItem(NCriticalPathItem* parentItem, NCriticalPathItem* newItem)
{
    int row = parentItem->childCount();
    QModelIndex parentIndex;
    if (parentItem != m_rootItem) {
        int parentRow = findRow(parentItem);
        int parentColumn = findColumn(parentItem);
        parentIndex = createIndex(parentRow, parentColumn, const_cast<NCriticalPathItem*>(parentItem));
    }
    beginInsertRows(parentIndex, row, row);
    parentItem->appendChild(newItem);
    endInsertRows();
}

QVector<QVariant> NCriticalPathModel::extractRow(QString l) const
{
    l = l.trimmed();

    // Using regular expressions to remove consecutive white spaces
    static QRegularExpression regex("\\s+");
    l = l.trimmed().replace(regex, " ");


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
