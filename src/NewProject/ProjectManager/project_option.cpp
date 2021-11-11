#include "project_option.h"

ProjectOption::ProjectOption(QObject *parent) : QObject(parent) {}

ProjectOption &ProjectOption::operator=(const ProjectOption &other) {
  this->m_mapOption = other.m_mapOption;
  return *this;
}

void ProjectOption::setOption(const QString &strKey, const QString &strValue) {
  m_mapOption[strKey] = strValue;
}

QString ProjectOption::getOption(QString strKey) {
  QString retStr = "";
  auto iter = m_mapOption.find(strKey);
  if (iter != m_mapOption.end()) {
    retStr = iter.value();
  }
  return retStr;
}
