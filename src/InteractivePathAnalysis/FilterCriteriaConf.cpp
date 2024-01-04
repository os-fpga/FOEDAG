#include "filtercriteriaconf.h"

const QString FilterCriteriaConf::KEY_ANY_MASK = "*";

FilterCriteriaConf::FilterCriteriaConf(const QString& criteria, bool caseSensetive, bool useRegExp):
        m_criteria(criteria)
    , m_caseSensetive(caseSensetive)
    , m_useRegExp(useRegExp)
{}

bool FilterCriteriaConf::set(const FilterCriteriaConf& rhs) 
{
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

