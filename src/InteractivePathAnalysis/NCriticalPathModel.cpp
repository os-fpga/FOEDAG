#include "NCriticalPathModel.h"
#include "NCriticalPathItem.h"
#include "SimpleLogger.h"

//#define DEBUG_DUMP_RECEIVED_CRIT_PATH_TO_FILE

NCriticalPathModel::NCriticalPathModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    qRegisterMetaType<ItemsHelperStructPtr>("std::shared_ptr<ItemsHelperStruct>");
    m_rootItem = new NCriticalPathItem;
}

NCriticalPathModel::~NCriticalPathModel()
{
    delete m_rootItem;
}

void NCriticalPathModel::clear()
{
    SimpleLogger::instance().debug("clear path model");
    beginResetModel();
    m_rootItem->deleteChildItems();
    endResetModel();

    m_inputNodes.clear();
    m_outputNodes.clear();

    emit cleared();
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

{
    NCriticalPathModelLoader* modelLoader = new NCriticalPathModelLoader(std::move(rawData));
    connect(modelLoader, &NCriticalPathModelLoader::itemsReady, this, &NCriticalPathModel::loadItems);
    connect(modelLoader, &QThread::finished, modelLoader, &QThread::deleteLater);
    modelLoader->start();
}

void NCriticalPathModel::loadItems(const ItemsHelperStructPtr& itemsHelperStructPtr)
{
    clear();

    for (const auto& [item, rootItem]: itemsHelperStructPtr->items) {
        if (rootItem) {
            insertNewItem(rootItem, item);   
        } else {
            insertNewItem(m_rootItem, item);  
        }
    }

    m_inputNodes = std::move(itemsHelperStructPtr->inputNodes);
    m_outputNodes = std::move(itemsHelperStructPtr->outputNodes);
    itemsHelperStructPtr->inputNodes.clear();
    itemsHelperStructPtr->outputNodes.clear();

    emit loadFinished();
    SimpleLogger::instance().debug("load model finished");
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
    newItem->setParent(parentItem);
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

