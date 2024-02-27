#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QTimer>

#include "NCriticalPathReportParser.h"

class NCriticalPathItem;

class NCriticalPathModel final : public QAbstractItemModel
{
    Q_OBJECT

    const int LINE_LIMITER_FILTER_TIME_MS = 1000;
    const int LINE_CHAR_NUM_MIN = 10;

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

    void limitLineCharsNum(std::size_t lineCharsMaxNum);

public slots:
    void loadFromString(const QString&);

signals:
    void loadFinished();
    void cleared();

private:
    NCriticalPathItem* m_rootItem = nullptr;

    std::map<QString, int> m_inputNodes;
    std::map<QString, int> m_outputNodes;

    QTimer m_lineLimiterTimer; // a single-shot timer is used to unite multiple requests into one
    std::size_t m_lineCharsMaxNum = 0;

    void setupModelData(const std::vector<GroupPtr>& groups);

    std::tuple<QString, QString, QString> extractRow(QString) const;
    void load(const std::vector<std::string>&);

    void insertNewItem(NCriticalPathItem* parentItem, NCriticalPathItem* newItem);
    int findRow(NCriticalPathItem*) const;
    int findColumn(NCriticalPathItem*) const;

    void applyLineCharsNum();
};

