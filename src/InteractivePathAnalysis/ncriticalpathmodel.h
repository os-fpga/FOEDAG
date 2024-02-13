#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "pathrpthelper.h"

class NCriticalPathItem;

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
    QModelIndex findPathIndex(const QString& pathId);
    QModelIndex parent(const QModelIndex &index) const override final;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override final;

    bool isSelectable(const QModelIndex &index) const;

public slots:
    void loadFromString(const QString&);

signals:
    void loadFinished();
    void cleared();

private:
    NCriticalPathItem* m_rootItem = nullptr;

    std::map<QString, int> m_inputNodes;
    std::map<QString, int> m_outputNodes;

    void setupModelData(const std::vector<BlockPtr>& blocks);

    QVector<QVariant> extractRow(QString) const;
    void load(const std::vector<std::string>&);

    void insertNewItem(NCriticalPathItem* parentItem, NCriticalPathItem* newItem);
    int findRow(NCriticalPathItem*) const;
    int findColumn(NCriticalPathItem*) const;
};

