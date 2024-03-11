#include "NCriticalPathFilterModel.h"
#include "NCriticalPathItem.h"

namespace FOEDAG {

bool NCriticalPathFilterModel::setFilterCriteria(const FilterCriteriaConf& inputCriteriaConf, const FilterCriteriaConf& outputCriteriaConf)
{
    bool isInputCriteriaChanged = m_inputCriteriaConf.set(inputCriteriaConf);
    bool isOutoutCriteriaChanged = m_outputCriteriaConf.set(outputCriteriaConf);

    bool isChanged = isInputCriteriaChanged || isOutoutCriteriaChanged;
    if (isChanged) {
        invalidateFilter();
    }
    return isChanged;
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

void NCriticalPathFilterModel::clear()
{
    resetFilterCriteria();
}

void NCriticalPathFilterModel::resetFilterCriteria()
{
    setFilterCriteria(FilterCriteriaConf{}, FilterCriteriaConf{});
}

} // namespace FOEDAG