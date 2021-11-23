#include "project_option.h"

using namespace FOEDAG;

ProjectOption::ProjectOption(QObject *parent) : QObject(parent) {
  m_mapOption.clear();
}

ProjectOption &ProjectOption::operator=(const ProjectOption &other) {
  if (this == &other) {
    return *this;
  }
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

QMap<QString, QString> ProjectOption::getMapOption() const {
  return m_mapOption;
}
