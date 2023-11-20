#pragma once

#include <QSortFilterProxyModel>

class NCriticalPathFilterModel final: public QSortFilterProxyModel {
    Q_OBJECT

public:
    NCriticalPathFilterModel(QObject* parent = nullptr): QSortFilterProxyModel(parent) {}
    ~NCriticalPathFilterModel() override final=default;

    void setFilterCriteria(const QString& criteria);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override final;
};
