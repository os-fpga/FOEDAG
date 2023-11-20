#include "ncriticalpathfiltermodel.h"

#include <QDebug>

void NCriticalPathFilterModel::setFilterCriteria(const QString& criteria)
{
    // TODO: implement logic here
    invalidateFilter();
}

bool NCriticalPathFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // TODO: implement logic here
    return true;
}
