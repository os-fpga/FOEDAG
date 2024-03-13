/**
  * @file FilterCriteriaConf.cpp
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

#include "FilterCriteriaConf.h"

namespace FOEDAG {

const QString FilterCriteriaConf::KEY_ANY_MASK = "*";

FilterCriteriaConf::FilterCriteriaConf(const QString& criteria,
                                       bool caseSensetive, bool useRegExp)
    : m_criteria(criteria),
      m_caseSensetive(caseSensetive),
      m_useRegExp(useRegExp) {}

bool FilterCriteriaConf::set(const FilterCriteriaConf& rhs) {
  bool is_changed = false;
  if (m_criteria != rhs.m_criteria) {
    m_criteria = rhs.m_criteria;
    is_changed = true;
  }
  if (m_caseSensetive != rhs.m_caseSensetive) {
    m_caseSensetive = rhs.m_caseSensetive;
    is_changed = true;
  }
  if (m_useRegExp != rhs.m_useRegExp) {
    m_useRegExp = rhs.m_useRegExp;
    is_changed = true;
  }
  return is_changed;
}

}  // namespace FOEDAG
