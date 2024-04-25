#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QHash>

#include <QThread>
#include <QDebug> 

#include <memory>

#include "NCriticalPathReportParser.h"

class NCriticalPathItem;

using ItemsStructPtr = std::shared_ptr<std::vector<std::pair<NCriticalPathItem*, NCriticalPathItem*>>>;
Q_DECLARE_METATYPE(ItemsStructPtr)

class ModelLoaderThread : public QThread {
    Q_OBJECT
public:
    explicit ModelLoaderThread(QString&& rawData) : QThread(nullptr), m_rawData(rawData) {}
    ~ModelLoaderThread() {}

signals:
    void itemsReady(const ItemsStructPtr&);

protected:
    void run() override;

private:
    QString m_rawData;

    void createItems(const std::vector<GroupPtr>& groups);
    std::tuple<QString, QString, QString> extractRow(QString) const;
};

class NCriticalPathModel final : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit NCriticalPathModel(QObject* parent = nullptr);
    ~NCriticalPathModel() override final;

    const std::map<QString, int>& inputNodes() const { return m_inputNodes; }
    const std::map<QString, int>& outputNodes() const { return m_outputNodes; }

    void clear();

    QVariant data(const QModelIndex &index, int role) const override final;
    Qt::ItemFlags flags(const QModelIndex &index) const override final;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override final;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
    QModelIndex findPathElementIndex(const QModelIndex& pathIndex, const QString& dataElement, int column); 
    QModelIndex parent(const QModelIndex &index) const override final;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override final;

    bool isSelectable(const QModelIndex &index) const;

 public slots:
    void loadFromString(QString rawData);
    void loadItems(const ItemsStructPtr& itemsPtr);

signals:
    void loadFinished();
    void cleared();

private:
    NCriticalPathItem* m_rootItem = nullptr;

    std::map<QString, int> m_inputNodes;
    std::map<QString, int> m_outputNodes;

    void insertNewItem(NCriticalPathItem* parentItem, NCriticalPathItem* newItem);
    int findRow(NCriticalPathItem*) const;
    int findColumn(NCriticalPathItem*) const;
};

