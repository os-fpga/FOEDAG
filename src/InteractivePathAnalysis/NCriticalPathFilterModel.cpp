/**
  * @file NCriticalPathFilterModel.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathFilterModel.h"

#include "NCriticalPathItem.h"

namespace FOEDAG {

bool NCriticalPathFilterModel::setFilterCriteria(
    const FilterCriteriaConf& inputCriteriaConf,
    const FilterCriteriaConf& outputCriteriaConf) {
  bool isInputCriteriaChanged = m_inputCriteriaConf.set(inputCriteriaConf);
  bool isOutoutCriteriaChanged = m_outputCriteriaConf.set(outputCriteriaConf);

  bool isChanged = isInputCriteriaChanged || isOutoutCriteriaChanged;
  if (isChanged) {
    invalidateFilter();
  }
  return isChanged;
}

bool NCriticalPathFilterModel::filterAcceptsRow(
    int sourceRow, const QModelIndex& sourceParentIndex) const {
  QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParentIndex);
  NCriticalPathItem* item =
      static_cast<NCriticalPathItem*>(index.internalPointer());
  if (!item) {
    return false;
  }
  if (!item->isPath()) {
    return true;  // we don't apply filter on path segments for now
  }

  bool inputMeetsCriteria = true;
  if (m_inputCriteriaConf.isSet()) {
    if (m_inputCriteriaConf.useRegExp()) {
      QRegularExpression pattern(m_inputCriteriaConf.criteria());
      return pattern.match(item->startPointLine()).hasMatch();
    } else {
      inputMeetsCriteria = item->startPointLine().contains(
          m_inputCriteriaConf.criteria(), m_inputCriteriaConf.caseSensetive());
    }
  }

  bool outputMeetsCriteria = true;
  if (m_outputCriteriaConf.isSet()) {
    if (m_outputCriteriaConf.useRegExp()) {
      QRegularExpression pattern(m_outputCriteriaConf.criteria());
      return pattern.match(item->endPointLine()).hasMatch();
    } else {
      outputMeetsCriteria =
          item->endPointLine().contains(m_outputCriteriaConf.criteria(),
                                        m_outputCriteriaConf.caseSensetive());
    }
  }

  return inputMeetsCriteria && outputMeetsCriteria;
}

void NCriticalPathFilterModel::clear() { resetFilterCriteria(); }

void NCriticalPathFilterModel::resetFilterCriteria() {
  setFilterCriteria(FilterCriteriaConf{}, FilterCriteriaConf{});
}

}  // namespace FOEDAG
