#include "ncriticalpathfiltermodel.h"
#include "ncriticalpathitem.h"

#include <QDebug>

void NCriticalPathFilterModel::setFilterCriteria(const FilterCriteriaConf& inputCriteriaConf, const FilterCriteriaConf& outputCriteriaConf)
{
    bool isInputCriteriaChanged = m_inputCriteriaConf.set(inputCriteriaConf);
    bool isOutoutCriteriaChanged = m_outputCriteriaConf.set(outputCriteriaConf);

    if (isInputCriteriaChanged || isOutoutCriteriaChanged) {
        invalidateFilter();
    }
}

bool NCriticalPathFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParentIndex) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParentIndex);
    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
    if (!item) {
        return false;
    }
    if (!item->isPath()) {
        return true; // we don't apply filter on path segments for now
    }

    bool inputMeetsCriteria = true;
    if (m_inputCriteriaConf.isSet()) {
        inputMeetsCriteria = item->startPointLine().contains(m_inputCriteriaConf.criteria(), m_inputCriteriaConf.caseSensetive());
    }

    bool outputMeetsCriteria = true;
    if (m_inputCriteriaConf.isSet()) {
        inputMeetsCriteria = item->startPointLine().contains(m_inputCriteriaConf.criteria(), m_inputCriteriaConf.caseSensetive());
    }

    return inputMeetsCriteria && outputMeetsCriteria;
}
