#include "project.h"

using namespace FOEDAG;

Q_GLOBAL_STATIC(Project, project)

Project *Project::Instance() { return project(); }

void Project::InitProject() {
  m_projectName.clear();
  m_projectPath.clear();
  delete m_projectConfig;
  m_projectConfig = new ProjectConfiguration;
  m_projectConfig->moveToThread(thread());
  m_projectConfig->setParent(this);
  qDeleteAll(m_mapProjectRun);
  m_mapProjectRun.clear();
  qDeleteAll(m_mapProjectFileset);
  m_mapProjectFileset.clear();
}

QString Project::projectName() const { return m_projectName; }

void Project::setProjectName(const QString &projectName) {
  m_projectName = projectName;
}

QString Project::projectPath() const { return m_projectPath; }

void Project::setProjectPath(const QString &projectPath) {
  m_projectPath = projectPath;
  emit projectPathChanged();
}

ProjectConfiguration *Project::projectConfig() const { return m_projectConfig; }

ProjectFileSet *Project::getProjectFileset(const QString &strName) const {
  ProjectFileSet *retProFileSet = nullptr;
  auto iter = m_mapProjectFileset.find(strName);
  if (iter != m_mapProjectFileset.end()) {
    retProFileSet = iter.value();
  }
  return retProFileSet;
}

int Project::setProjectFileset(const ProjectFileSet &projectFileset) {
  int ret = 0;
  ProjectFileSet *pf = new ProjectFileSet;
  if (nullptr != pf) {
    *pf = projectFileset;
    auto iter = m_mapProjectFileset.find(pf->getSetName());
    if (iter != m_mapProjectFileset.end()) {
      ret = 1;
    } else {
      m_mapProjectFileset.insert(pf->getSetName(), pf);
    }
  } else {
    ret = -1;
  }
  return ret;
}

void Project::deleteProjectFileset(const QString &strName) {
  ProjectFileSet *proFileSet = nullptr;
  auto iter = m_mapProjectFileset.find(strName);
  if (iter != m_mapProjectFileset.end()) {
    proFileSet = iter.value();
    delete proFileSet;
    m_mapProjectFileset.erase(iter);
  }
}

ProjectRun *Project::getProjectRun(const QString &strName) const {
  ProjectRun *retProRun = nullptr;
  auto iter = m_mapProjectRun.find(strName);
  if (iter != m_mapProjectRun.end()) {
    retProRun = iter.value();
  }
  return retProRun;
}

int Project::setProjectRun(const ProjectRun &projectRun) {
  int ret = 0;
  ProjectRun *pr = new ProjectRun;
  if (nullptr != pr) {
    *pr = projectRun;
    auto iter = m_mapProjectRun.find(pr->runName());
    if (iter != m_mapProjectRun.end()) {
      ret = 1;
    } else {
      m_mapProjectRun.insert(pr->runName(), pr);
    }
  } else {
    ret = -1;
  }
  return ret;
}

void Project::deleteprojectRun(const QString &strName) {
  ProjectRun *proRun = nullptr;
  auto iter = m_mapProjectRun.find(strName);
  if (iter != m_mapProjectRun.end()) {
    proRun = iter.value();
    delete proRun;
    m_mapProjectRun.erase(iter);
  }
}

QMap<QString, ProjectFileSet *> Project::getMapProjectFileset() const {
  return m_mapProjectFileset;
}

QMap<QString, ProjectRun *> Project::getMapProjectRun() const {
  return m_mapProjectRun;
}
