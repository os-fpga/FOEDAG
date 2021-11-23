#include "project.h"

using namespace FOEDAG;

Q_GLOBAL_STATIC(Project, project)

Project *Project::Instance() { return project(); }

void Project::InitProject() {
  m_projectName = "";
  m_projectPath = "";
  if (nullptr != m_projectConfig) {
    delete m_projectConfig;
  }
  m_projectConfig = new ProjectConfiguration(this);
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

void Project::setProjectFileset(ProjectFileSet *projectFileset) {
  if (nullptr != projectFileset) {
    auto iter = m_mapProjectFileset.find(projectFileset->getSetName());
    if (iter != m_mapProjectFileset.end()) {
      delete iter.value();
    }
    m_mapProjectFileset.insert(projectFileset->getSetName(), projectFileset);
  }
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

void Project::setProjectRun(ProjectRun *projectRun) {
  if (nullptr != projectRun) {
    auto iter = m_mapProjectRun.find(projectRun->runName());
    if (iter != m_mapProjectRun.end()) {
      delete iter.value();
    }
    m_mapProjectRun.insert(projectRun->runName(), projectRun);
  }
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
