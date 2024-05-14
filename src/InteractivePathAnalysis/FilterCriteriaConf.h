#pragma once

#include <QString>

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
