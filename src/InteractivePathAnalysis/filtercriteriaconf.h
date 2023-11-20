#pragma once

#include <QString>

static const QString KEY_ANY_MASK = "*";

class FilterCriteriaConf {
public:
    explicit FilterCriteriaConf(const QString& criteria, bool caseSensetive, bool useRegExp):
          m_criteria(criteria)
        , m_caseSensetive(caseSensetive)
        , m_useRegExp(useRegExp)
    {}

    FilterCriteriaConf()=default;

    bool set(const FilterCriteriaConf& rhs) {
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

    QString criteria() const { return m_criteria; }

    Qt::CaseSensitivity caseSensetive() const { return m_caseSensetive? Qt::CaseSensitive: Qt::CaseInsensitive; }

    bool isSet() const { return !m_criteria.isEmpty(); }

private:
    QString m_criteria;
    bool m_caseSensetive = false;
    bool m_useRegExp = false;
};
