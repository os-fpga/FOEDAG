/**
  * @file FilterCriteriaConf.h
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

#pragma once

#include <QString>

namespace FOEDAG {

/**
 * @brief Filter Criteria Configuration
 *
 * Implements a basic filter data structure used in conjunction with
 * NCriticalPathFilterModel.
 */
class FilterCriteriaConf {
 public:
  static const QString KEY_ANY_MASK;
  explicit FilterCriteriaConf(const QString& criteria, bool caseSensetive,
                              bool useRegExp);
  FilterCriteriaConf() = default;

  bool set(const FilterCriteriaConf& rhs);

  bool useRegExp() const { return m_useRegExp; }

  QString criteria() const { return m_criteria; }

  Qt::CaseSensitivity caseSensetive() const {
    return m_caseSensetive ? Qt::CaseSensitive : Qt::CaseInsensitive;
  }

  bool isSet() const { return !m_criteria.isEmpty(); }

 private:
  QString m_criteria;
  bool m_caseSensetive = false;
  bool m_useRegExp = false;
};

}  // namespace FOEDAG
