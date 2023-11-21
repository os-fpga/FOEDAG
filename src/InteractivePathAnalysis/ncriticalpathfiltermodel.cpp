#include "ncriticalpathfiltermodel.h"
#include "ncriticalpathitem.h"

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
        if (m_inputCriteriaConf.useRegExp()) {
            QRegularExpression pattern(m_inputCriteriaConf.criteria());
            return pattern.match(item->startPointLine()).hasMatch();
        } else {
            inputMeetsCriteria = item->startPointLine().contains(m_inputCriteriaConf.criteria(), m_inputCriteriaConf.caseSensetive());
        }
    }

    bool outputMeetsCriteria = true;
    if (m_outputCriteriaConf.isSet()) {
        if (m_outputCriteriaConf.useRegExp()) {
            QRegularExpression pattern(m_outputCriteriaConf.criteria());
            return pattern.match(item->endPointLine()).hasMatch();
        } else {
            outputMeetsCriteria = item->endPointLine().contains(m_outputCriteriaConf.criteria(), m_outputCriteriaConf.caseSensetive());
        }
    }

    return inputMeetsCriteria && outputMeetsCriteria;
}
