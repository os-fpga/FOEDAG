#pragma once

#include "filtercriteriaconf.h"

#include <QSortFilterProxyModel>

class NCriticalPathFilterModel final: public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit NCriticalPathFilterModel(QObject* parent = nullptr): QSortFilterProxyModel(parent) {}
    ~NCriticalPathFilterModel() override final=default;

    void setFilterCriteria(const FilterCriteriaConf& inputCriteria, const FilterCriteriaConf& outputCriteria);
    void clear();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParentIndex) const override final;

private:
    FilterCriteriaConf m_inputCriteriaConf;
    FilterCriteriaConf m_outputCriteriaConf;

    void resetFilterCriteria();
};
