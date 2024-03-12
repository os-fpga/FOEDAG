/**
  * @file NCriticalPathFilterModel.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
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

#pragma once

#include "FilterCriteriaConf.h"

#include <QSortFilterProxyModel>

namespace FOEDAG {
class NCriticalPathFilterModel final: public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit NCriticalPathFilterModel(QObject* parent = nullptr): QSortFilterProxyModel(parent) {}
    ~NCriticalPathFilterModel() override final=default;

    bool setFilterCriteria(const FilterCriteriaConf& inputCriteria, const FilterCriteriaConf& outputCriteria);
    void clear();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParentIndex) const override final;

private:
    FilterCriteriaConf m_inputCriteriaConf;
    FilterCriteriaConf m_outputCriteriaConf;

    void resetFilterCriteria();
};

} // namespace FOEDAG
