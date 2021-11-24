#include "project_run.h"

using namespace FOEDAG;

ProjectRun::ProjectRun(QObject *parent) : ProjectOption(parent) {
  m_runName = "";
  m_runType = "";
  m_srcSet = "";
  m_constrsSet = "";
  m_runState = "";
  m_synthRun = "";
}

ProjectRun &ProjectRun::operator=(const ProjectRun &other) {
  if (this == &other) {
    return *this;
  }
  this->m_runName = other.m_runName;
  this->m_runType = other.m_runType;
  this->m_runState = other.m_runState;
  this->m_srcSet = other.m_srcSet;
  this->m_synthRun = other.m_synthRun;
  this->m_constrsSet = other.m_constrsSet;
  ProjectOption::operator=(other);

  return *this;
}

QString ProjectRun::runName() const { return m_runName; }

void ProjectRun::setRunName(const QString &runName) { m_runName = runName; }

QString ProjectRun::runType() const { return m_runType; }

void ProjectRun::setRunType(const QString &runType) { m_runType = runType; }

QString ProjectRun::srcSet() const { return m_srcSet; }

void ProjectRun::setSrcSet(const QString &srcSet) { m_srcSet = srcSet; }

QString ProjectRun::constrsSet() const { return m_constrsSet; }

void ProjectRun::setConstrsSet(const QString &constrsSet) {
  m_constrsSet = constrsSet;
}

QString ProjectRun::runState() const { return m_runState; }

void ProjectRun::setRunState(const QString &runState) { m_runState = runState; }

QString ProjectRun::synthRun() const { return m_synthRun; }

void ProjectRun::setSynthRun(const QString &synthRun) { m_synthRun = synthRun; }
